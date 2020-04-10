
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "layers.h"
#include "strips.h"
#include "gluplot.h"

#include "wav_head.h"
#include "fftw3.h"
#include "spectro.h"
#include "process.h"

/** ============================ AUDIO data processing =============== */


// allocation memoire et lecture WAV 16 bits entier en memoire
// donnees stockées dans l'objet process
// puis calculs spectrogramme dans l'objet process
int process::wave_process_full()
{
int retval;

printf("ouverture %s en lecture\n", wnam );
wavp.hand = open( wnam, O_RDONLY | O_BINARY );
if	( wavp.hand == -1 )
	{ printf("file not found %s\n", wnam ); return -1;  }

WAVreadHeader( &wavp );
if	(
	( ( wavp.chan != 1 ) && ( wavp.chan != 2 ) ) ||
	( wavp.resol != 16 )
	)
	{ printf("programme seulement pour fichiers 16 bits mono ou stereo\n"); close(wavp.hand); return -2;  }
printf("freq = %u, block = %u, bpsec = %u, size = %u\n",
	wavp.freq, wavp.block, wavp.bpsec, wavp.wavsize );
fflush(stdout);

// allouer memoire pour lire WAV
Lbuf = (short *)malloc( wavp.wavsize * sizeof(short) );	// pour 1 canal
if	( Lbuf == NULL )
	gasp("echec malloc Lbuf %d samples", (int)wavp.wavsize );

if	( wavp.chan > 1 )
	{
	Rbuf = (short *)malloc( wavp.wavsize * sizeof(short) );	// pour 1 canal
	if	( Rbuf == NULL )
		gasp("echec malloc Rbuf %d samples", (int)wavp.wavsize );
	}

// lecture bufferisee
unsigned int rdbytes, remsamples, rdsamples, rdsamples1c, totalsamples1c = 0;	// "1c" veut dire "pour 1 canal"
#define QRAW 4096
#define Qs16 sizeof(short int)
short int rawsamples16[QRAW];	// buffer pour read()
unsigned int i, j;
j = 0;
while	( totalsamples1c < wavp.wavsize )
	{
	remsamples = ( wavp.wavsize - totalsamples1c ) * wavp.chan;
	if	( remsamples > QRAW )
		remsamples = QRAW;
	rdbytes = read( wavp.hand, rawsamples16, remsamples*Qs16 );
	rdsamples = rdbytes / Qs16;
	if	( rdsamples != remsamples )
		{ printf("truncated WAV data"); close(wavp.hand); return -4;  }
	rdsamples1c = rdsamples / wavp.chan;
	totalsamples1c += rdsamples1c;
	if	( wavp.chan == 2 )
		{
		for	( i = 0; i < rdsamples; i += 2)
			{
			Lbuf[j]   = rawsamples16[i];
			Rbuf[j++] = rawsamples16[i+1];
			}
		}
	else if	( wavp.chan == 1 )
		{
		for	( i = 0; i < rdsamples; ++i )
			Lbuf[j++] = rawsamples16[i];
		}
	}
if	( totalsamples1c != wavp.wavsize )	// cela ne peut pas arriver, cette verif serait parano
	{ printf("WAV size error %u vs %d", totalsamples1c , (int)wavp.wavsize ); close(wavp.hand); return -5;  }
close( wavp.hand );
printf("lu %d samples Ok\n", totalsamples1c ); fflush(stdout);

// creation spectrographe
// qspek = 1;
qspek = wavp.chan;

printf("\nstart init %d spectro\n", qspek ); fflush(stdout);

// parametres primitifs
Lspek.bpst = 10;		// binxel-per-semi-tone : resolution spectro log
Lspek.octaves = 7;		// 7 octaves
Lspek.fftsize = 8192;
Lspek.fftstride = 1024;
Lspek.window_type = 1;
Lspek.midi0 = 28;		// E1 = mi grave de la basse
Lspek.pal = mutpal;		// palette commune
if	( qspek >= 2 )
	{
	Rspek.bpst = 10;		// binxel-per-semi-tone : resolution spectro log
	Rspek.octaves = 7;		// 7 octaves
	Rspek.fftsize = 8192;
	Rspek.fftstride = 1024;
	Rspek.midi0 = 28;		// E1 = mi grave de la basse
	Rspek.window_type = 1;
	Rspek.pal = mutpal;		// palette commune
	}

// allocations pour spectro
if	( qspek >= 1 )
	{
	retval = Lspek.init( wavp.freq, wavp.wavsize );
	if	( retval )
		gasp("erreur init L spectro %d", retval );
	printf("end init L spectro, window type %d, avg %g\n\n", Lspek.window_type, Lspek.window_avg ); fflush(stdout);
	if	( qspek >= 2 )
		{
		retval = Rspek.init( wavp.freq, wavp.wavsize );
		if	( retval )
			gasp("erreur init R spectro %d", retval );
		printf("end init R spectro, window type %d, avg %g\n\n", Rspek.window_type, Rspek.window_avg ); fflush(stdout);
		}
	}
else	gasp("no spek to init");

// calcul spectre : hum, il faut les data en float mais on a lu la wav en s16
// provisoirement on va convertir en float l'audio qu'on a deja en RAM
{
printf("start calcul spectre\n"); fflush(stdout);
float * raw32 = (float *) malloc( wavp.wavsize * sizeof(float) );
if	( raw32 == NULL )
	gasp("echec alloc %d floats pour source fft", wavp.wavsize );
printf("Ok alloc %d floats pour source fft\n", wavp.wavsize );
unsigned int i;
if	( wavp.chan == 1 )
	{
	for	( i = 0; i < wavp.wavsize; ++i )
		raw32[i] = (1.0/32768.0) * (float)Lbuf[i];	// normalisation a la WAV32
		// raw32[i] = 2.0 * (float)wL->V[i];		// coeff de JAW04b
	Lspek.compute( raw32 );
	}
else if	( wavp.chan == 2 )
	{
	if	( qspek == 1 )	// spectre mono sur WAV stereo
		{
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (0.5/32768.0) * ((float)Lbuf[i] + (float)Rbuf[i]);	// normalisation a la WAV32
		Lspek.compute( raw32 );
		}
	else if	( qspek == 2 )
		{
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (1.0/32768.0) * (float)Lbuf[i];	// normalisation a la WAV32
		Lspek.compute( raw32 );
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (1.0/32768.0) * (float)Rbuf[i];	// normalisation a la WAV32
		Rspek.compute( raw32 );
		}
	else	gasp("pas de spek pour fft");
	}
else	gasp("pas de channel pour fft");

double rawmax = 0;
for	( i = 0; i < wavp.wavsize; ++i)
	if	( fabs(raw32[i]) > rawmax )
		rawmax = fabs(raw32[i]);
printf("rawmax : %g\n", rawmax );

free(raw32);
// a ce point on a 1 ou 2 spectres de la wav entiere dans 1 ou 2 tableaux unsigned int Xspek->spectre
// de dimensions Xspek->H x Xspek->W
printf("end calcul %d spectres\n\n", qspek ); fflush(stdout);
}

printf("start colorisation spectrogrammes\n"); fflush(stdout);
// creer chaque pixbuf, pour le spectre entier
Lpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Lspek.W, Lspek.H );
// ici il faudrait peut etre tester que c'est Ok...
if	( qspek >= 2 )
	Rpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Rspek.W, Rspek.H );
// adapter la palette a la limite umax et l'applique a tous les spectres
palettize( (Lspek.umax>Rspek.umax)?(Lspek.umax):(Rspek.umax) );
printf("end colorisation spectrogrammes\n" ); fflush(stdout);
return 0;
}

// la partie du process en relation avec jluplot

// layout pour le domaine temporel - doit etre fait apres lecture wav data et calcul spectres
void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_s16_lod * curcour;

panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour les waves
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "mono";
curbande->optX = 1;
curbande->optretX = 1;

// creer un layer
curcour = new layer_s16_lod;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le layer pour le canal L ou mono
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->label = string("Left");
curcour->fgcolor.dR = 0.75;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;

// creer le layer pour le canal R si stereo
if	( wavp.chan > 1 )
	{
	panneau->bandes[0]->Ylabel = "stereo";

	// creer le layer
	curcour = new layer_s16_lod;	// wave a pas uniforme
	curbande->courbes.push_back( curcour );
	// parentizer a cause des fonctions "set"
	panneau->parentize();

	// configurer le layer
	curcour->set_km( 1.0 );
	curcour->set_m0( 0.0 );
	curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
	curcour->set_n0( 0.0 );
	curcour->label = string("Right");
	curcour->fgcolor.dR = 0.0;
	curcour->fgcolor.dG = 0.75;
	curcour->fgcolor.dB = 0.0;
	}

layer_rgb * curcour2;
/* creer le strip pour le spectro */
curbande = new strip;
panneau->bandes.push_back( curbande );
// creer le layer
curcour2 = new layer_rgb;
curbande->courbes.push_back( curcour2 );
// parentizer a cause des fonctions "set"
panneau->parentize();
// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 1.0;
curbande->bgcolor.dB = 1.0;
curbande->Ylabel = "midi";
curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
curbande->optretX = 0;
curbande->optretY = 0;
curbande->kmfn = 0.004;	// on reduit la marge de 5% qui est appliquee par defaut au fullN
// curbande->optcadre = 1;	// pour economiser le fill du fond
// configurer le layer
curcour2->set_km( 1.0 / (double)Lspek.fftstride );	// M est en samples, U en FFT-runs
curcour2->set_m0( 0.5 * (double)Lspek.fftsize );	// recentrage au milieu de chaque fenetre FFT
curcour2->set_kn( (double)Lspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
							// la midinote correspondant au bas du spectre
curcour2->set_n0( (double)Lspek.midi0 - 0.5/(double)Lspek.bpst ); // -recentrage de 0.5 bins

if	( qspek >= 2 )
	{
	panneau->bandes.back()->Ylabel = "midi L";

	/* creer le strip pour le spectro */
	curbande = new strip;
	panneau->bandes.push_back( curbande );
	// creer le layer
	curcour2 = new layer_rgb;
	curbande->courbes.push_back( curcour2 );
	// parentizer a cause des fonctions "set"
	panneau->parentize();
	// configurer le strip
	curbande->bgcolor.dR = 1.0;
	curbande->bgcolor.dG = 1.0;
	curbande->bgcolor.dB = 1.0;
	curbande->Ylabel = "midi R";
	curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
	curbande->optretX = 0;
	curbande->optretY = 0;
	curbande->kmfn = 0.004;	// on reduit la marge de 5% qui est appliquee par defaut au fullN
	// curbande->optcadre = 1;	// pour economiser le fill du fond
	// configurer le layer
	curcour2->set_km( 1.0 / (double)Rspek.fftstride );	// M est en samples, U en FFT-runs
	curcour2->set_m0( 0.5 * (double)Rspek.fftsize );	// recentrage au milieu de chaque fenetre FFT
	curcour2->set_kn( (double)Rspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
								// la midinote correspondant au bas du spectre
	curcour2->set_n0( (double)Rspek.midi0 - 0.5/(double)Rspek.bpst ); // -recentrage de 0.5 bins
	}
}

// connexion du layout aux data
int process::connect_layout( gpanel * panneau )
{
int retval;
// pointeurs locaux sur les 2 ou 3 layers
layer_s16_lod * layL, * layR = NULL;
layer_rgb * laySL, * laySR;
// connecter les layers de ce layout sur les buffers existants
layL = (layer_s16_lod *)panneau->bandes[0]->courbes[0];
layL->V = Lbuf;
layL->qu = wavp.wavsize;
if	( wavp.chan > 1 )
	{
	layR = (layer_s16_lod *)panneau->bandes[0]->courbes[1];
	layR->V = Rbuf;
	layR->qu = wavp.wavsize;
	}
retval = layL->make_lods( 4, 4, 800 );
if	( retval )
	{ printf("echec make_lods err %d\n", retval ); return -6;  }
if	( wavp.chan > 1 )
	{
	retval = layR->make_lods( 4, 4, 800 );
	if	( retval )
		{ printf("echec make_lods err %d\n", retval ); return -7;  }
	}
panneau->kq = (double)(wavp.freq);	// pour avoir une echelle en secondes au lieu de samples

unsigned int ib = 1;	//
if	( ib >= panneau->bandes.size() )
	gasp("erreur sur layout");
laySL = (layer_rgb *)panneau->bandes[ib]->courbes[0];
laySL->spectropix = Lpix;
// a ce point on a dans layS->spectropix un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
// petite verification
unsigned int verif = gdk_pixbuf_get_width(  ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
	    verif *= gdk_pixbuf_get_height( ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
printf("verif connexion spectrogramme %u pixels\n", verif );

if	( qspek >= 2 )
	{
	ib++;
	if	( ib >= panneau->bandes.size() )
		gasp("erreur sur layout");
	laySR = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySR->spectropix = Rpix;
	unsigned int verif = gdk_pixbuf_get_width(  ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
		    verif *= gdk_pixbuf_get_height( ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
	printf("verif connexion spectrogramme %u pixels\n", verif );
	}

printf("end layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
return 0;
}

// layout pour le domaine frequentiel
void process::prep_layout2( gpanel * panneau )
{
strip * curbande;
layer_u16 * curcour;

panneau->offscreen_flag = 0;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour les waves
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );

// configurer le strip
curbande->bgcolor.dR = 0.8;
curbande->bgcolor.dG = 0.8;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "spec";
curbande->optX = 1;
curbande->optretX = 1;

// creer un layer
curcour = new layer_u16;	//
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le layer pour le spectre
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->label = string("Lin");
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;
}

// connexion du layout aux data
int process::connect_layout2( gpanel * panneau, int pos )
{
// pointeurs locaux sur les layers
layer_u16 * layL = NULL;
// connecter les layers de ce layout sur les buffers existants
layL = (layer_u16 *)panneau->bandes[0]->courbes[0];
layL->V = Lspek.spectre + ???? Lspek.H * pos ???? ; // attentio la wav est plus large que qsamp * Lspek.H * fftstride
layL->qu = Lspek.H;
return 0;
}



void fill_palette_simple( unsigned char * pal, unsigned int iend )
{
unsigned char * palR = pal;
unsigned char * palG = palR + 65536;
unsigned char * palB = palG + 65536;
unsigned int mul = ( 1 << 24 ) / iend;
unsigned char val = 0;
for	( unsigned int i = 0; i < iend; ++i )
	{
	val = ( i * mul ) >> 16;
	palR[i] = val;
	palG[i] = val;
	palB[i] = 69;
	}
// completer la zone de saturation
memset( palR + iend, val, 65536 - iend );
memset( palG + iend, val, 65536 - iend );
memset( palB + iend, val, 65536 - iend );
}

// colorisation d'un pixbuf sur le spectre precalcule, utilisant la palette referencee dans spek
// i.e. remplir le pixbuf avec l'image RBG obtenue par palettisation du spectre en u16
void colorize( spectro * spek, GdkPixbuf * lepix )
{
int rowstride = gdk_pixbuf_get_rowstride( lepix );
unsigned char * RGBdata = gdk_pixbuf_get_pixels( lepix );
int colorchans = gdk_pixbuf_get_n_channels( lepix );
spek->spectre2rgb( RGBdata, rowstride, colorchans  );
}

// adapte la palette a la limite iend et l'applique a tous les spectres
void process::palettize( unsigned int iend )
{
// mettre a jour palette
fill_palette_simple( mutpal, iend );
// coloriser les spectre (qui referencent deja cette palette)
colorize( &Lspek, Lpix );
if	( qspek >= 2 )
	colorize( &Rspek, Rpix );
}

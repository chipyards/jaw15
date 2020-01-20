
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

// remplissage de la palette et colorisation d'un pixbuf sur le spectre precalcule
void process::colorize( spectro * spek, GdkPixbuf * lepix, unsigned int iend )
{
if	( ( spek->spectre == NULL ) || ( lepix == NULL ) )
	return;
// mettre a jour palette en fonction de la magnitude max
spek->fill_palette( iend );
// remplir le pixbuf avec l'image RBG obtenue par palettisation du spectre en u16
int rowstride = gdk_pixbuf_get_rowstride( lepix );
unsigned char * RGBdata = gdk_pixbuf_get_pixels( lepix );
int colorchans = gdk_pixbuf_get_n_channels( lepix );
spek->spectre2rgb( RGBdata, rowstride, colorchans  );
}

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
Lspek.fftsize = 16384;
Lspek.fftstride = Lspek.fftsize/4;
if	( qspek >= 2 )
	{
	Rspek.fftsize = Lspek.fftsize;
	Rspek.fftstride = Lspek.fftstride;
	}

// allocations pour spectro
if	( qspek >= 1 )
	{
	retval = Lspek.init( wavp.freq, wavp.wavsize );
	if	( retval )
		gasp("erreur init L spectro %d", retval );
	printf("end init L spectro\n\n"); fflush(stdout);
	if	( qspek >= 2 )
		{
		retval = Rspek.init( wavp.freq, wavp.wavsize );
		if	( retval )
			gasp("erreur init R spectro %d", retval );
		printf("end init R spectro\n\n"); fflush(stdout);
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
		raw32[i] = (1.0/32768.0) * (float)Lbuf[i];	// normalisation
		// raw32[i] = 2.0 * (float)wL->V[i];		// coeff de JAW04b
	Lspek.compute( raw32 );
	}
else if	( wavp.chan == 2 )
	{
	if	( qspek == 1 )	// spectre mono sur WAV stereo
		{
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (0.5/32768.0) * ((float)Lbuf[i] + (float)Rbuf[i]);	// normalisation
		Lspek.compute( raw32 );
		}
	else if	( qspek == 2 )
		{
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (1.0/32768.0) * (float)Lbuf[i];	// normalisation
		Lspek.compute( raw32 );
		for	( i = 0; i < wavp.wavsize; ++i)
			raw32[i] = (1.0/32768.0) * (float)Rbuf[i];	// normalisation
		Rspek.compute( raw32 );
		}
	else	gasp("pas de spek pour fft");
	}
else	gasp("pas de channel pour fft");
free(raw32);
// a ce point on a 1 ou 2 spectres de la wav entiere dans 1 ou 2 tableaux unsigned int Xspek->spectre
// de dimensions Xspek->H x Xspek->W
printf("end calcul %d spectres\n\n", qspek ); fflush(stdout);
}

printf("start creation pixbufs pour spectro\n"); fflush(stdout);
// creer le pixbuf, pour le spectre entier
Lpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Lspek.W, Lspek.H );
if	( qspek >= 2 )
	Rpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Rspek.W, Rspek.H );
printf("end creation pixbufs pour spectro\n"); fflush(stdout);

printf("start colorisation L spectrogramme\n"); fflush(stdout);
colorize( &Lspek, Lpix, Lspek.umax );
if	( qspek >= 2 )
	{
	printf("start colorisation R spectrogramme\n"); fflush(stdout);
	colorize( &Rspek, Rpix, Rspek.umax );
	}
printf("end colorisation spectrogramme\n" ); fflush(stdout);

return 0;
}

// la partie du process en relation avec jluplot

// layout pour les waveforms - doit etre fait apres lecture wav data et calcul spectres
void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_s16_lod * curcour;

panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour le canal L ou mono
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );
// creer le layer
curcour = new layer_s16_lod;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "mono";
curbande->optX = 1;
curbande->optretX = 1;
// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->label = string(wnam);
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.75;

// creer le strip pour le canal R si stereo
if	( wavp.chan > 1 )
	{
	panneau->bandes[0]->Ylabel = "left";
	panneau->bandes[0]->optX = 0;

	// creer le strip
	curbande = new strip;
	panneau->bandes.push_back( curbande );
	// creer le layer
	curcour = new layer_s16_lod;	// wave a pas uniforme
	curbande->courbes.push_back( curcour );
	// parentizer a cause des fonctions "set"
	panneau->parentize();

	// configurer le strip
	curbande->bgcolor.dR = 1.0;
	curbande->bgcolor.dG = 0.9;
	curbande->bgcolor.dB = 0.8;
	curbande->Ylabel = "right";
	curbande->optX = 1;
	curbande->optretX = 1;
	// configurer le layer
	curcour->set_km( 1.0 );
	curcour->set_m0( 0.0 );
	curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
	curcour->set_n0( 0.0 );
	curcour->fgcolor.dR = 0.0;
	curcour->fgcolor.dG = 0.0;
	curcour->fgcolor.dB = 0.75;
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
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "Hz";
curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
curbande->optretX = 0;
curbande->optretY = 0;
curbande->kmfn = 0.0;	// on enleve la marge de 5% qui est appliquee par defaut au fullN
// curbande->optcadre = 1;	// pour economiser le fill du fond
// configurer le layer
curcour2->set_km( 1.0 / (double)Lspek.fftstride );	// M est en samples, U en FFT-runs
curcour2->set_m0( 0.5 * (double)Lspek.fftsize );	// recentrage au milieu de chaque fenetre FFT
//curcour2->set_kn( (double)Lspek.bpst );		// N est en Hz, V est en bins de fsamp/fftsize
//curcour2->set_n0( (double)Lspek.midi0 - 0.5/(double)Lspek.bpst ); // -recentrage de 0.5 bins
curcour2->set_kn( double(Lspek.fftsize)/double(wavp.freq) );
curcour2->set_n0( -0.5 * double(wavp.freq)/double(Lspek.fftsize) );

if	( qspek >= 2 )
	{
	panneau->bandes.back()->Ylabel = "Hz L";

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
	curbande->bgcolor.dG = 0.9;
	curbande->bgcolor.dB = 0.8;
	curbande->Ylabel = "Hz R";
	curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
	curbande->optretX = 0;
	curbande->optretY = 0;
	curbande->kmfn = 0.0;	// on enleve la marge de 5% qui est appliquee par defaut au fullN
	// curbande->optcadre = 1;	// pour economiser le fill du fond
	// configurer le layer
	curcour2->set_km( 1.0 / (double)Lspek.fftstride );		// M est en samples, U en FFT-runs
	curcour2->set_m0( 0.5 * (double)Lspek.fftsize );		// recentrage au milieu de chaque fenetre FFT
	curcour2->set_kn( double(Lspek.fftsize)/double(wavp.freq) );
	curcour2->set_n0( -0.5 * double(wavp.freq)/double(Lspek.fftsize) );
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
	layR = (layer_s16_lod *)panneau->bandes[1]->courbes[0];
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

unsigned int ib = wavp.chan;	// on suppose que c'est bien le nombre de wav strips, le prochain c'est le 1er spectro
if	( ib >= panneau->bandes.size() )
	gasp("erreur sur layout");
laySL = (layer_rgb *)panneau->bandes[ib]->courbes[0];
laySL->spectropix = Lpix;
// a ce point on a dans layS->spectropix un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
// petite verification
unsigned int verif = gdk_pixbuf_get_width(  ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
	    verif *= gdk_pixbuf_get_height( ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
printf("verif colorisation spectrogramme %u pixels\n", verif );

if	( qspek >= 2 )
	{
	ib++;
	if	( ib >= panneau->bandes.size() )
		gasp("erreur sur layout");
	laySR = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySR->spectropix = Rpix;
	unsigned int verif = gdk_pixbuf_get_width(  ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
		    verif *= gdk_pixbuf_get_height( ((layer_rgb *)panneau->bandes[ib]->courbes[0])->spectropix );
	printf("verif colorisation spectrogramme %u pixels\n", verif );
	}

printf("end layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
return 0;
}

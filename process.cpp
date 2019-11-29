
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

/** ============================ AUDIO data processing =============== */

// layout pour les waveforms - doit etre fait avant lecture wav data mais apres lecture wav header
// car wave_read_full() reference les buffers V[] dans les layers qui doivent exister
void prep_layout1( gpanel * panneau, int ntracks, const char * fnam )
{
strip * curbande;
layer_s16_lod * curcour;

panneau->offscreen_flag = 1;	// 1 par defaut

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
// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->label = string(fnam);
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.75;

// creer le strip pour le canal R si stereo
if	( ntracks > 1 )
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
	// configurer le layer
	curcour->set_km( 1.0 );
	curcour->set_m0( 0.0 );
	curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
	curcour->set_n0( 0.0 );
	curcour->fgcolor.dR = 0.0;
	curcour->fgcolor.dG = 0.0;
	curcour->fgcolor.dB = 0.75;
	}
// marge pour les textes
panneau->mx = 60;
}

// layout pour le spectro - doit etre fait apres init du spectro
void prep_layout2( gpanel * panneau, spectro * spek )
{
strip * curbande;
layer_rgb * curcour;
/* creer le strip pour le spectro */
curbande = new strip;
panneau->bandes.push_back( curbande );
// creer le layer
curcour = new layer_rgb;
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();
// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "spectro";
curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
curbande->kmfn = 0.0;	// on enleve la marge de 5% qui est appliquee par defaut au fullN
// curbande->optcadre = 1;	// pour economiser le fill du fond
// configurer le layer
curcour->set_km( 1.0 / (double)spek->fftstride );	// M est en samples, U en FFT-runs
curcour->set_m0( 0.5 * (double)spek->fftsize );		// recentrage au milieu de chaque fenetre FFT
curcour->set_kn( (double)spek->bpst );			// N est en MIDI-note (demi-tons), V est en bins
curcour->set_n0( (double)spek->midi0 );			// la midinote correspondant au bas du spectre
}

// lecture WAV 16 bits entier en memoire, lui fournir wavpars vide
// donnees stockées dans un ou deux (stereo) layer_s16
// appelle prep_layout1 pour lui passer le nombre de canaux
// alloue memoire pour WAV, rend NULL pour tout echec
// puis fait le spectrogramme
// appelle prep_layout2
int wave_process_full( const char * fnam, wavpars * s, gpanel * panneau, spectro * spek )
{
// pointeurs locaux sur les 2 ou 3 layers
layer_s16_lod * layL, * layR = NULL;
layer_rgb * layS;

printf("ouverture %s en lecture\n", fnam );
s->hand = open( fnam, O_RDONLY | O_BINARY );
if	( s->hand == -1 )
	{ printf("file not found %s\n", fnam ); return -1;  }

WAVreadHeader( s );
if	(
	( ( s->chan != 1 ) && ( s->chan != 2 ) ) ||
	( s->resol != 16 )
	)
	{ printf("programme seulement pour fichiers 16 bits mono ou stereo\n"); close(s->hand); return -2;  }
printf("freq = %u, block = %u, bpsec = %u, size = %u\n",
	s->freq, s->block, s->bpsec, s->wavsize );

prep_layout1( panneau, s->chan, fnam );


layL = (layer_s16_lod *)panneau->bandes[0]->courbes[0];
layL->allocV( s->wavsize );	// pour 1 canal
if	( layL->V == NULL )
	{ printf("echec allocV %d samples\n", (int)s->wavsize ); close(s->hand); return -3;  }

if	( s->chan > 1 )
	{
	layR = (layer_s16_lod *)panneau->bandes[1]->courbes[0];
	layR->allocV( s->wavsize );	// pour 1 canal
	if	( layR->V == NULL )
		{ printf("echec allocV %d samples\n", (int)s->wavsize ); close(s->hand); return -3;  }
	}

// lecture bufferisee
unsigned int rdbytes, remsamples, rdsamples, rdsamples1c, totalsamples1c = 0;	// "1c" veut dire "pour 1 canal"
#define QRAW 4096
#define Qs16 sizeof(short int)
short int rawsamples16[QRAW];	// buffer pour read()
unsigned int i, j;
j = 0;
while	( totalsamples1c < s->wavsize )
	{
	remsamples = ( s->wavsize - totalsamples1c ) * s->chan;
	if	( remsamples > QRAW )
		remsamples = QRAW;
	rdbytes = read( s->hand, rawsamples16, remsamples*Qs16 );
	rdsamples = rdbytes / Qs16;
	if	( rdsamples != remsamples )
		{ printf("truncated WAV data"); close(s->hand); return -4;  }
	rdsamples1c = rdsamples / s->chan;
	totalsamples1c += rdsamples1c;
	if	( s->chan == 2 )
		{
		for	( i = 0; i < rdsamples; i += 2)
			{
			layL->V[j]   = rawsamples16[i];
			layR->V[j++] = rawsamples16[i+1];
			}
		}
	else if	( s->chan == 1 )
		{
		for	( i = 0; i < rdsamples; ++i )
			layL->V[j++] = rawsamples16[i];
		}
	}
if	( totalsamples1c != s->wavsize )	// cela ne peut pas arriver, cette verif serait parano
	{ printf("WAV size error %u vs %d", totalsamples1c , (int)s->wavsize ); close(s->hand); return -5;  }
close( s->hand );
printf("lu %d samples Ok\n", totalsamples1c );

int retval = layL->make_lods( 4, 4, 800 );
if	( retval )
	{ printf("echec make_lods err %d\n", retval ); return -6;  }
if	( s->chan > 1 )
	{
	retval = layR->make_lods( 4, 4, 800 );
	if	( retval )
		{ printf("echec make_lods err %d\n", retval ); return -7;  }
	}
panneau->kq = (double)(s->freq);	// pour avoir une echelle en secondes au lieu de samples
fflush(stdout);

// creation spectrographe
printf("\nstart init spectro\n"); fflush(stdout);

// parametres primitifs
spek->bpst = 10;		// binxel-per-semi-tone : resolution spectro log
spek->octaves = 7;		// 7 octaves
spek->fftsize = 8192;
spek->fftstride = 1024;
spek->midi0 = 28;		// E1 = mi grave de la basse


// allocations
retval = spek->init( s->freq, s->wavsize );
if	( retval )
	gasp("erreur init spectro %d", retval );
printf("end init spectro\n\n"); fflush(stdout);

// calcul spectre : hum, il faut les data en float mais on a lu la wav en s16
// provisoirement on va convertir en float l'audio qu'on a deja en RAM
// de plus on fait seulement du mono
{
printf("start calcul spectre\n"); fflush(stdout);
float * raw32 = (float *) malloc( s->wavsize * sizeof(float) );
if	( raw32 == NULL )
	gasp("echec alloc %d floats pour source fft", s->wavsize );
printf("Ok alloc %d floats pour source fft\n", s->wavsize );
unsigned int i;
if	( ( s->chan == 1 ) && ( panneau->bandes.size() >= 1 ) )
	{
	for	( i = 0; i < s->wavsize; ++i )
		raw32[i] = (1.0/32768.0) * (float)layL->V[i];	// normalisation
		// raw32[i] = 2.0 * (float)wL->V[i];		// coeff de JAW04b
	}
else if	( ( s->chan == 2 ) && ( panneau->bandes.size() >= 2 ) )
	{
	for	( i = 0; i < s->wavsize; ++i)
		raw32[i] = (0.5/32768.0) * ((float)layL->V[i] + (float)layR->V[i]);	// normalisation
		// raw32[i] = ((float)wL->V[i] + (float)wR->V[i]);		// coeff de JAW04b
	}
else	gasp("echec preparation float32 pour fft");
spek->compute( raw32 );
// a ce point on a un spectre de la wav entiere dans un tableau unsigned int glo->spec.spectre
// de dimensions spek->H x spek->W
free(raw32);
printf("end calcul spectre\n\n"); fflush(stdout);
}

printf("start colorisation spectrogramme\n"); fflush(stdout);
// mettre a jour palette en fonction de la magnitude max
spek->fill_palette( spek->umax );

prep_layout2( panneau, spek );
layS = (layer_rgb *)panneau->bandes.back()->courbes[0];

// creer le pixbuf, pour le spectre entier
{
GdkPixbuf * lepix;
lepix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, spek->W, spek->H );
// remplir le pixbuf avec l'image RBG obtenue par palettisation du spectre en u16

int rowstride = gdk_pixbuf_get_rowstride( lepix );
unsigned char * RGBdata = gdk_pixbuf_get_pixels( lepix );
int colorchans = gdk_pixbuf_get_n_channels( lepix );
spek->spectre2rgb( RGBdata, rowstride, colorchans  );
layS->spectropix = lepix;
}
// a ce point on a dans layS->spectropix un pixbuf RGB de la wav entiere, de dimensions spek->H x spek->W
// petite verification
i = panneau->bandes.size() - 1;
unsigned int verif = gdk_pixbuf_get_width( ((layer_rgb *)panneau->bandes[i]->courbes[0])->spectropix );
	    verif *= gdk_pixbuf_get_height( ((layer_rgb *)panneau->bandes[i]->courbes[0])->spectropix );
printf("end colorisation spectrogramme %u pixels\n\n", verif ); fflush(stdout);

return 0;
}

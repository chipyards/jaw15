
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <mpg123.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "JLUP/jluplot.h"
#include "JLUP/gluplot.h"
#include "JLUP/layer_lod.h"
#include "JLUP/layer_rgb.h"
#include "JLUP/layer_u.h"

#include "autobuf.h"
#include "wavio.h"
#include "mp3in.h"
#include "fftw3.h"
#include "spectro.h"
#include "process.h"

/** ============================ AUDIO data processing =============== */

#define QMORE	(1<<21)	// quantum pour reallocation ( 1 page = 4M = (1<<21)shorts )

// allocation memoire et lecture WAV ou MP3 16 bits entier en memoire
// donnees stockées dans les buffers de l'objet process
int process::audiofile_process()
{
int retval;
int mp3flag = 0;

retval = strlen( wnam );
mp3flag = ( wnam[retval-1] == '3' );

printf("ouverture %s %s en lecture\n", (mp3flag?"MP3":"WAV"), wnam ); fflush(stdout);

// 1ere etape : lire un premiere bloc pour avoir les parametres
if	( mp3flag )
	{
	af = (audiofile *)&m3;
	retval = m3.read_head( wnam );
	if	( retval )
		{
		printf("error read_head: %s : %s\n", m3.errfunc, mpg123_plain_strerror(retval) );
		fflush(stdout); return -1;
		}
	printf("recommended buffer %d bytes\n", (int)m3.outblock );
	}
else	{
	af = (audiofile *)&wavp;
	retval = wavp.read_head( wnam );
	if	( retval )
		{
		printf("file not found %s\n", wnam );
		fflush(stdout); return -1;
		}
	}

printf("got %d channels @ %d Hz, monosamplesize %d\n",
	af->qchan, af->fsamp, af->monosamplesize );
printf("estimated length : %u PCM frames\n", (unsigned int)af->estpfr );
fflush(stdout);
if	(
	( ( af->qchan != 1 ) && ( af->qchan != 2 ) ) ||
	( af->monosamplesize != 2 )
	)
	{
	printf("programme seulement pour fichiers 16 bits mono ou stereo\n");
	af->afclose();
	return -2;
	}
fflush(stdout);

// 2eme etape : allouer de la memoire pour les buffers

// un petit buffer pour l'audio entrelace
#define QPFR 1152	// optimal pour mp3, en PCM frames
#define QRAW 2048	// optimal pour WAV, en PCM frames

short pcmbuf[QRAW*2];	// supporte stereo, supporte mp3 si QRAW > QPFR


/* pre-allocation (facultative) des buffers pour l'audio entier a servir a jluplot */
if	( Lbuf.more( af->estpfr ) )
	gasp("echec malloc Lbuf %d samples", (int)af->estpfr );

if	( af->qchan > 1 )
	{
	if	( Rbuf.more( af->estpfr ) )
		gasp("echec malloc Rbuf %d samples", (int)af->estpfr );
	}
//*/

// 3eme etape : boucler sur le buffer
int i;
unsigned int j = 0;
unsigned int qpfr = (mp3flag?QPFR:QRAW);
if	( af->qchan == 2 )
	{			// boucle stereo
	do	{
		retval = af->read_data_p( (void *)pcmbuf, qpfr );	// dynamic binding!
		if	( retval > 0 )
			{
			// printf("%u vs %u\n", m3.realpfr, Lbuf.capa ); fflush(stdout);
			if	( af->realpfr > Lbuf.capa )
				{
				if	( Lbuf.more( QMORE ) )
					gasp("echec autobuf::more()");
				printf("realloc Lbuf\n"); fflush(stdout);
				}
			if	( af->realpfr > Rbuf.capa )
				{
				if	( Rbuf.more( QMORE ) )
					gasp("echec autobuf::more()");
				}
			for	( i = 0; i < ( retval * 2 ); i += 2 )
				{
				Lbuf.data[j]   = pcmbuf[i];
				Rbuf.data[j++] = pcmbuf[i+1];
				}
			}
		} while ( retval == (int)qpfr );
	Lbuf.size = Rbuf.size = af->realpfr;
	}
	{			// boucle mono
	do	{
		retval = af->read_data_p( (void *)pcmbuf, qpfr );
		if	( retval > 0 )
			{
			if	( af->realpfr > Lbuf.capa )
				{
				if	( Lbuf.more( QMORE ) )
					gasp("echec autobuf::more()");
				printf("realloc Lbuf\n"); fflush(stdout);
				}
			for	( i = 0; i < retval; i++ )
				{
				Lbuf.data[j++]  = pcmbuf[i];
				}
			}
		} while ( retval == (int)qpfr );
	Lbuf.size = af->realpfr;
	}
if	( j != af->realpfr )	// cela ne peut pas arriver, cette verif est parano
	gasp("erreur JAW #213978");

if	( ( mp3flag ) && ( retval < 0 ) )
	{
	printf("error mp3 read_data: %s\n", m3.errfunc );
	}

printf("decoded PCM frames %u vs %u estimated\n", (unsigned int)af->realpfr, (unsigned int)af->estpfr );

af->afclose();

fflush(stdout); fflush(stderr);

return 0;
}

// calculs spectrogramme dans l'objet process
int process::spectrum_compute( int force_mono )
{
int retval;

// creation spectrographe
if	( force_mono )
	qspek = 1;
else	qspek = af->qchan;

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
	retval = Lspek.init( af->fsamp, Lbuf.size );
	if	( retval )
		gasp("erreur init L spectro %d", retval );
	printf("end init L spectro, window type %d, avg %g\n\n", Lspek.window_type, Lspek.window_avg ); fflush(stdout);
	if	( qspek >= 2 )
		{
		retval = Rspek.init( af->fsamp, Rbuf.size );
		if	( retval )
			gasp("erreur init R spectro %d", retval );
		printf("end init R spectro, window type %d, avg %g\n\n", Rspek.window_type, Rspek.window_avg ); fflush(stdout);
		}
	}
else	gasp("no spek to init");

// calcul spectre : hum, il faut les data en float mais on a lu la wav en s16
// provisoirement on va convertir en float l'audio qu'on a deja en RAM

printf("start calcul spectre\n"); fflush(stdout);
float * raw32 = (float *) malloc( Lbuf.size * sizeof(float) );
if	( raw32 == NULL )
	gasp("echec alloc %d floats pour source fft", Lbuf.size );
printf("Ok alloc %d floats pour source fft\n", Lbuf.size );
unsigned int i;
if	( af->qchan == 1 )
	{
	for	( i = 0; i < Lbuf.size; ++i )
		raw32[i] = (float)Lbuf.data[i];
	Lspek.wav_peak = 32767.0;
	Lspek.compute( raw32 );
	}
else if	( af->qchan == 2 )
	{
	if	( qspek == 1 )	// spectre mono sur WAV stereo
		{
		for	( i = 0; i < Lbuf.size; ++i)
			raw32[i] = (float)Lbuf.data[i] + (float)Rbuf.data[i];
		Lspek.wav_peak = 65534.0;
		Lspek.compute( raw32 );
		}
	else if	( qspek == 2 )
		{
		for	( i = 0; i < Lbuf.size; ++i)
			raw32[i] = (float)Lbuf.data[i];
		Lspek.wav_peak = 32767.0;
		Lspek.compute( raw32 );
		for	( i = 0; i < Rbuf.size; ++i)
			raw32[i] = (float)Rbuf.data[i];
		Rspek.wav_peak = 32767.0;
		Rspek.compute( raw32 );
		}
	else	gasp("pas de spek pour fft");
	}
else	gasp("pas de channel pour fft");

free(raw32);
// a ce point on a 1 ou 2 spectres de la wav entiere dans 1 ou 2 tableaux unsigned int Xspek->spectre
// de dimensions Xspek->H x Xspek->W
printf("end calcul %d spectres\n\n", qspek ); fflush(stdout);


// printf("start colorisation spectrogrammes\n"); fflush(stdout);
// adapter la palette a la limite umax et l'applique a tous les spectres
// (umax a ete  calcule par spectro::compute)
palettize( (Lspek.umax>Rspek.umax)?(Lspek.umax):(Rspek.umax) );
// printf("end colorisation spectrogrammes\n" ); fflush(stdout);
return 0;
}

// la partie du process en relation avec jluplot

// layout pour pour wav data (depend de af->qchan)
void process::prep_layout_W( gpanel * panneau )
{
gstrip * curbande;
layer_lod<short> * curcour;

panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour les waves
curbande = new gstrip;
panneau->add_strip( curbande );

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "mono";
curbande->optX = 1;
curbande->optretX = 1;
gpanel::smenu_set_title( curbande->smenu_y, "SIGNAL AXIS" );

// creer un layer
curcour = new layer_lod<short>;	// wave a pas uniforme
curbande->add_layer( curcour, "Left" );

// configurer le layer pour le canal L ou mono
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->fgcolor.dR = 0.75;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;

// creer le layer pour le canal R si stereo
if	( af->qchan > 1 )
	{
	panneau->bandes[0]->Ylabel = "stereo";

	// creer le layer
	curcour = new layer_lod<short>;	// wave a pas uniforme
	curbande->add_layer( curcour, "Right" );

	// configurer le layer
	curcour->set_km( 1.0 );
	curcour->set_m0( 0.0 );
	curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
	curcour->set_n0( 0.0 );
	curcour->fgcolor.dR = 0.0;
	curcour->fgcolor.dG = 0.75;
	curcour->fgcolor.dB = 0.0;
	}
}

// layout pour pour spectrograms (depend de qspek)
void process::prep_layout_S( gpanel * panneau )
{
gstrip * curbande;
if	( qspek >= 1 )
	{
	layer_rgb * curcour2;
	/* creer le strip pour le spectro */
	curbande = new gstrip;
	panneau->add_strip( curbande );
	// configurer le strip
	curbande->bgcolor.dR = 1.0;
	curbande->bgcolor.dG = 1.0;
	curbande->bgcolor.dB = 1.0;
	curbande->Ylabel = "midi";
	curbande->optX = 1;
	if	( panneau->bandes.size() > 1 )
		panneau->bandes[0]->optX = 0;
	curbande->optretX = 0;
	curbande->optretY = 0;
	curbande->kmfn = 0.004;	// on reduit la marge de 5% qui est appliquee par defaut au fullN
	gpanel::smenu_set_title( curbande->smenu_y, "MELODIC RANGE" );
	// curbande->optcadre = 1;	// pour economiser le fill du fond

	// creer le layer
	curcour2 = new layer_rgb;
	curbande->add_layer( curcour2, "RGB left" );

	// configurer le layer
	curcour2->set_km( 1.0 / (double)Lspek.fftstride );	// M est en samples, U en FFT-runs
	curcour2->set_m0( 0.5 * (double)(Lspek.fftsize-Lspek.fftstride ) );
	curcour2->set_kn( (double)Lspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
								// la midinote correspondant au bas du spectre
	curcour2->set_n0( (double)Lspek.midi0 - 0.5/(double)Lspek.bpst ); // -recentrage de 0.5 bins
	}
if	( qspek >= 2 )
	{
	panneau->bandes.back()->Ylabel = "midi L";
	layer_rgb * curcour2;
	/* creer le strip pour le spectro */
	curbande = new gstrip;
	panneau->add_strip( curbande );
	// configurer le strip
	curbande->bgcolor.dR = 1.0;
	curbande->bgcolor.dG = 1.0;
	curbande->bgcolor.dB = 1.0;
	curbande->Ylabel = "midi R";
	curbande->optX = 0;	// l'axe X reste entre les waves et le spectro, pourquoi pas ?
	curbande->optretX = 0;
	curbande->optretY = 0;
	curbande->kmfn = 0.004;	// on reduit la marge de 5% qui est appliquee par defaut au fullN
	gpanel::smenu_set_title( curbande->smenu_y, "MELODIC RANGE" );
	// curbande->optcadre = 1;	// pour economiser le fill du fond

	// creer le layer
	curcour2 = new layer_rgb;
	curbande->add_layer( curcour2, "RGB right" );

	// configurer le layer
	curcour2->set_km( 1.0 / (double)Rspek.fftstride );	// M est en samples, U en FFT-runs
	curcour2->set_m0( 0.5 * (double)(Rspek.fftsize-Rspek.fftstride) );
	curcour2->set_kn( (double)Rspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
								// la midinote correspondant au bas du spectre
	curcour2->set_n0( (double)Rspek.midi0 - 0.5/(double)Rspek.bpst ); // -recentrage de 0.5 bins
	}
}

// connexion du layout aux data type wave
int process::connect_layout_W( gpanel * panneau )
{
int retval;
// pointeurs locaux sur les layers
layer_lod<short> * layL, * layR = NULL;
// connecter les layers de ce layout sur les buffers existants
layL = (layer_lod<short> *)panneau->bandes[0]->courbes[0];
layL->V = Lbuf.data;
layL->qu = Lbuf.size;
if	( af->qchan > 1 )
	{
	layR = (layer_lod<short> *)panneau->bandes[0]->courbes[1];
	layR->V = Rbuf.data;
	layR->qu = Rbuf.size;
	}
retval = layL->make_lods( 4, 4, 2000 );
if	( retval )
	{ printf("echec make_lods err %d\n", retval ); return -6;  }
if	( af->qchan > 1 )
	{
	retval = layR->make_lods( 4, 4, 2000 );
	if	( retval )
		{ printf("echec make_lods err %d\n", retval ); return -7;  }
	}
panneau->kq = (double)(af->fsamp);	// pour avoir une echelle en secondes au lieu de samples
printf("end layout W, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
return 0;
}

// connexion du layout aux data type spek
int process::connect_layout_S( gpanel * panneau )
{
unsigned int ib = 1;
layer_rgb * laySL, * laySR;

if	( qspek >= 1 )
	{
	if	( ib >= panneau->bandes.size() )
		gasp("erreur sur layout");
	laySL = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySL->spectropix = Lpix;	// on a la un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
	}
if	( qspek >= 2 )
	{
	ib++;
	if	( ib >= panneau->bandes.size() )
		gasp("erreur sur layout");
	laySR = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySR->spectropix = Rpix;	// on a la un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
	}

printf("end layout S, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
return 0;
}

// layout pour le domaine frequentiel (spectre ponctuel)
void process::prep_layout2( gpanel * panneau )
{
gstrip * curbande;
layer_u<unsigned short> * curcour;

panneau->offscreen_flag = 0;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour les spectres
curbande = new gstrip;
panneau->add_strip( curbande );

// configurer le strip
curbande->bgcolor.dR = 0.8;
curbande->bgcolor.dG = 0.8;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "spec";
curbande->optX = 1;
curbande->optretX = 1;
// gpanel::smenu_set_title( curbande->smenu_y, "MAG ?" );

// creer un layer
curcour = new layer_u<unsigned short>;	//
curbande->add_layer( curcour, "Lin" );

// configurer le layer pour le spectre
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;
}

// connexion du layout aux data
int process::connect_layout2( gpanel * panneau, int pos )
{
// pointeurs locaux sur les layers
layer_u<unsigned short> * layL;
// connecter les layers de ce layout sur les buffers existants
layL = (layer_u<unsigned short> *)panneau->bandes[0]->courbes[0];

int m0 = (Lspek.fftsize-Lspek.fftstride )/2;
int ibin = ( pos - m0 ) / Lspek.fftstride; 	// division entiere (ou floor)
if	( ibin < 0 ) ibin = 0;
if	( ibin > (int)( Lspek.W - 1 ) ) ibin = Lspek.W - 1;	// bornage sur [0,W-1]

layL->V = Lspek.spectre + ( ibin * Lspek.H ); 

// H est le nombre de "bins" apres passage en echelle log
layL->qu = Lspek.H;
layL->scan();
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
// c'est un wrapper sur spectro::spectre2rgb
void spectre2rgb( spectro * spek, GdkPixbuf * lepix )
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
// creer les pixbufs si necessaire, chacun pour le spectre entier
if	( Lpix == NULL )
	Lpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Lspek.W, Lspek.H );
if	( qspek >= 2 )
	{
	if	( Rpix == NULL )
	Rpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Rspek.W, Rspek.H );
	}
// coloriser les spectre (qui sont supposes deja referencer cette palette)
spectre2rgb( &Lspek, Lpix );
if	( qspek >= 2 )
	spectre2rgb( &Rspek, Rpix );
}

// liberer la memoire des spectros et pixbufs
void process::clean_spectros()
{
if	( Lspek.allocatedWH )
	{ Lspek.specfree( 0 ); printf("Lspek freed\n"); fflush(stdout); }
if	( Rspek.allocatedWH )
	{ Rspek.specfree( 0 ); printf("Rspek freed\n"); fflush(stdout); }
if	( Lpix )	// since gdk_pixbuf_unref() is deprecated
	{ g_object_unref( Lpix ); printf("Lpix freed\n"); fflush(stdout); }
if	( Rpix )
	{ g_object_unref( Rpix ); printf("Rpix freed\n"); fflush(stdout); }	
}

// deconnecter et defaire le layout S
void process::unlay_S( gpanel * panneau )
{
unsigned int ib, ic;	// on saute le premier strip reserve aux WAV
for	( ib = 1; ib < panneau->bandes.size(); ++ib )
	{
	for	( ic = 0; ic < panneau->bandes[ib]->courbes.size(); ++ic )
		delete panneau->bandes[ib]->courbes[ic];
	delete panneau->bandes[ib];
	}
panneau->bandes.resize(1);
}

// deconnecter et defaire le layout W (seulement si le layout S est deja defait !)
void process::unlay_W( gpanel * panneau )
{
unsigned int ic;
if	( panneau->bandes.size() == 1 )
	{
	for	( ic = 0; ic < panneau->bandes[0]->courbes.size(); ++ic )
		delete panneau->bandes[0]->courbes[ic];
	delete panneau->bandes[0];
	panneau->bandes.clear();
	}
}

// deconnecter et defaire le layout 2
void process::unlay_2( gpanel * panneau )
{
unsigned int ib, ic;
for	( ib = 0; ib < panneau->bandes.size(); ++ib )
	{
	for	( ic = 0; ic < panneau->bandes[ib]->courbes.size(); ++ic )
		delete panneau->bandes[ib]->courbes[ic];
	delete panneau->bandes[ib];
	}
panneau->bandes.clear();
}


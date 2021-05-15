
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
#include "JLUP/strip_x_midi.h"

#include "autobuf.h"
#include "wavio.h"
#include "mp3in.h"
#include "MIDI/midirender.h"
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
int mp3flag = 0, midiflag = 0;

retval = strlen( wnam );
if	( retval < 4 )
	return -1;
mp3flag  = ( wnam[retval-1] == '3' );
midiflag = ( wnam[retval-2] == 'i' );

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
else if	( midiflag )
	{
	sf2file = "F:\\STUDIO\\MIDI\\SF2\\GM\\GeneralUser1.471.sf2";
	af = (audiofile *)&mid;
	mid.flusyn.sf2file = sf2file;
	retval = mid.read_head( wnam );
	if	( retval )
		{
		printf("error midi read_head: %d\n", retval );
		fflush(stdout); return -1;
		}
	mid.flusyn.set_gain( 1.0 );
	}
else	{
	af = (audiofile *)&wavp;
	retval = wavp.read_head( wnam );
	if	( retval )
		{
		if	( retval == -1 )
			printf("file not found %s\n", wnam );
		else	printf("erreur wavio::read_head %d\n", retval );
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
if	( af->estpfr > Lbuf.capa )
	{
	Lbuf.reset();	// pour eviter realloc, qui serait inefficace ici
	if	( Lbuf.more( af->estpfr ) )
		gasp("echec alloc Lbuf %d samples", (int)af->estpfr );
	}
if	( ( af->qchan > 1 ) && ( af->estpfr > Rbuf.capa ) )
	{
	Rbuf.reset();
	if	( Rbuf.more( af->estpfr ) )
		gasp("echec alloc Rbuf %d samples", (int)af->estpfr );
	}
//*/

// 3eme etape : boucler sur le buffer
int i;
unsigned int j = 0; af->realpfr = 0;
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
		} while ( retval > 0 );
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
		} while ( retval > 0 );
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
int process::spectrum_compute2D( int force_mono, int opt_lin )
{
int retval, qspek;

// creation spectrographe
if	( force_mono )
	qspek = 1;
else	qspek = af->qchan;

if	( opt_lin )
	{ qspek = 2; force_mono = 1; } 

if	( ( Lspek.spectre2D ) || ( ( Rspek.spectre2D ) && ( qspek > 1 ) ) )
	{
	printf("\nspectrum_compute2D aborted (not freed)\n"); fflush(stdout);
	return 0;
	}

printf("\nstart init %d spectro\n", qspek ); fflush(stdout);
// N.B. les parametres en commentaire sont supposes avoir ete injectes avant l'appel
// -- parametres FFT
// Lspek.fftsize2D = 8192;
// Lspek.fftstride = 1024;
// Lspek.window_type = 1;
// Lspek.qthread;
// -- parametres conversion LOG
// Lspek.bpst = 9;		// binxel-per-semi-tone : resolution spectro log
Lspek.octaves = 6;		// 7 octaves
Lspek.midi0 = 28;		// E1 = mi grave de la basse
Lspek.pal = mutpal;		// palette commune
if	( qspek >= 2 )
	{
	// Rspek.fftsize2D = 8192;
	// Rspek.fftstride = 1024;
	// Rspek.window_type = 1;
	// Rspek.qthread = 1;
	// Rspek.bpst = 9;
	Rspek.octaves = 6;
	Rspek.midi0 = 28;
	Rspek.pal = mutpal;
	if	( opt_lin )
		Rspek.disable_log = 1;
	}

// init2D
retval = Lspek.init2D( af->fsamp, Lbuf.size );
if	( retval )
	gasp("erreur init2D L spectro %d", retval );
printf("ready L spek, FFT %u/%u, window type %d, avg %g\n\n", Lspek.fftsize2D, Lspek.fftstride, Lspek.window_type, Lspek.window_avg ); fflush(stdout);
if	( qspek >= 2 )
	{
	retval = Rspek.init2D( af->fsamp, ( opt_lin ? Lbuf.size : Rbuf.size ) );
	if	( retval )
		gasp("erreur init2D R spectro %d", retval );
	printf("ready R spek\n"); fflush(stdout);
	}

// calcul spectre2D
printf("start calcul spectre2D sur %d threads\n", Lspek.qthread ); fflush(stdout);
if	( af->qchan == 1 )
	{
	Lspek.wav_peak = 32767.0;		// spectre2D mono sur WAV stereo
	Lspek.compute2D( Lbuf.data );
	if	( opt_lin )
		{				// un second spectre2D sur Lbuf, mais en lin
		Rspek.wav_peak = 32767.0;
		Rspek.compute2D( Lbuf.data );
		}
	}
else if	( af->qchan == 2 )
	{
	if	( qspek == 1 )			// spectre2D mono sur WAV stereo
		{
		Lspek.wav_peak = 65534.0;
		Lspek.compute2D( Lbuf.data, Rbuf.data );
		}
	else if	( qspek == 2 )
		{
		if	( opt_lin )
			{			// deux spectre2D mono sur WAV stereo, dont 1 lin
			Lspek.wav_peak = 65534.0;
			Lspek.compute2D( Lbuf.data, Rbuf.data );
			Rspek.wav_peak = 65534.0;
			Rspek.compute2D( Lbuf.data, Rbuf.data );
			}
		else	{			// spectre2D stereo sur WAV stereo
			Lspek.wav_peak = 32767.0;
			Lspek.compute2D( Lbuf.data );
			Rspek.wav_peak = 32767.0;
			Rspek.compute2D( Rbuf.data );
			}
		}
	else	gasp("pas de spek pour fft");
	}
else	gasp("pas de channel pour fft");

// a ce point on a 1 ou 2 spectres de la wav entiere dans 1 ou 2 tableaux unsigned int Xspek->spectre2D
// de dimensions Xspek->H x Xspek->W
printf("end calcul %d spectres\n\n", qspek ); fflush(stdout);


// printf("start colorisation spectrogrammes\n"); fflush(stdout);
// adapter la palette a la limite umax et l'applique a tous les spectres
// (umax a ete  calcule par spectro::compute2D)
if	( qspek == 2 )
	palettize( (Lspek.umax>Rspek.umax)?(Lspek.umax):(Rspek.umax) );
else	palettize( Lspek.umax );
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


// layout pour spectrogrammes
void process::auto_layout_S( gpanel * panneau, int opt_lin )
{
if	( Lspek.spectre2D == NULL )
	return;
//// partie non repetable
if	( panneau->bandes.size() == 1 )
	{
	gstrip * curbande;
	layer_rgb * curcour;
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
	curcour = new layer_rgb;
	curbande->add_layer( curcour, "RGB left" );
	if	( Rspek.spectre2D )
		{
		panneau->bandes.back()->Ylabel = "midi L";
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
		curcour = new layer_rgb;
		curbande->add_layer( curcour, "RGB right" );
		}
	panneau->full_valid = 0;
	printf("end hard layout S, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
	}	// fin de la partie non-repetable
//// partie repetable a chaud, compatible avec un changement des params FFT
unsigned int ib = 1;
layer_rgb * laySL, * laySR;
if	( ib < panneau->bandes.size() )
	{
	laySL = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySL->set_km( 1.0 / (double)Lspek.fftstride );	// M est en samples, U en FFT-runs
	laySL->set_m0( 0.5 * (double)(Lspek.fftsize2D-Lspek.fftstride ) );
	laySL->set_kn( (double)Lspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
								// la midinote correspondant au bas du spectre2D
	laySL->set_n0( (double)Lspek.midi0 - 0.5/(double)Lspek.bpst ); // -recentrage de 0.5 bins
	laySL->spectropix = Lpix;	// on a la un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
	if	( opt_lin )
		panneau->bandes[ib]->Ylabel = "midi";
	}
++ib;
if	( ib < panneau->bandes.size() )
	{
	laySR = (layer_rgb *)panneau->bandes[ib]->courbes[0];
	laySR->set_km( 1.0 / (double)Rspek.fftstride );	// M est en samples, U en FFT-runs
	laySR->set_m0( 0.5 * (double)(Rspek.fftsize2D-Rspek.fftstride ) );
	laySR->set_kn( (double)Rspek.bpst );			// N est en MIDI-note (demi-tons), V est en bins
								// la midinote correspondant au bas du spectre2D
	laySR->set_n0( (double)Rspek.midi0 - 0.5/(double)Rspek.bpst ); // -recentrage de 0.5 bins
	laySR->spectropix = Rpix;	// on a la un pixbuf RGB de la wav entiere, de dimensions spek.H x spek.W
	if	( opt_lin )
		{
		panneau->bandes[ib]->Ylabel = "Hz";
		laySR->set_kn( double(Rspek.fftsize2D) / double(af->fsamp) );
		laySR->set_n0( 0.0 );
		}
	}
printf("end soft layout S, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
}

// layout pour le spectre1D ponctuel (domaine frequentiel)
void process::auto_layout2( gpanel * panneau, int time_curs )
{
if	( Lspek.spectre2D == NULL )
	return;
//// partie non repetable
if	( panneau->bandes.size() == 0 )
	{
	gstrip * curbande;
	layer_u<unsigned short> * curcour;

	panneau->offscreen_flag = 0;	// 1 par defaut
	panneau->mx = 60;

	// creer le strip pour les spectres
	curbande = new strip_x_midi;
	panneau->add_strip( curbande );
	// configurer le strip
	curbande->bgcolor.dR = 0.94;
	curbande->bgcolor.dG = 0.94;
	curbande->bgcolor.dB = 0.94;
	curbande->Ylabel = "spec";
	curbande->optX = 1;
	curbande->optretX = 0;
	// gpanel::smenu_set_title( curbande->smenu_y, "MAG ?" );

	// creer un layer
	curcour = new layer_u<unsigned short>;
	curbande->add_layer( curcour, "Lin" );
	panneau->full_valid = 0;
	printf("end hard layout 2, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
	}	// fin de la partie non-repetable
//// partie repetable a chaud, compatible avec un changement des params FFT
	{
	layer_u<unsigned short> * layL;
	layL = (layer_u<unsigned short> *)panneau->bandes[0]->courbes[0];
	if	( time_curs >= 0 )
		{
		layL->fgcolor.dR = 0.9;
		layL->fgcolor.dG = 0.0;
		layL->fgcolor.dB = 0.2;
		}
	else	{
		layL->fgcolor.dR = 0.0;
		layL->fgcolor.dG = 0.1;
		layL->fgcolor.dB = 0.9;
		}
	layL->set_kn( 1.0 );
	layL->set_n0( 0.0 );
	layL->set_km( (double)Lspek.bpst );		// M est en MIDI-note (demi-tons), U est en bins
							// la midinote correspondant au bas du spectre2D
	layL->set_m0( (double)Lspek.midi0);  // + 0.5/(double)Lspek.bpst ); PAS de recentrage de 0.5 bins
	if	( time_curs >= 0 )
		{			// le cas du spectre1D "ponctuel" extrait du spectrogramme existant
		// prise en compte de la position du curseur temporel (time_curs, en samples)
		int time_m0 = ( Lspek.fftsize2D - Lspek.fftstride ) / 2; // c'est le m0 du spectrogramme
		int ibin = ( time_curs - time_m0 ) / (int)Lspek.fftstride; 	// division entiere (ou floor)
		// bornage sur [0,W-1] ==> robustesse vs time_curs
		if	( ibin < 0 ) ibin = 0;
		if	( ibin > (int)( Lspek.W - 1 ) ) ibin = Lspek.W - 1;
		// printf("time_curs = %d ==> ibin = %d/%d\n", time_curs, ibin, Lspek.W ); fflush(stdout);
		// aller piquer une colonne du spectrogramme
		layL->V = Lspek.spectre2D + ( ibin * Lspek.H ); 
		}
	else	{			// le cas du spectre1D calcule a la demande
		layL->V = Lspek.spectre1D;
		}
	// H est le nombre de "bins" apres passage en echelle log
	layL->qu = Lspek.H;
	layL->scan();
	}
printf("end soft layout 2, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
}

// remplir la palette d'un gradient qui va de l'indice 0 a iend (exclu)
// completer le reste (saturation) avec la derniere couleur
// N.B. la palette contient toujours 65536 triplets RGB
void fill_palette_simple( unsigned char * pal, unsigned int iend )
{
if	( iend > 65535 )	// ne pas deborder de la palette !
	iend = 65535;
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
	palB[i] = 69;	// fond bleu
	}
// completer la zone de saturation
memset( palR + iend, val, 65536 - iend );
memset( palG + iend, val, 65536 - iend );
memset( palB + iend, val, 65536 - iend );
}

// colorisation d'un pixbuf sur le spectre2D precalcule, utilisant la palette referencee dans spek
// i.e. remplir le pixbuf avec l'image RBG obtenue par palettisation du spectre2D en u16
// c'est un wrapper sur spectro::spectre2rgb
void spectre2rgb( spectro * spek, GdkPixbuf * lepix )
{
printf("spectre2rgb, pixbuf %p\n", lepix );
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
// creer les pixbufs si necessaire, chacun pour le spectre2D entier
if	( Lpix == NULL )
	{
	printf("creation d'un pixbuf %u x %u\n", Lspek.W, Lspek.H ); fflush(stdout);
	Lpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Lspek.W, Lspek.H );
	}
if	( ( Rspek.spectre2D ) && ( Rpix == NULL ) )
	{
	printf("creation d'un pixbuf %u x %u\n", Rspek.W, Rspek.H ); fflush(stdout);
	Rpix = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, Rspek.W, Rspek.H );
	}
// coloriser les spectre2D (qui sont supposes deja referencer cette palette)
if	( Lspek.spectre2D )
	spectre2rgb( &Lspek, Lpix );
if	( Rspek.spectre2D )
	spectre2rgb( &Rspek, Rpix );
}

// liberer la memoire des spectros et pixbufs
// (safe to call unnecessarily)
void process::clean_spectros()
{
if	( Lspek.spectre2D )
	{ Lspek.specfree( 0 ); printf("Lspek freed\n"); fflush(stdout); }
if	( Rspek.spectre2D )
	{ Rspek.specfree( 0 ); printf("Rspek freed\n"); fflush(stdout); }
if	( Lpix )	// since gdk_pixbuf_unref() is deprecated
	{ g_object_unref( Lpix ); Lpix = NULL; printf("Lpix freed\n"); fflush(stdout); }
if	( Rpix )
	{ g_object_unref( Rpix ); Rpix = NULL; printf("Rpix freed\n"); fflush(stdout); }
}


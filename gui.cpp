// JAW = JLN's Audio Workstation
// portaudio est necessaire pour jouer le son, mais on peut compiler sans lui pour tester le GUI
#define  PAUDIO

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
#ifdef PAUDIO
#include "portaudio.h"
#include "pa_devs.h"
#endif
#include "fftw3.h"
#include "spectro.h"
#include "glostru.h"
#include "modpop2.h"

// unique variable globale exportee pour gasp() de modpop2
GtkWindow * global_main_window = NULL;

/** ============================ WAV file processing =============== */

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
curbande->optcadre = 1;	// pour economiser le fill du fond
// configurer le layer
curcour->set_km( 1.0 / (double)spek->fftstride );	// M est en samples, U en FFT-runs
curcour->set_m0( 0.5 * (double)spek->fftsize );		// recentrage au milieu de chaque fenetre FFT
curcour->set_kn( 1.0 / ( 12.0 * spek->log_opp ) );	// N est en MIDI-note (demi-tons), V est en bins
curcour->set_n0( (double)spek->midi0 );			// la midinote correspondant a spek->log_fbase
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
unsigned int bpst = 10;			// binxel-per-semi-tone : resolution spectro log
unsigned int octaves = 7;		// 7 octaves
spek->fftsize = 8192;
spek->fftstride = 1024;
spek->midi0 = 28;			// E1 = mi grave de la basse

// parametres derives
spek->H = octaves * 12 * bpst;
spek->log_opp = 1.0 / (double)( bpst * 12 );
spek->log_fbase = midi2Hz( spek->midi0 ) ;			// en Hz
spek->log_fbase /= ( (double)s->freq / (double)spek->fftsize );	// en pitch spectro

// allocations
retval = spek->init( s->wavsize );	// cette fonction calcule W
if	( retval )
	gasp("erreur init spectro %d", retval );
// preparer resampling log
spek->log_resamp_precalc();
//spek->log_resamp_dump();
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

/** ============================ PLAY with portaudio ======================= */

#define CODEC_FREQ		44100
#define CODEC_QCHAN		2
#define FRAMES_PER_BUFFER	256	// 256 <==> 5.8 ms i.e. 172.265625 buffers/s

#ifdef PAUDIO

// pilotage PLAY :	0 <= glo->iplay < glo->iplay1

// ---- portaudio callback vers codec stereo --------
// It may called at interrupt level on some machines so don't do anything
// that could mess up the system like calling malloc() or free().
static int portaudio_call( const void *inbuf, void *outbuf,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* time_info,
			PaStreamCallbackFlags status_flags,
			// void *userData
			glostru * glo )
{
unsigned int i, iend;
layer_s16_lod * wL, *wR;
short valL, valR;

iend = framesPerBuffer * CODEC_QCHAN;
wL = (layer_s16_lod *)glo->panneau.bandes[0]->courbes[0];
if	( glo->wavp.chan > 1 )	// test nombre de canaux (1 ou 2, pas plus !)
	wR = (layer_s16_lod *)glo->panneau.bandes[1]->courbes[0];
else	wR = wL;

for	( i = 0; i < iend; i+= CODEC_QCHAN )
	{
	if	( glo->iplay < 0 )
		valL = valR = 0;
	else	{
		if	( ( glo->iplay >= wL->qu ) || ( glo->iplay >= glo->iplay1 ) )
			{
			glo->iplay = -1;
			valL = valR = 0;
			}
		else	{
			valL = wL->V[glo->iplay];
			valR = wR->V[glo->iplay++];
			}
		}
	((short *)outbuf)[i]   = valL;
	((short *)outbuf)[i+1] = valR;
	}
return 0;
}

// demarrer la sortie audio
void audio_engine_start( glostru * glo, double mylatency, int mydevice, int pa_dev_options )
{
PaError err;

// Initialize library before making any other calls.
err = Pa_Initialize();
if	( err != paNoError )
	{ Pa_Terminate(); gasp("Error PA : %d : %s", err, Pa_GetErrorText( err ) ); }

// printf("nombre de devices : %d\n", Pa_GetDeviceCount() );

// Open an audio I/O stream.
// "For an example of enumerating devices and printing information about their capabilities
//  see the pa_devs.c program in the test directory of the PortAudio distribution."
// "On the PC, the user can specify a default device by setting an environment variable. For example, to use device #1.
//  set PA_RECOMMENDED_OUTPUT_DEVICE=1
//  The user should first determine the available device ids by using the supplied application "pa_devs"."

PaStreamParameters papa;
if	( mydevice < 0 )
	{
	// print_pa_devices( pa_dev_options );
	papa.device = Pa_GetDefaultOutputDevice();	// un device index au sens de PA (all host APIs merged)
	}
else 	papa.device = mydevice;
papa.channelCount = 2;    		// JAW uses always stereo output, best portability
papa.sampleFormat = paInt16;		// 16 bits, same reason
papa.suggestedLatency = mylatency;	// in seconds
papa.hostApiSpecificStreamInfo = NULL;

err = Pa_OpenStream(	&glo->stream,
			NULL,			// no input channels
			&papa,			// output channel parameters
                        (double)CODEC_FREQ,	// sample frequ
			FRAMES_PER_BUFFER,	// 256 <==> 5.8 ms i.e. 172.265625 buffers/s */
			0,			// no PaStreamFlags for the moment
			// cette usine a gaz c'est parceque notre callback prend un glostru* au lieu de void*
                        (int (*)(const void *, void *, unsigned long, const PaStreamCallbackTimeInfo*,
				 PaStreamCallbackFlags, void *))portaudio_call,
                        glo );
if	( err != paNoError )
	{ Pa_Terminate(); gasp("Error PA : %d : %s", err, Pa_GetErrorText( err ) ); }

const PaStreamInfo* PaInfo = Pa_GetStreamInfo( glo->stream );
if	( PaInfo == NULL )
	{ Pa_Terminate(); gasp("NULL PaStreamInfo");  }
// printf("output latency : %g vs %g s\n", PaInfo->outputLatency, mylatency );

err = Pa_StartStream( glo->stream );
if	( err != paNoError )
	{ Pa_Terminate(); gasp("Error PA : %d : %s", err, Pa_GetErrorText( err ) ); }
glo->iplay = -1;
}

// arreter la sortie audio
void audio_engine_stop( glostru * glo )
{
Pa_StopStream( glo->stream );
Pa_CloseStream( glo->stream );
Pa_Terminate();
}
#endif


/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
char lbuf[128];
volatile double newx;
if	( glo->iplay >= 0 )
	{			// Playing
#ifdef PAUDIO
	double t0 = (double)glo->iplay0 / (double)(glo->wavp.freq);
	// le temps present selon le timer portaudio, ramene au debut du play
	double t = t0 + Pa_GetStreamTime( glo->stream ) - glo->play_start_time;
	// le temps associe a l'indice courant, ramene au debut du play
	double pt = (double)glo->iplay / (double)(glo->wavp.freq);
	snprintf( lbuf, sizeof(lbuf), "%7.3f %7.3f  %4.3f", pt, t, t - pt );
	gtk_entry_set_text( GTK_ENTRY(glo->esta), lbuf );
#endif
	// gestion curseur temporel (reference = premier strip)
	if	( ( glo->iplay >= glo->iplay0 ) && ( glo->iplay < glo->iplay1 ) )
		{
		newx = glo->panneau.XdeM( glo->iplay );
		glo->panneau.xcursor = newx;
		glo->panneau.queue_flag = 1;
		}
	}
else	{			// Not Playing
	newx = glo->panneau.XdeM( glo->iplayp );
	if	( ( newx != glo->panneau.xdirty ) && ( newx >= 0.0 ) )
		{
		glo->panneau.xcursor = newx;
		glo->panneau.force_repaint = 1;
		glo->panneau.queue_flag = 1;
		}
	// profileur
	glo->idle_profiler_cnt++;
	if	( glo->idle_profiler_time != time(NULL) )
		{
		glo->idle_profiler_time = time(NULL);
		snprintf( lbuf, sizeof(lbuf), "%3d i %3d p", glo->idle_profiler_cnt, glo->panneau.paint_cnt );
		gtk_entry_set_text( GTK_ENTRY(glo->esta), lbuf );
		glo->idle_profiler_cnt = 0;
		glo->panneau.paint_cnt = 0;
		}
	}
// moderateur de drawing
if	( ( glo->panneau.queue_flag ) || ( glo->panneau.force_repaint ) )
	{
	// gtk_widget_queue_draw( glo->darea );
	glo->panneau.queue_flag = 0;
	glo->panneau.paint();
	}

return( -1 );
}

gint close_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{
gtk_main_quit();
return (TRUE);		// ne pas destroyer tout de suite
}


void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_main_quit();
}

void play_pause_call( GtkWidget *widget, glostru * glo )
{
if	( glo->iplay < 0 )
	{
	#ifdef PAUDIO
	// demarrer le son
	glo->play_start_time = Pa_GetStreamTime( glo->stream );
	glo->iplay = glo->iplayp;
	#endif
	}
else	{
	glo->iplayp = glo->iplay;	// retenir la position
	glo->iplay = -1;		// stop
	}
}

void rewind_call( GtkWidget *widget, glostru * glo )
{
// prendre les limites de la fenetre
glo->iplay0 = (int)glo->panneau.MdeX(0.0);
glo->iplayp = glo->iplay0;
glo->iplay1 = (int)glo->panneau.MdeX((double)glo->panneau.ndx);
// normalement idle_call va detecter si le curseur n'est pas au bon endroit
// et va le retracer
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back( double M, double N, void * vglo )
{
printf("youtpi %g %g\n", M, N );
glostru * glo = (glostru *)vglo;
glo->iplayp = M;
// normalement idle_call va detecter si le curseur n'est pas au bon endroit
// et va le retracer
}

void key_call_back( int v, void * vglo )
{
if	( v == ' ' )
	play_pause_call( NULL, (glostru *)vglo );
else if	( v == 'r' )
	rewind_call( NULL, (glostru *)vglo );
else if	( v == 'd' )
	{
	glostru * glo = (glostru *)vglo;
	glo->panneau.dump();
	printf("xdirty=%g iplayp=%d, xcursor=%g\n", glo->panneau.xdirty, glo->iplayp, glo->panneau.xcursor );
	}

}

/** ============================ context menus ======================= */

// enrichissement du menu X du panel glo->panneau qui est suppose etre audio
static void wav_X_menu_sec_call( GtkWidget *widget, glostru * glo )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	{
	double m0, m1;
	m0 = glo->panneau.MdeX( 0.0 );
	m1 = glo->panneau.MdeX( (double)glo->panneau.ndx );
	glo->panneau.kq = (double)(glo->wavp.freq);
	glo->panneau.zoomM( m0, m1 );
	glo->panneau.force_repaint = 1;
	}
}

static void wav_X_menu_samp_call( GtkWidget *widget, glostru * glo )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	{
	double m0, m1;		// ce truc complique car panel garde q0,q1 plutot que m0,m1
	m0 = glo->panneau.MdeX( 0.0 );
	m1 = glo->panneau.MdeX( (double)glo->panneau.ndx );
	glo->panneau.kq = 1.0;
	glo->panneau.zoomM( m0, m1 );
	glo->panneau.force_repaint = 1;
	}
}

void enrich_wav_X_menu( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;
GSList *group = NULL;

curmenu = glo->panneau.menu1_x;    // Don't need to show menus

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "seconds");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( wav_X_menu_sec_call ), (gpointer)glo );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), TRUE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
curitem = gtk_radio_menu_item_new_with_label( group, "samples");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( wav_X_menu_samp_call ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );
}

/** ============================ main, quoi ======================= */

static glostru theglo;

int main( int argc, char *argv[] )
{
glostru * glo = &theglo;
GtkWidget *curwidg;

gtk_init(&argc,&argv);

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "JAW");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

/* creer une drawing area resizable depuis la fenetre */
glo->darea = glo->panneau.layout( 640, 480 );
gtk_box_pack_start( GTK_BOX( glo->vmain ), glo->darea, TRUE, TRUE, 0 );

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
glo->sarea = glo->zbar.layout( 640 );
gtk_box_pack_start( GTK_BOX( glo->vmain ), glo->sarea, FALSE, FALSE, 0 );


/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Play/Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( play_pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bpla = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Rewind ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( rewind_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brew = curwidg;

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 160, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->esta = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bqui = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau.zoombar = &glo->zbar;
glo->panneau.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau;
// enrichir les menus de clic droit
enrich_wav_X_menu( glo );


gtk_widget_show_all( glo->wmain );

double mylatency = 0.090;	// 90 ms c'est conservateur
int myoutput = -1;		// choose default device
int pa_dev_options = 0;

if	( argc < 2 )
	gasp("fournir un nom de fichier WAV");

if	( argc >= 3 )
	{
	if	( argc < 4 )
		pa_dev_options = atoi( argv[2] );
	else	{
		myoutput = atoi( argv[3] );
		mylatency = strtod( argv[2], NULL );
		}
	}
// traiter choix options B1 vs B2
if	( pa_dev_options & 4 )
	{
	glo->panneau.drawab = glo->darea->window;	// special B1
	printf("Sol. B1\n");
	}
else	printf("Sol. B2\n");

snprintf( glo->wnam, sizeof(glo->wnam), argv[1] );

if	( wave_process_full( glo->wnam, &glo->wavp, &glo->panneau, &glo->spec ) )
	gasp("echec lecture %s", glo->wnam );

glo->panneau.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau.key_callback_register( key_call_back, (void *)glo );

// forcer un full initial pour que tous les coeffs de transformations soient a jour
glo->panneau.full_valid = 0;
// refaire un configure car celui appele par GTK est arrive trop tot
glo->panneau.configure();
//glo->panneau.dump();

rewind_call( NULL, glo );

#ifdef PAUDIO
// demarrer la sortie audio (en silence)
audio_engine_start( glo, mylatency, myoutput, pa_dev_options );
printf("audio engine started\n");
#endif

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);
gtk_main();


printf("closing\n");
#ifdef PAUDIO
audio_engine_stop( glo );
#endif
return(0);
}

// JAW = JLN's Audio Workstation
// portaudio est necessaire pour jouer le son, mais on peut compiler sans lui pour tester le GUI
// #define  PAUDIO

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
#include "gluplot.h"
#ifdef PAUDIO
#include "portaudio.h"
#include "pa_devs.h"
#endif
#include "fftw3.h"
#include "spectro.h"
#include "wav_head.h"
#include "process.h"
#include "glostru.h"
#include "modpop2.h"

// unique variable globale exportee pour gasp() de modpop2
GtkWindow * global_main_window = NULL;


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
if	( glo->pro.wavp.chan > 1 )	// test nombre de canaux (1 ou 2, pas plus !)
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
	double t0 = (double)glo->iplay0 / (double)(glo->pro.wavp.freq);
	// le temps present selon le timer portaudio, ramene au debut du play
	double t = t0 + Pa_GetStreamTime( glo->stream ) - glo->play_start_time;
	// le temps associe a l'indice courant, ramene au debut du play
	double pt = (double)glo->iplay / (double)(glo->pro.wavp.freq);
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

void level_slider_call( GtkAdjustment *adjustment, glostru * glo )
{
if	( adjustment->value != glo->level )
	{
	glo->level = adjustment->value;
	printf("level = %g now\n", glo->level ); fflush(stdout);
	double abslevel = double(glo->pro.Lspek.umax);
	abslevel *= glo->level;
	abslevel /= 100.0;
	abslevel = round(abslevel);
	glo->pro.colorize( &glo->pro.Lspek, glo->pro.Lpix, int(abslevel) );
	if	( glo->pro.qspek >= 2 )
		glo->pro.colorize( &glo->pro.Rspek, glo->pro.Rpix, int(abslevel) );
	glo->panneau.force_redraw = 1;
	glo->panneau.force_repaint = 1;
	}
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

// cette fonction devra etre transportee dans gpanel
// ic = -1 <==> strip entier
static void toggle_vis( glostru * glo, unsigned int ib, int ic )	// ignore ic pour le moment
{
if	( ib >= glo->panneau.bandes.size() )
	return;
if	( ic < 0 )
	glo->panneau.bandes[ib]->visible ^= 1;
else	{
	if	( (unsigned int)ic >= glo->panneau.bandes[ib]->courbes.size() )
		return;
	glo->panneau.bandes[ib]->courbes[ic]->visible ^= 1;
	}
int ww, wh;
ww = glo->panneau.fdx; wh = glo->panneau.fdy;	// les dimensions de la drawing area ne changent pas
glo->panneau.resize( ww, wh );			// mais il faut recalculer la hauteur des bandes
glo->panneau.refresh_proxies();
glo->panneau.force_repaint = 1;
}

void key_call_back( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	case '0' : toggle_vis( glo, 0, -1 ); break;
	case '1' : toggle_vis( glo, 1, -1 ); break;
	case '2' : toggle_vis( glo, 2, -1 ); break;
	case '3' : toggle_vis( glo, 3, -1 ); break;
	case '4' : toggle_vis( glo, 4, 0 ); break;
	case '5' : toggle_vis( glo, 4, 1 ); break;
	case '6' : toggle_vis( glo, 4, 2 ); break;
	case ' ' :
		play_pause_call( NULL, glo );
		break;
	case 'r' :
		rewind_call( NULL, glo );
		break;
	case 'd' :
		glo->panneau.dump();
		printf("xdirty=%g iplayp=%d, xcursor=%g\n", glo->panneau.xdirty, glo->iplayp, glo->panneau.xcursor );
		fflush(stdout);
		break;
	case 'c' :	// re calculer les courbes en fonction du slider
		double abslevel = double(glo->pro.Lspek.umax);
		abslevel *= glo->level;
		abslevel /= 100.0;
		abslevel = round(abslevel);
		unsigned int fplancher = 6;	// Hz (2.69 Hz/bin @ 16384/44100)
		glo->pro.find_peaks( &glo->pro.Lspek, 0, (fplancher*glo->pro.Lspek.fftsize)/(glo->pro.wavp.freq), (unsigned short)(abslevel)/2 );
		glo->pro.find_peaks( &glo->pro.Rspek, 1, (fplancher*glo->pro.Rspek.fftsize)/(glo->pro.wavp.freq), (unsigned short)(abslevel)/2 );
		glo->panneau.force_redraw = 1;
		glo->panneau.force_repaint = 1;
		break;
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
	glo->panneau.kq = (double)(glo->pro.wavp.freq);
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
glo->darea = glo->panneau.layout( 800, 600 );
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

#ifdef PAUDIO
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
#endif

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

GtkAdjustment *curadj;

curwidg = gtk_hbox_new( FALSE, 4 );
gtk_box_pack_start( GTK_BOX(glo->vmain), curwidg, FALSE, FALSE, 0);
glo->hsli = curwidg;

curwidg = gtk_label_new("peak level :");
gtk_box_pack_start( GTK_BOX(glo->hsli), curwidg, FALSE, FALSE, 0);

glo->level = 100;		//value,lower,upper,step_increment,page_increment,page_size);
curadj = GTK_ADJUSTMENT( gtk_adjustment_new( glo->level, 1.0, 100.0, 1.0, 5.0, 0 ));
g_signal_connect( curadj, "value_changed",
		  G_CALLBACK(level_slider_call), (gpointer)glo );

curwidg = gtk_hscale_new(curadj);
gtk_scale_set_digits( GTK_SCALE(curwidg), 0 );
gtk_box_pack_start( GTK_BOX(glo->hsli), curwidg, TRUE, TRUE, 0 );

// connecter la zoombar au panel et inversement
glo->panneau.zoombar = &glo->zbar;
glo->panneau.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau;
// enrichir les menus de clic droit
enrich_wav_X_menu( glo );


gtk_widget_show_all( glo->wmain );


if	( argc < 2 )
	gasp("fournir un nom de fichier WAV");

#ifdef PAUDIO
double mylatency = 0.090;	// 90 ms c'est conservateur
int myoutput = -1;		// choose default device
int pa_dev_options = 0;
if	( argc >= 3 )
	{
	if	( argc < 4 )
		pa_dev_options = atoi( argv[2] );
	else	{
		myoutput = atoi( argv[3] );
		mylatency = strtod( argv[2], NULL );
		}
	}
// traiter choix options B1 vs B2 (c'est ballot, cette option n'est accessible que si on a PAUDIO)
if	( pa_dev_options & 4 )
	{
	glo->panneau.drawab = glo->darea->window;	// special B1
	printf("Sol. B1\n");
	}
else	printf("Sol. B2\n");
#else
if	( argc >= 3 )
	{
        unsigned int fmax = atoi(argv[2]);
	if	( fmax > 50 )
		{
		glo->pro.Lspek.fmax = fmax;
		glo->pro.Rspek.fmax = fmax;
		}
	if	( argc >= 4 )
		snprintf( glo->pro.swnam, sizeof( glo->pro.swnam), argv[3] );
	}
#endif

snprintf( glo->pro.wnam, sizeof( glo->pro.wnam), argv[1] );

int retval = glo->pro.wave_process_full();
if	( retval )
	gasp("echec lecture %s, erreur %d", glo->pro.wnam, retval );
fflush(stdout);
// preparer le layout pour wav L (et R si stereo) et spectro
glo->pro.prep_layout( &glo->panneau );
retval = glo->pro.connect_layout( &glo->panneau );
if	( retval )
	gasp("echec connect layout, erreur %d", retval );
fflush(stdout);

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

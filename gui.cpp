// JAW = JLN's Audio Workstation
// portaudio est necessaire pour jouer le son, mais on peut compiler sans lui pour tester le GUI
// pour l'activer, definir USE_PORTAUDIO dans le makefile ou le projet
// de plus pour qu'il utilise ASIO, definir PA_USE_ASIO

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <mpg123.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "JLUP/jluplot.h"
#include "JLUP/gluplot.h"
#ifdef USE_PORTAUDIO
  #include "portaudio_2011.h"
  #include "pa_devs.h"
#endif
#include "fftw3.h"
#include "spectro.h"
#include "autobuf.h"
#include "wavio.h"
#include "mp3in.h"
#include "process.h"
#include "param.h"
#include "gui.h"
#include "modpop3.h"
#include "cli_parse.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;


/** ============================ PLAY with portaudio ======================= */

#define CODEC_FREQ		44100
#define CODEC_QCHAN		2
#define FRAMES_PER_BUFFER	128	// 256 <==> 5.8 ms i.e. 172.265625 buffers/s

#ifdef USE_PORTAUDIO
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
unsigned int i, samplesPerBuffer;
short * wL, *wR;

samplesPerBuffer = framesPerBuffer * CODEC_QCHAN;

// do we have enough samples to play ?
if	( glo->iplay >= 0 )
	{
	if	(
		( ( glo->iplay + (int)framesPerBuffer ) > (int)glo->pro.Lbuf.size ) ||
		( ( glo->iplay + (int)framesPerBuffer ) > glo->iplay1 )
		)
		glo->iplay = -1;
	}

if	( glo->iplay >= 0 )
	{			// yes we have at least samplesPerBuffer samples to play
	wL = glo->pro.Lbuf.data;
	if	( glo->pro.af->qchan > 1 )
		wR = glo->pro.Rbuf.data;	// stereo
	else	wR = wL;			// mono
	for	( i = 0; i < samplesPerBuffer; i+= CODEC_QCHAN )
		{
		((short *)outbuf)[i]   = wL[glo->iplay];
		((short *)outbuf)[i+1] = wR[glo->iplay++];
		}
	}
else	{			// play silence
	for	( i = 0; i < samplesPerBuffer; i++ )
		{
		((short *)outbuf)[i]   = 0;
		}
	// petite experience de debug pour verifier que c'est bien notre FRAMES_PER_BUFFER
	glo->iplay = -framesPerBuffer;
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

if	( pa_dev_options >= 0 )
	print_pa_devices( pa_dev_options );

PaStreamParameters papa;
if	( mydevice < 0 )
	papa.device = Pa_GetDefaultOutputDevice();	// un device index au sens de PA (all host APIs merged)
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
volatile double newx;
if	( glo->iplay >= 0 )
	{			// Playing
	/* experience incomprehensible a clarifier *
	#ifdef USE_PORTAUDIO
	double t0 = (double)glo->iplay0 / (double)(glo->pro.af->fsamp);
	// le temps present selon le timer portaudio, ramene au debut du play
	double t = t0 + Pa_GetStreamTime( glo->stream ) - glo->play_start_time;
	// le temps associe a l'indice courant, ramene au debut du play
	double pt = (double)glo->iplay / (double)(glo->pro.af->fsamp);
	char lbuf[128];
	snprintf( lbuf, sizeof(lbuf), "%7.3f %7.3f  %4.3f", pt, t, t - pt );
	gtk_entry_set_text( GTK_ENTRY(glo->esta), lbuf );
	#endif
	//*/
	// gestion curseur temporel (reference = premier strip)
	if	( ( glo->iplay >= glo->iplay0 ) && ( glo->iplay < glo->iplay1 ) )
		{
		newx = glo->panneau.XdeM( glo->iplay );
		glo->panneau.xcursor = newx;
		glo->panneau.paint();		// meme si glo->panneau.force_repaint == 0;
		}
	}
else	{			// Not Playing
	newx = glo->panneau.XdeM( glo->iplayp );
	if	( ( newx != glo->panneau.xdirty ) && ( newx >= 0.0 ) )
		{
		glo->panneau.xcursor = newx;
		glo->panneau.force_repaint = 1;
		}
	if	( glo->panneau.force_repaint )
		glo->panneau.paint();
	}

// moderateur de repaint pour le panel qui est dans la fenetre parametres
// 	- glo->para.wmain est invisible au depart de l'appli et chaque fois qu'on la ferme avec bouton [X]
//	  mais reste visible quand iconifiee ou cachee derriere une autre
//	- les pages de notebook sont toujours 'visibles', mais la page affichees est identifiable par son index  
if	( ( gtk_widget_get_visible(glo->para.wmain) ) && ( gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->para.nmain) ) == 0 ) )
	if	( glo->para.panneau.force_repaint )
		glo->para.panneau.paint();

return( -1 );
}

gint close_event_call( GtkWidget *widget, GdkEvent *event, gpointer data )
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
	#ifdef USE_PORTAUDIO
	if	( glo->option_noaudio == 0 )
		{
		// demarrer le son
		// glo->play_start_time = Pa_GetStreamTime( glo->stream );
		glo->iplay = glo->iplayp;
		}
	#endif
	}
else	{
	glo->iplayp = glo->iplay;	// retenir la position
	glo->iplay = -1;		// stop
	}
printf("play/pause: iplay=%d, iplayp=%d, iplay0=%d, iplay1=%d\n", glo->iplay, glo->iplayp, glo->iplay0, glo->iplay1 ); fflush(stdout);
}

// cette fonction restreint le play a la fenetre courante, et met le curseur au debut
void rewind_call( GtkWidget *widget, glostru * glo )
{
// prendre les limites de la fenetre
glo->iplay0 = (int)glo->panneau.MdeX(0.0);
if	( glo->iplay0 < 0 )
	glo->iplay0 = 0;
glo->iplayp = glo->iplay0; // normalement idle_call va detecter si le curseur n'est pas au bon endroit et va le retracer
glo->iplay1 = (int)glo->panneau.MdeX((double)glo->panneau.ndx);	// iplay1 trop grand est supporte par portaudio_call

printf("rewind: iplay=%d, iplayp=%d, iplay0=%d, iplay1=%d\n", glo->iplay, glo->iplayp, glo->iplay0, glo->iplay1 ); fflush(stdout);
}

void param_call( GtkWidget *widget, glostru * glo )
{
glo->para.show();
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back( double M, double N, void * vglo )
{
// printf("clic M N %g %g\n", M, N );
glostru * glo = (glostru *)vglo;
glo->iplayp = M;
// normalement idle_call va detecter si le curseur n'est pas au bon endroit et va le retracer

// spectre instantane
if	( glo->pro.Lspek.allocatedWH )
	{
	glo->pro.auto_layout2( &glo->para.panneau, glo->iplayp );	// assure le scan
	glo->para.panneau.force_repaint = 1;
	glo->para.panneau.fullMN();
	}
}

void key_call_back( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	case GDK_KEY_KP_0 :
	case '0' : glo->panneau.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau.toggle_vis( 0, 1 ); break;
	case GDK_KEY_KP_2 :
	case '2' : glo->panneau.toggle_vis( 1, 0 ); break;
	case GDK_KEY_KP_3 :
	case '3' : glo->panneau.toggle_vis( 2, 0 ); break;
	case ' ' :
		play_pause_call( NULL, glo );
		break;
	case 'r' :
		rewind_call( NULL, glo );
		break;
	case 'p' :
		glo->panneau.png_save_drawpad( "drawpad.png" );
		break;
	case 'd' :
		{
		glo->panneau.dump();
		printf("xdirty=%g iplayp=%d, xcursor=%g\n", glo->panneau.xdirty, glo->iplayp, glo->panneau.xcursor );
		glo->para.panneau.dump();
		fflush(stdout);
		} break;
	case GDK_KEY_F1 :
		printf("F key hit\n"); fflush(stdout);
		break;
	//
	case 'v' :
		printf("sarea realized=%d, visible=%d, main visible=%d\n",
			gtk_widget_get_realized(glo->para.sarea), gtk_widget_get_visible(glo->para.sarea),
			gtk_widget_get_visible(glo->para.wmain) );
		printf("notebook page %d\n", gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->para.nmain) ) );
		fflush(stdout);
		break;
	//
	case 'F' :
		GtkWidget *dialog;
		dialog = gtk_file_chooser_dialog_new ("Ouvrir WAV ou MP3", GTK_WINDOW(glo->wmain),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL );
		gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER (dialog), "." );
		if	( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
			{
			char * fnam;
			fnam = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER (dialog) );
			printf("chosen: %s\n", fnam ); fflush(stdout);
			glo->wavisualize( fnam );
			if	( glo->option_spectrogramme )
				glo->spectrographize();
			g_free( fnam );
			}
		else	gasp("audio already loaded");
		gtk_widget_destroy (dialog);
		break;
	}
}

/** ============================ methodes ======================= */

void glostru::wavisualize( const char * fnam )	// chargement et layout d'un fichier audio
{
snprintf( this->pro.wnam, sizeof( this->pro.wnam ), fnam );
gtk_entry_set_text( GTK_ENTRY( this->esta ), fnam );
// nettoyer TOUT
panneau.reset();
para.panneau.reset();
pro.clean_spectros();
panneau.dump();
// charger fichier
int retval;
retval = this->pro.audiofile_process();
if	( retval )
	{
	printf("echec lecture %s, erreur audiofile_process %d\n", this->pro.wnam, retval );
	fflush(stdout);
	return;
	}
fflush(stdout);
// preparer le layout pour wav L (et R si stereo)
this->pro.prep_layout_W( &this->panneau );
retval = this->pro.connect_layout_W( &this->panneau );
if	( retval )
	gasp("echec connect layout, erreur %d", retval );
fflush(stdout);
this->panneau.force_repaint = 1; this->panneau.force_redraw = 1;
iplay = -1; iplayp = iplay0 = 0; iplay1 = 2000000000;
}


void glostru::spectrographize()	// creation et layout du spectrogramme
{
if	( ( this->pro.Lbuf.size ) && ( this->panneau.bandes.size() >= 1 ) )
	{
	int retval;
	parametrize();
	retval = this->pro.spectrum_compute( this->option_monospec );
	if	( retval )
		gasp("echec spectrum, erreur %d", retval );
	fflush(stdout);
	this->pro.auto_layout_S( &this->panneau );
	this->pro.auto_layout2( &this->para.panneau, this->iplayp ); 
	fflush(stdout);
	this->panneau.full_valid = 0; this->panneau.init_flags = 0;
	}
this->panneau.force_repaint = 1; this->panneau.force_redraw = 1;
}

void glostru::parametrize()	// recuperation des parametres editables
{
if	( GTK_IS_COMBO_BOX(para.cfwin) )	// pour contourner les widgets qui ne sont pas prets
	{
	pro.Lspek.window_type = gtk_combo_box_get_active( GTK_COMBO_BOX(para.cfwin) );
	pro.Lspek.fftsize     = 1 << ( 9 + gtk_combo_box_get_active( GTK_COMBO_BOX(para.cfsiz) ) );
	unsigned int stride   = atoi( gtk_entry_get_text( GTK_ENTRY(para.estri) ) ); 
	if	( stride < ( pro.Lspek.fftsize / 16 ) )
		stride = ( pro.Lspek.fftsize / 16 );
	if	( stride > pro.Lspek.fftsize )
		stride = pro.Lspek.fftsize;
	pro.Lspek.fftstride   = stride;
	char tbuf[16];
	snprintf( tbuf, sizeof(tbuf), "%u", stride );
	gtk_entry_set_text( GTK_ENTRY(para.estri), tbuf );
	pro.Lspek.bpst        = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(para.bbpst) );

	pro.Rspek.window_type = pro.Lspek.window_type;
	pro.Rspek.fftsize     = pro.Lspek.fftsize;
	pro.Rspek.fftstride   = pro.Lspek.fftstride;
	pro.Rspek.bpst        = pro.Lspek.bpst;
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
	glo->panneau.kq = (double)(glo->pro.af->fsamp);
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

curmenu = glo->panneau.smenu_x;    // Don't need to show menus

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
setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

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
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 600 );
glo->panneau.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
glo->darea = curwidg;

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
curwidg = gtk_drawing_area_new();
glo->zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->zarea = curwidg;


/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Play/Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( play_pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bpla = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("Restrict Play");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( rewind_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->brew = curwidg;

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 160, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->esta = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Param ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( param_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bpar = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bqui = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau.zoombar = &glo->zbar;
glo->panneau.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau;
// enrichir les menus de clic droit
enrich_wav_X_menu( glo );


gtk_widget_show_all( glo->wmain );

// Traitement des arguments CLI, accepte dans n'importe quel ordre :
// - des options de la forme -Xval ou -X=val ou -X Val ou -X
// - une chaine nue t.q. file path
 
//	variables concernees, avec les valeurs par defaut
double mylatency = 0.090;	// -L // 90 ms c'est conservateur
int myoutput = -1;		// -d // -1 = choose system default device
int pa_dev_options = -1;	// -p // listage audio devices (-1=rien, 0=minimal, 1=sample rates, 2=ASIO, 3=tout)
int solB = 0;			// -B // variante pour copie drawpad (cf gluplot.cpp) "-B" pour B1, sinon defaut = B2 
glo->option_spectrogramme = 0;	// -S // calcul de spectrogrammes 2D
glo->option_monospec = 0;	// -m // spectrogrammes 2D sur L+R si stereo
glo->option_noaudio = 0;	// -N // no audio output (for debug)
const char * fnam = NULL;

// 	parsage CLI
cli_parse * lepar = new cli_parse( argc, (const char **)argv, "Ldp" );
// le parsage est fait, on recupere les args !
const char * val;
if	( ( val = lepar->get( 'L' ) ) )	mylatency = strtod( val, NULL );
if	( ( val = lepar->get( 'd' ) ) )	myoutput = atoi( val );
if	( ( val = lepar->get( 'p' ) ) )	pa_dev_options = atoi( val );
if	( ( val = lepar->get( 'B' ) ) )	solB = 1;
if	( ( val = lepar->get( 'S' ) ) )	glo->option_spectrogramme = 1;
if	( ( val = lepar->get( 'm' ) ) )	glo->option_monospec = 1;
if	( ( val = lepar->get( 'N' ) ) )	glo->option_noaudio = 1;
if	( ( val = lepar->get( 'h' ) ) )
	{
	printf( "options :\n"
	"-L <val> : latence demandee (defaut : 0.090)\n"
	"-d <id>  : choix output device (defaut -1 = system default)\n"
	"-p <opt> : listage devices (-1=rien, 0=minimal, 1=sample rates, 2=ASIO, 3=tout)\n"
	"-B	  : variante pour copie drawpad (cf gluplot.cpp) '-B' pour B1, sinon defaut = B2\n"
	"-S	  : calcul spectrogramme a l'ouverture\n"
	"-m	  : spectrogramme toujours mono\n"
	"-N	  : no audio output\n" );
	return 0;
	}
fnam = lepar->get( '@' );		// get avec la clef '@' rend la chaine nue 

printf( "output device %d, latency %g, pa_dev option %d\n", myoutput, mylatency, pa_dev_options );
fflush(stdout);

// traiter choix options B1 vs B2
if	( solB == 1 )
	{
	glo->panneau.drawab = glo->darea->window;	// force B1  pour gpanel::drawpad_compose()
	printf("Sol. B1\n");
	}
else	printf("Sol. B2\n");

// traitement fichier donnees audio, wav ou mp3
if	( fnam )
	{
	glo->wavisualize( fnam );
	if	( glo->option_spectrogramme )
		glo->spectrographize();
	}
glo->panneau.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau.key_callback_register( key_call_back, (void *)glo );

// preparer le layout de la fenetre non modale 'param'
glo->para.build();
// alors glo->para.panneau existe et est connecte a glo->para.sarea
// glo->pro.prep_layout2( &glo->para.panneau ); <-- seulement sur action clic_call_back()
// glo->pro.connect_layout2( &glo->para.panneau, 0 );  <-- idem

#ifdef USE_PORTAUDIO
if	( glo->option_noaudio == 0 )
	{
	// demarrer la sortie audio (en silence)
	audio_engine_start( glo, mylatency, myoutput, pa_dev_options );
	printf("audio engine started\n");
	}
#endif

glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

fflush(stdout);

gtk_main();

g_source_remove( glo->idle_id );

#ifdef USE_PORTAUDIO
printf("stopping audio out\n");
if	( glo->option_noaudio == 0 )
	audio_engine_stop( glo );
#endif

/* experience de liberation de memoire (non necessaire ici) *

printf(">>>> begin deletion of waves\n"); fflush(stdout);
glo->panneau.dump();
glo->panneau.reset();
glo->panneau.dump();

printf(">>>> begin deletion of aux plot\n"); fflush(stdout);
glo->para.panneau.dump();
glo->para.panneau.reset();
glo->para.panneau.dump();

printf(">>>> begin deletion of spectra\n"); fflush(stdout);
glo->pro.clean_spectros();

printf(">>>> end deletion\n"); fflush(stdout);

//*/
return(0);
}

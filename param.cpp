#include <gtk/gtk.h>
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

/** GTK callbacks ------------------------------------ */

// pour le bouton X du bandeau, en fait on ne delete pas, on cache seult
static gint param_view_delete_call( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_widget_hide( widget );
return (TRUE);
}

void load_call( GtkWidget *widget, glostru * glo )
{
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
}

/** main methods --------------------------------------*/

void param_view::build()
{
GtkWidget *curwidg;

// c'est une fenetre banale en fait, elle se declare comme la fenetre principale
// ses events sont traites par la main loop principale
// la seule difference c'est que son "delete_event" la cache au lieu de terminer l'application
curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
gtk_window_set_modal( GTK_WINDOW(curwidg), FALSE );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( param_view_delete_call ), NULL );

gtk_window_set_title( GTK_WINDOW(curwidg), "Parameters" );
gtk_container_set_border_width( GTK_CONTAINER(curwidg), 2 );
// set a minimum size
gtk_widget_set_size_request( curwidg, 640, 480 );
wmain = curwidg;

// notebook (collection de tabs)
curwidg = gtk_notebook_new();
gtk_container_add( GTK_CONTAINER( wmain ), curwidg );
nmain = curwidg;

// container pour les params de layout
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Multitrack") );
vlay = curwidg;

// container pour les params de spectre
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Spectrum") );
vspe = curwidg;

// container pour les fichiers
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Files") );
vfil = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("Load File");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( load_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

// container pour les I/O (audio et midi)
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("I/O ports") );
vpor = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 640, 320 );
panneau.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( vspe ), curwidg, TRUE, TRUE, 0 );
sarea = curwidg;

// on doit faire un show de tous les widgets sauf la top window
// solution peu elegante :
//	gtk_widget_show_all( wmain );
//	gtk_widget_hide( wmain );
// solution retenue: faire un show de tous les descendants de la top mais pas elle :
GList * momes = gtk_container_get_children( GTK_CONTAINER(wmain) );
while	( momes )
	{
	gtk_widget_show_all( GTK_WIDGET(momes->data) );
	momes = momes->next;
	}
}


void param_view::show()
{
if	( !wmain )
	build();
gtk_window_present( GTK_WINDOW(wmain) );
}

void param_view::hide()
{
gtk_widget_hide( wmain );
}

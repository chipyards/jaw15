#include <gtk/gtk.h>

#include "param.h"


/** GTK callbacks ------------------------------------ */

// pour le bouton X du bandeau, en fait on ne delete pas, on cache seult
static gint param_view_delete_call( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_widget_hide( widget );
return (TRUE);
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

// container pour les I/O (audio et midi)
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("I/O ports") );
vpor = curwidg;

gtk_widget_show_all( wmain );
}


void param_view::show()
{
if	( wmain )
	gtk_window_present( GTK_WINDOW(wmain) );
else	build();
}

void param_view::hide()
{
gtk_widget_hide( wmain );
}

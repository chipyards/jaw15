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

// container pour les params de process
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Process") );
vpro = curwidg;

// container pour les params de layout
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Multitrack") );
vlay = curwidg;

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

/** param_analog : un slider */

GtkWidget * param_analog::build()
{
hbox = gtk_hbox_new( FALSE, 4 );
label = gtk_label_new( tag );
gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 0);
double ainc = 1.0; int d = decimales;
while	( d > 0 )
	{ ainc *= 0.1; --d; }
						//value,lower,upper,step_increment,page_increment,page_size);
adjustment = GTK_ADJUSTMENT( gtk_adjustment_new( amin, amin, amax, ainc, ainc*2, 0 ) );
if	( callback )
	g_signal_connect( adjustment, "value_changed", G_CALLBACK(callback), (gpointer)this );

hscale = gtk_hscale_new(adjustment);
gtk_scale_set_digits( GTK_SCALE(hscale), decimales );
gtk_box_pack_start( GTK_BOX(hbox), hscale, TRUE, TRUE, 0 );
gtk_widget_show_all( hbox );

return hbox;
}

double param_analog::get_value()
{
return adjustment->value;
}

void param_analog::set_value( double v )
{
adjustment->value = v;
}

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

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "gluplot.h"
#include "layer_u.h"
#include "layer_f_param.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "demo1.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
// moderateur de drawing
if	( glo->panneau1.force_repaint )
	glo->panneau1.paint();
	
if	( glo->panneau2.force_repaint )
	glo->panneau2.paint();

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

// callbacks de widgets
void run_call( GtkWidget *widget, glostru * glo )
{
glo->running = 1;
glo->panneau2.pdf_modal_layout( glo->wmain );
}

void pause_call( GtkWidget *widget, glostru * glo )
{
glo->running = 0;
char fnam[32], capt[128];
snprintf( fnam, sizeof(fnam), "demo1.2.pdf" );
modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
if	( ( glo->panneau2.bandes.size() ) && ( glo->panneau2.bandes[0]->courbes.size() ) )
	snprintf( capt, sizeof(capt), "demo1.2: %s", glo->panneau2.bandes[0]->courbes[0]->label.c_str() );
else	return;
modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
glo->panneau2.pdfplot( fnam, capt );
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back( double M, double N, void * vglo )
{
printf("clic M N %g %g\n", M, N );
// glostru * glo = (glostru *)vglo;
}

void key_call_back( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// la visibilite
	case GDK_KEY_KP_0 :
	case '0' : glo->panneau1.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau1.toggle_vis( 0, 1 ); break;
	// l'option offscreen drawpad
	case 'o' : glo->panneau1.offscreen_flag = 1; break;
	case 'n' : glo->panneau1.offscreen_flag = 0; break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		{
		glo->panneau1.dump();
		glo->panneau2.dump();
		fflush(stdout);
		} break;
	// demo modpop3
	case 'k' :
		{
		char tbuf[32];
		snprintf( tbuf, sizeof(tbuf), "%g", glo->k );
		modpop_entry( "K setting", "rapport des frequences", tbuf, sizeof(tbuf), GTK_WINDOW(glo->wmain) );
		glo->k = strtod( tbuf, NULL );
		glo->gen_data();
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		glo->panneau2.force_repaint = 1;
		} break;
	//
	case 't' :
		glo->panneau1.qtkx *= 1.5;
		glo->panneau2.qtkx *= 1.5;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		glo->panneau2.force_repaint = 1;
		break;
	//
	case 'P' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo1.1.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "C'est imposant ma soeur" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ l'application ==================== */

void glostru::gen_data()
{
// donnees pour les tests
double omega1 = 2.0 * M_PI / 100.0;
double omega2 = k * omega1;
for	( int i = 0; i < QBUF; ++i )
	{
	// Xbuf[i] = 0.012  * (float)( i % 100 );
	Xbuf[i] = 0.12 + (float)cos( omega1 * (float)i ); 
	Ybuf[i] = (float)sin( omega2 * (float)i ); 
	}

}

void glostru::process()
{
gen_data();

// layout jluplot

// marge pour les textes
// panneau1.mx = 60;
// panneau1.offscreen_flag = 0;

// creer le strip pour les waves
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 2;

// creer un layer
layer_u<float> * curcour;
curcour = new layer_u<float>;
curbande->add_layer( curcour, "ValX" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// creer un layer
curcour = new layer_u<float>;
curbande->add_layer( curcour, "ValY" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
curcour = (layer_u<float> *)panneau1.bandes[0]->courbes[0];
curcour->V = Xbuf;
curcour->qu = QBUF;
curcour->scan();	// alors on peut faire un scan

curcour = (layer_u<float> *)panneau1.bandes[0]->courbes[1];
curcour->V = Ybuf;
curcour->qu = QBUF;
curcour->scan();	// alors on peut faire un scan

// layout jluplot pour panneau2
panneau2.offscreen_flag = 0;

// creer le strip
curbande = new gstrip;
panneau2.add_strip( curbande );

panneau2.pdf_DPI = 100;	// defaut est 72

// configurer le strip
curbande->bgcolor.set( 1.0, 0.95, 0.85 );
curbande->Ylabel = "XY";
curbande->optX = 1;
curbande->subtk = 10;

// creer un layer
layer_f_param * curcour2;
curcour2 = new layer_f_param;
curbande->add_layer( curcour2, "Lissajoux" );

// configurer le layer
curcour2->set_km( 1.0 );
curcour2->set_m0( 0.0 );
curcour2->set_kn( 1.0 );
curcour2->set_n0( 0.0 );
curcour2->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour2 = (layer_f_param *)panneau2.bandes[0]->courbes[0];
curcour2->U = Xbuf;
curcour2->V = Ybuf;
curcour2->qu = QBUF;
curcour2->scan();	// alors on peut faire un scan

}


/** ============================ main, quoi ======================= */

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
gtk_widget_set_size_request( curwidg, 800, 360 );
glo->panneau1.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
glo->darea1 = curwidg;

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
curwidg = gtk_drawing_area_new();
glo->zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->zarea1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 360 );
glo->panneau2.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
glo->darea2 = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("PDF (file chooser)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("PDF (modpop3)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bpau = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;

gtk_widget_show_all( glo->wmain );

glo->panneau1.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau1.key_callback_register( key_call_back, (void *)glo );

glo->process();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);

gtk_main();


printf("closing\n");
return(0);
}


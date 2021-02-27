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
#include "layer_f.h"

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
if	( ( glo->panneau.queue_flag ) || ( glo->panneau.force_repaint ) )
	{
	// gtk_widget_queue_draw( glo->darea );
	glo->panneau.queue_flag = 0;
	glo->panneau.paint();
	}

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
}
void pause_call( GtkWidget *widget, glostru * glo )
{
glo->running = 0;
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
	case '0' : glo->panneau.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau.toggle_vis( 0, 1 ); break;
	// l'option offscreen drawpad
	case 'o' : glo->panneau.offscreen_flag = 1; break;
	case 'n' : glo->panneau.offscreen_flag = 0; break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		{
		glo->panneau.dump();
		fflush(stdout);
		} break;
	}
}

/** ============================ l'application ==================== */

#define QBUF 		(1<<15)		// taille de buffer
#define BUFMASK 	(QBUF-1)

void glostru::process()
{
// donnees pour les tests
static float Xbuf[QBUF];
static float Ybuf[QBUF];
double omega1 = 2.0 * M_PI / 100.0;
for	( int i = 0; i < QBUF; ++i )
	{
	Xbuf[i] = 0.012  * (float)( i % 100 );
	Ybuf[i] = (float)sin( omega1 * (float)i ); 
	}

// layout jluplot
gstrip * curbande;
layer_f * curcour;

// marge pour les textes
// panneau.mx = 60;
// panneau.offscreen_flag = 0;

// creer le strip pour les waves
curbande = new gstrip;
panneau.add_strip( curbande );

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->optretX = 1;

// creer un layer
curcour = new layer_f;
curbande->add_layer( curcour );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = string("ValX");
curcour->fgcolor.dR = 0.75;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;

// creer un layer
curcour = new layer_f;
curbande->add_layer( curcour );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = string("ValY");
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.8;

// connexion layout - data
curcour = (layer_f *)panneau.bandes[0]->courbes[0];
curcour->V = Xbuf;
curcour->qu = QBUF;
curcour->scan();	// alors on peut faire un scan

curcour = (layer_f *)panneau.bandes[0]->courbes[1];
curcour->V = Ybuf;
curcour->qu = QBUF;
curcour->scan();	// alors on peut faire un scan

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

/* simple bouton */
curwidg = gtk_button_new_with_label (" Run ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bpau = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau.zoombar = &glo->zbar;
glo->panneau.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau;

gtk_widget_show_all( glo->wmain );

glo->panneau.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau.key_callback_register( key_call_back, (void *)glo );

glo->process();

// forcer un full initial pour que tous les coeffs de transformations soient a jour
glo->panneau.full_valid = 0;
// refaire un configure car celui appele par GTK est arrive trop tot
glo->panneau.configure();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);

gtk_main();


printf("closing\n");
return(0);
}


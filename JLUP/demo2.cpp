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

#include "layer_f_fifo.h"
#include "layer_f_param.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "demo2.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
static unsigned int ticks = 0;
ticks++;

if	( glo->running )
	{
	// acquisition de donnees en temps reel (simule ;-)
	if	( ( ticks % 3 ) == 0 )
		glo->gen_data_point();
	// cadrages avec exemples de moderations
	if	( ( ticks % 8 ) == 0 )
		if	( ( glo->option_tabs == 0 ) || ( gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->nmain) ) == 0 ) )
			{		// strip du fifo en scroll continu
			layer_f_fifo * lelay0 = (layer_f_fifo *)glo->panneau1.bandes[0]->courbes[0];
			layer_f_fifo * lelay1 = (layer_f_fifo *)glo->panneau1.bandes[0]->courbes[1];
			// cadrage vertical
			lelay0->scan();
			lelay1->scan();
			glo->panneau1.bandes[0]->fullN();
			// cadrage horizontal, fenetre de largeur constante Uspan
			double U0, U1;
			U1 = (double)lelay0->Ue + 0.05 * glo->Uspan;
			U0 = U1 - glo->Uspan;
			glo->panneau1.zoomM( U0, U1 );
			glo->panneau1.force_repaint = 1;
			}
	if	( ( ticks % 32 ) == 0 )
		if	( ( glo->option_tabs == 0 ) || ( gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->nmain) ) == 1 ) )
			{		// strip XY
			layer_f_param * lelay2 = (layer_f_param *)glo->panneau2.bandes[0]->courbes[0];
			lelay2->scan();
			glo->panneau2.fullMN(); glo->panneau2.force_repaint = 1;
			}
	}

// moderateur de drawing
// N.B. les pages du notebook sont toujours toutes "visibles" au sens de gtk_widget_get_visible()
// les pages cachees sont non realized au sens de gtk_widget_get_realized() jusqu'a etre affichees
// une premiere fois. Ceci est detecte par GDK_IS_DRAWABLE(larea->window) dans gluplot.
// Le test gtk_notebook_get_current_page() est la meilleure maniere de moderer l'activite. 
if	( 
	( glo->panneau1.force_repaint ) &&
	( ( glo->option_tabs == 0 ) || ( gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->nmain) ) == 0 ) )
	) glo->panneau1.paint();

if	(
	( glo->panneau2.force_repaint ) &&
	( ( glo->option_tabs == 0 ) || ( gtk_notebook_get_current_page( GTK_NOTEBOOK(glo->nmain) ) == 1 ) )
	) glo->panneau2.paint();

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
glo->running ^= 1;
if	( glo->running == 0 )
	{
	glo->panneau1.fullM();	// pour que la zoombar soit utilisable
	glo->panneau1.force_repaint = 1;
	glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
	}
}

void clear_call( GtkWidget *widget, glostru * glo )
{
glo->gen_data_point( 1 );
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back1( double M, double N, void * vglo )
{
printf("clic M N %g %g\n", M, N );
// glostru * glo = (glostru *)vglo;
}

void key_call_back1( int v, void * vglo )
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
		glo->panneau1.dump(); fflush(stdout);
		break;
	// demo modpop3
	case 'k' :
		{
		char tbuf[32];
		snprintf( tbuf, sizeof(tbuf), "%g", glo->k );
		modpop_entry( "K setting", "rapport des frequences", tbuf, sizeof(tbuf), GTK_WINDOW(glo->wmain) );
		glo->k = strtod( tbuf, NULL );
		} break;
	//
	case 't' :
		glo->panneau1.bandes[0]->subtk *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'U' :
		glo->Uspan *= 2;
		break;
	case 'u' :
		glo->Uspan *= 0.5;
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo2.1.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot FIFO" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
	}
}

void key_call_back2( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau2.dump(); fflush(stdout);
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo2.2.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot XY" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau2.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ l'application ==================== */


void glostru::gen_data_point( int razflag )
{
static double phi = 0.0;
static double amp = 0.1;
static unsigned i = 0;
layer_f_fifo * lelay0 = (layer_f_fifo *)panneau1.bandes[0]->courbes[0];
layer_f_fifo * lelay1 = (layer_f_fifo *)panneau1.bandes[0]->courbes[1];
layer_f_param * lelay2 = (layer_f_param *)panneau2.bandes[0]->courbes[0];
double X = amp * cos( phi );
double Y = amp * sin( k * phi );

// action sur le graphe fifo
lelay0->push(X);
lelay1->push(Y);

// action sur le graphe XY
if	( razflag )
	{
	i = 0;
	lelay2->qu = 0;
	}
else	{
	Xbuf[i&(QBUF-1)] = X;
	Ybuf[i&(QBUF-1)] = Y;
	i++;
	if	( i <= QBUF )
		lelay2->qu = i;
	}

phi += 0.02;
amp *= 1.0002;
}

void glostru::layout1()
{
// panneau1.offscreen_flag = 0;

// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 1;

// creer un layer
layer_f_fifo * curcour;
curcour = new layer_f_fifo(12);
curbande->add_layer( curcour, "ValX" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );
curcour->style = 1;

// creer un layer
curcour = new layer_f_fifo(12);
curbande->add_layer( curcour, "ValY" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
// sans objet pour layer_f_fifo : il possede les data
}

void glostru::layout2()
{
// layout jluplot pour panneau2
panneau2.offscreen_flag = 0;

// creer le strip
gstrip * curbande;
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
curcour2->qu = 0;
}


/** ============================ main, quoi ======================= */

int main( int argc, char *argv[] )
{
glostru * glo = &theglo;
GtkWidget *curwidg;

gtk_init(&argc,&argv);
setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

cli_parse * clip = new cli_parse( argc, (const char **)argv, "" );
if	( clip->get('N') )
	glo->option_tabs = 1;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "JAW");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

// creer boite verticale pour : en haut plot panels, en bas boutons et entries
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

// creer boite verticale pour panel X(t) Y(t) et sa zoombar
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
// gtk_paned_pack1( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->vpan1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, TRUE, TRUE, 0 );
glo->darea1 = curwidg;

/* creer une drawing area pour la zoombar */
curwidg = gtk_drawing_area_new();
glo->zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, FALSE, FALSE, 0 );
glo->zarea1 = curwidg;

/* creer une drawing area pour panel Y(X) */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
// gtk_paned_pack2( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->darea2 = curwidg;

if	( glo->option_tabs )
	{		// layout type notebook
	curwidg = gtk_notebook_new();
	gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
	gtk_widget_set_size_request( curwidg, 800, 700 );
	glo->nmain = curwidg;
	// premier onglet //
	gtk_notebook_append_page( GTK_NOTEBOOK( glo->nmain), glo->vpan1, gtk_label_new("courbes X(t), Y(t)") );
	// second onglet //
	gtk_notebook_append_page( GTK_NOTEBOOK( glo->nmain), glo->darea2, gtk_label_new("courbe Y(X)") );
	}
else	{		// paire verticale "paned" pour : en haut panel X(t) Y(t) avec zoombar, en bas panel Y(X)
	curwidg = gtk_vpaned_new ();
	gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
	// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
	gtk_widget_set_size_request( curwidg, 800, 700 );
	glo->vpans = curwidg;
	// y placer les deux panels
	gtk_paned_pack1( GTK_PANED(glo->vpans), glo->vpan1, TRUE, FALSE ); // resizable, not shrinkable
	gtk_paned_pack2( GTK_PANED(glo->vpans), glo->darea2, TRUE, FALSE ); // resizable, not shrinkable
	}

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Run/Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Restart ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( clear_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->braz = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;
// connecter les callbacks events --> appli
glo->panneau1.clic_callback_register( clic_call_back1, (void *)glo );
glo->panneau1.key_callback_register( key_call_back1, (void *)glo );
glo->panneau2.key_callback_register( key_call_back2, (void *)glo );

glo->panneau1.events_connect( GTK_DRAWING_AREA( glo->darea1 ) );
glo->panneau2.events_connect( GTK_DRAWING_AREA( glo->darea2 ) );

glo->layout1();

gtk_widget_show_all( glo->wmain );

glo->layout2();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);

gtk_main();


printf("closing\n");
return(0);
}


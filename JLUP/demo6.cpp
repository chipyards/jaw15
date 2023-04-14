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

#include "../modpop3.h"
#include "demo6.h"

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

void pdf_dialogs( glostru * glo, int fast )
{
if	( fast )
	{
	char fnam[32], capt[128];
	snprintf( fnam, sizeof(fnam), "demo6.pdf" );
	modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
	snprintf( capt, sizeof(capt), "Bode rules !" );
	modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
	glo->panneau1.pdfplot( fnam, capt );
	}
else	glo->panneau1.pdf_modal_layout( glo->wmain );
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
	case GDK_KEY_KP_2 :
	case '2' : glo->panneau1.toggle_vis( 1, 0 ); break;
	case GDK_KEY_KP_3 :
	case '3' : glo->panneau1.toggle_vis( 1, 1 ); break;
	// l'option offscreen drawpad
	case 'o' : glo->panneau1.offscreen_flag = 1; break;
	case 'n' : glo->panneau1.offscreen_flag = 0; break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		{
		glo->panneau1.dump();
		fflush(stdout);
		} break;
	//
	case 't' :
		glo->panneau1.qtkx *= 1.5;
		glo->panneau1.bandes[0]->qtky *= 2.0;
		glo->panneau1.bandes[1]->qtky *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'P' :
		pdf_dialogs( glo, 1 );
		break;
	}
}

/** ============================ context menus ======================= */

// call backs
static void pdf_export_0( GtkWidget *widget, glostru * glo )
{ pdf_dialogs( glo, 0 ); }
static void pdf_export_1( GtkWidget *widget, glostru * glo )
{ pdf_dialogs( glo, 1 ); }

static void offscreen_opt( GtkWidget *widget, glostru * glo )
{
glo->panneau1.offscreen_flag = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) );
glo->panneau1.dump(); fflush(stdout);
}

// enrichissement du menu global du panel
void enrich_global_menu( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;

curmenu = glo->panneau1.gmenu;    // Don't need to show menus

curitem = gtk_separator_menu_item_new();	// separateur indispensable
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_check_menu_item_new_with_label( "offscreen drawing" );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), false );
g_signal_connect( G_OBJECT( curitem ), "toggled",
		  G_CALLBACK( offscreen_opt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("PDF export (file chooser)");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( pdf_export_0 ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("PDF export (fast)");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( pdf_export_1 ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

}

/** ============================ l'application ==================== */

//
// calcul module et argument d'un point de la fonction de transfert
// H(s) = 1 / ( as2 + bs +1 ) du filtre LP du 2nd ordre, avec s = jw, w = frequence en rad/s
//
static void HLP( double w, double a, double b, double * mod, double * arg )
{
// on realifie le denominateur en multipliant haut et bas par le conjugue du denominateur
double Nr, Ni;	// partie reelle et imaginaire du (nouveau) numerateur
double D;	// denominateur realifie
Nr = 1 - ( w * w * a );
Ni = -w * b;
D = ( Nr * Nr ) + ( Ni * Ni );
*mod = 1.0 / sqrt( D );
*arg = atan2( Ni, Nr );
}

//
// calcul module et argument d'un point de la fonction de transfert
// H(s) = bs / ( as2 + bs +1 ) du filtre BP du 2nd ordre, avec s = jw, w = frequence en rad/s
//
static void HBP( double w, double a, double b, double * mod, double * arg )
{
// on realifie le denominateur en multipliant haut et bas par le conjugue du denominateur
double Nr, Ni;	// partie reelle et imaginaire du (nouveau) numerateur
double D;	// denominateur realifie
Nr = 1 - ( w * w * a );
Ni = -w * b;
D = ( Nr * Nr ) + ( Ni * Ni );
*mod = ( w * b ) / sqrt( D );
*arg = atan2( Ni, Nr ) + ( M_PI / 2.0 );
}

// plot de 2 filtres
//	fc, Q : param des filtres
//	f0, f1 : plage de frequences a balayer, ppd = points-per-decade
//	bodeplots[4] : resultats : module et argument pour chacun des 2 filtres
// retour : nombre de points par courbe 
int glostru::gen_data()
{
// coefficients pour les filtres
double a, b, wc;
wc = fc * 2 * M_PI;
a = 1.0 / ( wc * wc );
b = 1.0 / ( Q * wc );
// coeff de progression des frequences
double kw = pow( 10, 1.0 / ppd );
// allocation memoire
qsamples = log10( f1 / f0 ) * ppd;
bodeplots[0] = (double *)malloc( qsamples * sizeof(double) );
if	( bodeplots[0] == NULL )
	return 0;
bodeplots[1] = (double *)malloc( qsamples * sizeof(double) );
if	( bodeplots[1] == NULL )
	return 0;
bodeplots[2] = (double *)malloc( qsamples * sizeof(double) );
if	( bodeplots[2] == NULL )
	return 0;
bodeplots[3] = (double *)malloc( qsamples * sizeof(double) );
if	( bodeplots[3] == NULL )
	return 0;
// calculs des modules et arguments
double w = f0 * 2 * M_PI;
for	( int i = 0; i < qsamples; i++ )
	{
	HLP( w, a, b, bodeplots[0] + i, bodeplots[1] + i );
	HBP( w, a, b, bodeplots[2] + i, bodeplots[3] + i );
	printf("m=%d, f=%g\n", i, w / ( 2 * M_PI ) );
	w *= kw;
	}
return qsamples;
}

void glostru::process()
{

// layout panneau1

// marge pour les textes
// panneau1.mx = 60;

panneau1.offscreen_flag = 0;

// creer le strip pour les modules ----------
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "dB";
curbande->optX = 1;

// creer un layer
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "LP" );
// configurer le layer
curcour->fgcolor.set( 0.75, 0.0, 0.0 );
curcour->style = 2;
curcour->k0dB = 1.0;		// 1.0 <==> 0dB
curcour->Vfloor = 0.001;	// -60dB

// creer un layer
curcour = new layer_u<double>;
curbande->add_layer( curcour, "BP" );
// configurer le layer
curcour->fgcolor.set( 0.0, 0.0, 0.8 );
curcour->style = 2;
curcour->k0dB = Q;		// 1/Q <==> 0dB to align the peak with LP
curcour->Vfloor = 0.001;	// -60dB

// creer le strip pour les phases ---------
curbande = new gstrip;
panneau1.add_strip( curbande );
curbande->kr = M_PI/180.0;	// echelle en degres vs data en radian

// configurer le strip
curbande->bgcolor.set( 0.90, 1.0, 0.95 );
curbande->Ylabel = "deg";
curbande->optX = 1;

// creer un layer
curcour = new layer_u<double>;
curbande->add_layer( curcour, "LP" );
// configurer le layer
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// creer un layer
curcour = new layer_u<double>;
curbande->add_layer( curcour, "BP" );
// configurer le layer
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// configurer le panel pour l'echelle log horizontale (frequences)
// cette fonction active l'echelle log via optLog10
// et ajuste la tranformation Q <--> M pour que q soit log10 de f
// m = indice de chaque samples dont la f est en progression exponentielle
// N.B. dans cet exemple f1 est au dela du dernier sample (d'indice qsamples-1)
// mais c'est bien la frequence qui correspond a l'indice qsamples
panneau1.logscale_helper( f0, f1, double(qsamples) );

// connexion layout - data
curcour = (layer_u<double> *)panneau1.bandes[0]->courbes[0];
curcour->V = bodeplots[0];
curcour->qu = qsamples;
curcour->scan();	// alors on peut faire un scan

curcour = (layer_u<double> *)panneau1.bandes[0]->courbes[1];
curcour->V = bodeplots[2];
curcour->qu = qsamples;
curcour->scan();	// alors on peut faire un scan

curcour = (layer_u<double> *)panneau1.bandes[1]->courbes[0];
curcour->V = bodeplots[1];
curcour->qu = qsamples;
curcour->scan();	// alors on peut faire un scan

curcour = (layer_u<double> *)panneau1.bandes[1]->courbes[1];
curcour->V = bodeplots[3];
curcour->qu = qsamples;
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

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;

gtk_widget_show_all( glo->wmain );

glo->panneau1.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau1.key_callback_register( key_call_back, (void *)glo );

if	( glo->gen_data() == 0 )
	gasp("erreur malloc");
printf("generated %d samples\n", glo->qsamples ); fflush(stdout);
glo->process();

// enrichissement du menu global du panel
enrich_global_menu( glo );

glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

fflush(stdout);

gtk_main();

g_source_remove( glo->idle_id );

return(0);
}

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
#include "../cli_parse.h"
#include "demo4.h"

#include <profileapi.h>		// pour chronometrage

#define PAINT_PROFILER

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
// moderateur de drawing
if	( glo->panneau1.force_repaint )
	{
	#ifdef PAINT_PROFILER
	LARGE_INTEGER freq, start, stop;
	QueryPerformanceFrequency(&freq);	// en Hz; t.q. 10000000
	double fkHz = ((double)freq.QuadPart) / 1000.0;
	// printf("perf timer freq = %g kHz\n", fkHz ); fflush(stdout);
	QueryPerformanceCounter(&start);
	glo->panneau1.paint();
	QueryPerformanceCounter(&stop);
	double dur = (double)(start.QuadPart - stop.QuadPart);
	printf("paint %g ms\n", dur / fkHz ); fflush(stdout);
	#else
	glo->panneau1.paint();
	#endif
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
glo->panneau1.pdf_modal_layout( glo->wmain );
}

void pause_call( GtkWidget *widget, glostru * glo )
{
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
	case '2' : glo->panneau1.toggle_vis( 0, 2 ); break;
	case GDK_KEY_KP_3 :
	case '3' : glo->panneau1.toggle_vis( 1, 0 ); break;
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
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'P' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo4.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "C'est imposant ma soeur" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ context menus ======================= */

// call backs
static void pdf_export( GtkWidget *widget, glostru * glo )
{
glo->panneau1.pdf_modal_layout( glo->wmain );
}

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
		  G_CALLBACK( pdf_export ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

}

/** ============================ l'application ==================== */

void glostru::gen_data()
{
// allocation
Fbuf = (float *)malloc( qbuf * sizeof(float) );
Sbuf = (short *)malloc( qbuf * sizeof(short) );
Ubuf = (unsigned short *)malloc( qbuf * sizeof(unsigned short) );
if	( ( Fbuf == NULL ) || ( Fbuf == NULL ) || ( Fbuf == NULL ) )
	gasp("pb malloc data buffers");

// donnees pour les tests
double omega1 = 2.0 * M_PI / 200.0;
for	( unsigned int i = 0; i < qbuf; ++i )
	{
	Fbuf[i] = cos( omega1 * (float)i ); 
	Sbuf[i] = (short)( Fbuf[i] * 32767.0 );
	Ubuf[i] = Sbuf[i] + 32767;
	}

}

void glostru::process()
{
gen_data();

// layout panneau1

// marge pour les textes
// panneau1.mx = 60;

panneau1.offscreen_flag = 0;

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
curbande->add_layer( curcour, "float" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );
curcour->ecostroke = ecostroke;

// curbande->add_layer( curcour );

// creer un layer
layer_u<short> * curcour2;
curcour2 = new layer_u<short>;
curbande->add_layer( curcour2, "s16" );

// configurer le layer
curcour2->set_km( 1.0 );		// sets APRES add_layer
curcour2->set_m0( 0.0 );
curcour2->set_kn( 32767.0 );
curcour2->set_n0( 0.0 );
curcour2->fgcolor.set( 0.0, 0.0, 0.8 );
curcour2->ecostroke = ecostroke;

// creer un layer
layer_u<unsigned short> * curcour3;
curcour3 = new layer_u<unsigned short>;
curbande->add_layer( curcour3, "u16" );

// configurer le layer
curcour3->set_km( 1.0 );		// sets APRES add_layer
curcour3->set_m0( 0.0 );
curcour3->set_kn( 32767.0 );
curcour3->set_n0( 0.0 );
curcour3->fgcolor.set( 0.75, 0.0, 0.0 );
curcour3->ecostroke = ecostroke;
curcour3->style = 1;

// connexion layout - data
curcour = (layer_u<float> *)panneau1.bandes[0]->courbes[0];
curcour->V = Fbuf;
curcour->qu = qbuf;
curcour->scan();	// alors on peut faire un scan

curcour2 = (layer_u<short> *)panneau1.bandes[0]->courbes[1];
curcour2->V = Sbuf;
curcour2->qu = qbuf;
curcour2->scan();	// alors on peut faire un scan

curcour3 = (layer_u<unsigned short> *)panneau1.bandes[0]->courbes[2];
curcour3->V = Ubuf;
curcour3->qu = qbuf;
curcour3->scan();	// alors on peut faire un scan


// creer le strip
curbande = new gstrip;
panneau1.add_strip( curbande );

panneau1.pdf_DPI = 100;	// defaut est 72

// configurer le strip
curbande->bgcolor.set( 1.0, 0.95, 0.85 );
curbande->Ylabel = "dB";
curbande->optX = 0;
curbande->subtk = 10;

// creer un layer
// layer_u<unsigned short> * curcour3;
curcour3 = new layer_u<unsigned short>;
curbande->add_layer( curcour3, "u16/dB");

// configurer le layer
curcour3->set_km( 1.0 );		// sets APRES add_layer
curcour3->set_m0( 0.0 );
curcour3->set_kn( 1.0 );
curcour3->set_n0( 0.0 );
curcour3->fgcolor.set( 0.75, 0.0, 0.0 );
curcour3->ecostroke = ecostroke;
curcour3->style = 2;
curcour3->k0dB = 1.0/32767.0;	// 32767.0 <==> 0dB
curcour3->Vfloor = 0.003162;	// -50dB

// connexion layout - data
curcour3 = (layer_u<unsigned short> *)panneau1.bandes[1]->courbes[0];
curcour3->V = Ubuf;
curcour3->qu = qbuf;
curcour3->scan();	// alors on peut faire un scan

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

cli_parse * clip = new cli_parse( argc, (const char **)argv, "qe" );
const char * zearg = clip->get('q');
if	( zearg )
	glo->qbuf = atoi( zearg );
if	( glo->qbuf <= 1 )
	glo->qbuf = 2;

zearg = clip->get('e');
if	( zearg )
	glo->ecostroke = atoi( zearg );
if	( glo->ecostroke < 1 )
	glo->ecostroke = 1;

printf("qbuf = %u, ecostroke = %u\n", glo->qbuf, glo->ecostroke ); fflush(stdout);

glo->process();

// enrichissement du menu global du panel
enrich_global_menu( glo );

glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

fflush(stdout);

gtk_main();

g_source_remove( glo->idle_id );

/* experience de liberation de memoire (non necessaire ici) */
printf("begin deletion\n"); fflush(stdout);
unsigned int ib, ic;
for	( ib = 0; ib < glo->panneau1.bandes.size(); ++ib )
	{
	for	( ic = 0; ic < glo->panneau1.bandes[ib]->courbes.size(); ++ic )
		delete glo->panneau1.bandes[ib]->courbes[ic];
	delete glo->panneau1.bandes[ib];
	}
glo->panneau1.bandes.clear();
glo->panneau1.dump();
printf("panneau1 : layers and strips deleted\n");
//*/
return(0);
}



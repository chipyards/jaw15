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

#include "layer_lod.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "demo3.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
static unsigned int ticks = 0;
ticks++;

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

// callbacks de widgets
void run_call( GtkWidget *widget, glostru * glo )
{
}

void clear_call( GtkWidget *widget, glostru * glo )
{
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
	case 'i' :
		int i0, i1;
		i0 = ((layer_lod<float> *)glo->panneau1.bandes[0]->courbes[0])->ilod;
		i1 = ((layer_lod<short> *)glo->panneau1.bandes[0]->courbes[1])->ilod;
		printf("ilod: %d %d\n", i0, i1 ); fflush(stdout);
		break;
	//
	case 't' :
		glo->panneau1.bandes[0]->subtk *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo3.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot FIFO" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ l'application ==================== */


void glostru::process()
{
int retval;
// production de donnees
printf("generating %u samples, please wait...\n", QBUF ); fflush(stdout);
double val;
double w1 = 6.28 / 100.0;
double w2 = 6.28 / 2000.0;
double w3 = 6.28 / 40000.0;
double w4 = 6.28 / 800000.0;
for	( unsigned int i = 0; i < QBUF; ++i )
	{
	val =  ( ( 0.4 * sin( w1 * i ) + 0.4 * sin( w2 * i ) ) * sin( w3 * i ) ) + 0.1 * sin( w4 * i );
	Fbuf[i] = (float)val;
	Sbuf[i] = (short)(val * 32767.0);
	}
printf("%u samples generated\n", QBUF ); fflush(stdout);

// layout jluplot

// marge pour les textes
// panneau1.mx = 60;
// panneau1.offscreen_flag = 0;

panneau1.pdf_DPI = 100;	// defaut est 72

// creer le strip pour les waves
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 1;

// creer un layer
layer_lod<float> * curcour;
curcour = new layer_lod<float>;
curbande->add_layer( curcour, "f32" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour->V = Fbuf;
curcour->qu = QBUF;
retval = curcour->make_lods( k, k, 2000 );
if	( retval )
	{ gasp("echec make_lods err %d", retval ); }
printf("%d f lods made\n", curcour->lods.size() ); fflush(stdout);

// creer un layer
layer_lod<short> * curcour2;
curcour2 = new layer_lod<short>;
curbande->add_layer( curcour2, "s16" );

// configurer le layer
curcour2->set_km( 1.0 );
curcour2->set_m0( 0.0 );
curcour2->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour2->set_n0( 0.0 );
curcour2->fgcolor.set( 0.0, 0.0, 0.75 );

// connexion layout - data
curcour2->V = Sbuf;
curcour2->qu = QBUF;
retval = curcour2->make_lods( k, k, 2000 );
if	( retval )
	{ gasp("echec make_lods err %d", retval ); }
printf("%d s16 lods made\n", curcour2->lods.size() ); fflush(stdout);
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

// creer boite verticale pour : en haut plot panel, en bas boutons et entries
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 400 );
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
curwidg = gtk_button_new_with_label (" R ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" C ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( clear_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->braz = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;

gtk_widget_show_all( glo->wmain );

glo->panneau1.clic_callback_register( clic_call_back1, (void *)glo );
glo->panneau1.key_callback_register( key_call_back1, (void *)glo );

cli_parse * clip = new cli_parse( argc, (const char **)argv, "k" );
const char * zearg = clip->get('k');
glo->k = 4;
if	( zearg )
	glo->k = atoi( zearg );
if	( glo->k <= 1 )
	glo->k = 2;
printf("coeff de decimation LODs : k = %d\n", glo->k ); fflush(stdout);

glo->process();

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



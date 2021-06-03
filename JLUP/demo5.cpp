/**
	Experiences sur le "filtre passe bas ideal" base sur le sinus cardinal
	et l'application au resampling a bande limitee (interpolation et decimation)

- theoreme : le filtre ideal est un filtre dont la reponse impulsionnelle finie est la fonction sinc( k * i )
  avec k = 2 * pi * (Fc/Fsamp)
         = pi / pispan, avec pispan = 1 / ( Fc / ( Fsamp / 2 ) ) = "intervalle entre deux zeros de sinc"

- cas limite : pispan = 1 <==> Fc = Fsamp/2 = limite de Nyquist
  Un tel filtre ne sert a rien pour filtrer un signal incident, mais il a un sens pour faire de l'interpolation
  en limitant le bruit d'interpolation a la limite de Nyquist.
  En pratique on prendra toujours pispan > 1

- filtre imparfait : on se limite a une reponse impulsionnelle finie symetrique, avec eventuellement un fenetrage

EXPERIENCE 1 : FT de la reponse impulsionnelle finie = reponse frequentielle

- amplitude : on doit diviser le module de la transformee par pispan pour reponse DC a 0dB
  (l'integrale de sinc correspond a un rectangle de largeur pispan)

- frequence : dans le cas ideal la transition est attendue à l'abcisse a = fftsize / ( 2 * pispan ) 
  (en effet Fc = a * (Fsamp/fftsize) = Fsamp * 1 / ( 2 * pispan ) )
  On peut donc demander a jluplot une echelle en frequence relative à l'ideal

- incidence de la longueur de la reponse impulsionnelle qfir = pispan * qpis (@ pispan = 777) (sans fenetre)

	qpis		stopband 1st zero	niveau 1ere bosse
	5		1.24			0.106
	6		1.21			0.0751
	10		1.125			0.0803
	10.5		1.122			0.0897
	11		1.112			0.098
	20		1.0625			0.085
	30		1.0422			0.086
	31		1.0407			0.093
	40		1.0318			0.087
  conclusion : l'ecart entre Fc et le premier zero est inversement proportionnel a qpis.
  de plus la transition passe par Fc toujours pres de 0.5 (-6dB)
  la largeur totale de la transition (de 1.0 a 0) est 25% pour qpis=10, 6.3% pour qpis=40
  La premiere bosse depend des conditions aux bords, toujours pire avec qpis impair

- fenetrage  (@ pispan = 777)

	qpis   fenetre		stopband 1st zero	niveau 1ere bosse
	10	rect		1.125			-21.9
	10	hann		1.336			-44.0 dB
	10	hamming		1.346			-51.7
	10	blackman	1.56 @ -75 dB		-75.6
	10	b-harris	1.66 @ -75 dB		n.a.

	20	rect		1.063			-21.4
	20	hann		1.167			-44.0 dB
	20	hamming		1.175			-52.7
	20	blackman	1.28 @ -75 dB		-75.4
	20	b-harris	1.34 @ -75 dB		n.a.

	30	rect		1.042			-21.3
	30	hann		1.113			-44
	30	hamming		1.117			-53.1
	30	blackman	1.187 @ -75 dB		-75.3
	30	b-harris	1.228 @ -75 dB		n.a.

La transition intercepte toujours Fc à -6dB !
Le niveau DC n'est pas affecté par la fenetre, aucune correction n'est requise !
Le premier zéro est inexistant avec Blackman et Blackman-Harris, on le remplace par un seuil arbitraire
L'écart entre Fc et le premier zero est toujours inversement proportionnel a qpis,
mais il est tres augmente par la fenetre (presque triple avec Hamming).


*/
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
#include <fftw3.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "gluplot.h"

#include "layer_u.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "demo5.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{

// moderateur de drawing
// N.B. les pages du notebook sont toujours toutes "visibles" au sens de gtk_widget_get_visible()
// les pages cachees sont non realized au sens de gtk_widget_get_realized() jusqu'a etre affichees
// une premiere fois. Ceci est detecte par GDK_IS_DRAWABLE(larea->window) dans gluplot.
// Le test gtk_notebook_get_current_page() est la meilleure maniere de moderer l'activite. 
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

static double mysinc( double x )
{
if	( fabs(x) < 1e-5 )
	return 1.0;
return ( sin(x) / x );
}

// fonction imitee de spectro::window_precalc( unsigned int fft_size )
void glostru::window_precalc( double * window, unsigned int size )
{
double a0, a1, a2, a3;
switch	( window_type )		// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
	{
	case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
	case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
	case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
	case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
	default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
	}
				// on doit diviser par size ( non par (size-1) ), alors 
double m = M_PI / size;		// le dernier echantillon n'est pas egal au premier mais au second, c'est normal
for	( unsigned int i = 0; i < size; ++i )
	{
        window[i] = a0
		- a1 * cos( 2 * m * i )
		+ a2 * cos( 4 * m * i )
		- a3 * cos( 6 * m * i );
	}
}

int glostru::generate()
{
// allocation
if	( Cbuf == NULL )
	Cbuf = (double *)fftw_malloc( qbuf * sizeof(double) );
if	( Sbuf == NULL )
	Sbuf = (double *)fftw_malloc( qbuf * sizeof(double) );
if	( Tbuf == NULL )
	Tbuf = (double *)fftw_malloc( qbuf * sizeof(double) );
if	( ( Cbuf == NULL ) || ( Sbuf == NULL ) || ( Tbuf == NULL ) )
	return -1;
// preparer le plan de travail de fftw3
if	( plan )
	fftw_destroy_plan(plan);
plan = fftw_plan_dft_r2c_1d( qbuf, Cbuf, (fftw_complex*)Tbuf, FFTW_ESTIMATE );
if	( plan == NULL )
	return -3;
// generation APRES le plan FFTW car celui-ci raze le buf d'entree
window_precalc( Cbuf, qfir );
double x;
double k = M_PI / pispan;
unsigned int i;

for	( i = 0; i < qfir; ++i )
	{
	x = k * ( double( (int)i - (int)(qfir/2) ) );
	Cbuf[i] *= mysinc( x );
	}
for	( i = qfir; i < qbuf; ++i )
	{
	Cbuf[i] = 0;
	}
// fft
fftw_execute( plan );
// calcul magnitudes sur place (Tbuf)
unsigned int a = 0;
// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
k = 1.0 / pispan;
for	( unsigned int j = 0; j <= qbuf/2; ++j )
	{
	Tbuf[j] = k * hypot( Tbuf[a], Tbuf[a+1] );
	a += 2;
	}

return 0;
}

void glostru::layout1()
{
panneau1.offscreen_flag = 0;

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
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "real" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour->V = Cbuf;
curcour->qu = qfir;
curcour->scan();	// alors on peut faire un scan

/*
curcour = new layer_u<double>;
curbande->add_layer( curcour, "real" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
curcour->V = Sbuf;
curcour->qu = qfir;
curcour->scan();	// alors on peut faire un scan
*/
}

void glostru::layout2()
{
// layout jluplot pour panneau2
panneau2.offscreen_flag = 0;
// normalisation d'echelle horizontale t.q. Fc theorique <==> 1.0
panneau2.kq = qbuf / ( 2 * pispan );
 
// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau2.add_strip( curbande );

// configurer le strip
curbande->Ylabel = "val";
curbande->optX = 1;

// creer un layer
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "real" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );
curcour->style = 2;

// connexion layout - data
curcour->V = Tbuf;
curcour->qu = floor( panneau2.kq * 8 ); // pour que le fullM (horiz.) se limite a 8 * Fc 
curcour->scan();	// alors on peut faire un scan

}


/** ============================ main, quoi ======================= */

int main( int argc, char *argv[] )
{
glostru * glo = &theglo;
GtkWidget *curwidg;

gtk_init(&argc,&argv);
setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

cli_parse * lepar = new cli_parse( argc, (const char **)argv, "LPZw" );
const char * val;
int qbuflog = 20;
double qpis = 6.0;
if	( ( val = lepar->get( 'L' ) ) )	qbuflog = atoi( val );		// log de fftsize
if	( ( val = lepar->get( 'P' ) ) )	glo->pispan = atoi( val );	// taille de PI en samples pour calcul sinc
if	( ( val = lepar->get( 'Z' ) ) )	qpis = strtod( val, NULL );	// qfir / pispan ( <--> "nombre de zeros")
if	( ( val = lepar->get( 'w' ) ) )	glo->window_type = atoi( val );	// 0 = rect, etc...
if	( ( qbuflog < 8 ) || ( glo->pispan < 8 ) || ( qpis < 2.0 ) )
	{ printf("invalid argument\n"); return -1; }
glo->qbuf = 1 << qbuflog;
glo->qfir = floor( qpis * double(glo->pispan) );

printf("fftsize = %u, fir span = %u i.e. %g times PI, windows %d\n", glo->qbuf, glo->qfir, qpis, glo->window_type ); fflush(stdout);

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
// option paned
	{		// paire verticale "paned" pour : en haut panel X(t) Y(t) avec zoombar, en bas panel Y(X)
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

int retval = glo->generate();
if	( retval )
	gasp(" erreur %d", retval );
glo->layout1();

gtk_widget_show_all( glo->wmain );

glo->layout2();

glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

fflush(stdout);

gtk_main();

g_source_remove( glo->idle_id );

return(0);
}


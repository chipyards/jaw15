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

// sliders
static void k_call( GtkAdjustment *adjustment, glostru * glo )
{
glo->spectrographize();		// ne fera la computation que si elle n'a pas encore ete faite
double k = adjustment->value;
if	( k > 0.5 )
	glo->pro.palettize( (unsigned int)(((double)glo->pro.Lspek.umax) / k ) );
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
}

static void recomp_call( GtkWidget *widget, glostru * glo )
{
printf(">>>> begin deletion of strips\n"); fflush(stdout);
glo->panneau.shrink(1);
printf(">>>> begin deletion of spectra\n"); fflush(stdout);
glo->pro.clean_spectros();
printf(">>>> end deletion\n"); fflush(stdout);
glo->spectrographize();
double k = glo->para.adjk->value;
if	( k > 0.5 )
	glo->pro.palettize( (unsigned int)(((double)glo->pro.Lspek.umax) / k ) );
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
glo->para.panneau.force_repaint = 1; glo->para.panneau.force_redraw = 1;

printf("gmenu fix: %d\n", glo->panneau.fix_gmenu() ); fflush(stdout);
}

static void load_call( GtkWidget *widget, glostru * glo )
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
gtk_widget_destroy (dialog);
}

static void reset_call( GtkWidget *widget, glostru * glo )
{
printf(">>>> begin deletion of waves\n"); fflush(stdout);
glo->panneau.reset();
glo->panneau.dump();

printf(">>>> begin deletion of aux plot\n"); fflush(stdout);
glo->para.panneau.reset();
glo->para.panneau.dump();

printf(">>>> begin deletion of spectra\n"); fflush(stdout);
glo->pro.clean_spectros();
printf(">>>> end deletion\n"); fflush(stdout);
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
}

static void layout_W_call( GtkWidget *widget, glostru * glo )
{
if	( glo->panneau.bandes.size() )
	return;
// preparer le layout pour wav L (et R si stereo)
glo->pro.prep_layout_W( &glo->panneau );
int retval = glo->pro.connect_layout_W( &glo->panneau );
if	( retval )
	gasp("echec connect layout, erreur %d", retval );
fflush(stdout);
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
glo->iplay = -1; glo->iplayp = glo->iplay0 = 0; glo->iplay1 = 2000000000;
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
printf("gmenu fix: %d\n", glo->panneau.fix_gmenu() ); fflush(stdout);
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
}


static void reset_S_call( GtkWidget *widget, glostru * glo )
{
printf(">>>> begin deletion of waves\n"); fflush(stdout);
glo->panneau.dump();
glo->panneau.shrink(1);
glo->panneau.dump();
printf(">>>> begin deletion of spectra\n"); fflush(stdout);
glo->pro.clean_spectros();
printf(">>>> end deletion\n"); fflush(stdout);
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
printf("gmenu fix: %d\n", glo->panneau.fix_gmenu() ); fflush(stdout);
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
}

static void compute_call( GtkWidget *widget, glostru * glo )
{
glo->spectrographize();
glo->panneau.force_repaint = 1; glo->panneau.force_redraw = 1;
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
printf("gmenu fix: %d\n", glo->panneau.fix_gmenu() ); fflush(stdout);
printf("gmenu check: %d\n", glo->panneau.check_gmenu() ); fflush(stdout);
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
// gtk_widget_set_size_request( curwidg, 640, 480 );
wmain = curwidg;

// notebook (collection de tabs)
// ATTENTION : des fonctions appelees par idle function ou events peuvent utiliser
// gtk_notebook_get_current_page() pour identifier la page visible.
// Si on change l'ordre des pages ces identifications devront etre reparametrees
curwidg = gtk_notebook_new();
gtk_container_add( GTK_CONTAINER( wmain ), curwidg );
nmain = curwidg;

// container pour les params de layout
//curwidg = gtk_vbox_new( FALSE, 10 );
//gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Multitrack") );
//vlay = curwidg;

// container pour le spectre
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Spectrum") );
vspe = curwidg;

curwidg = gtk_hbox_new( FALSE, 10 );
gtk_box_pack_start( GTK_BOX( vspe ), curwidg, TRUE, TRUE, 0 );
hspe = curwidg;

// container pour les parametres de spectre
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_box_pack_start( GTK_BOX( hspe ), curwidg, FALSE, FALSE, 0 );
vspp = curwidg;

// combo pour le type de fenetre FFT
// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
curwidg = gtk_label_new("FFT window");
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
curwidg = gtk_combo_box_text_new();
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "0 Rect" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "1 Hann" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "2 Hamming" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "3 Blackman" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "4 BlackmanHarris" );
gtk_combo_box_set_active( GTK_COMBO_BOX(curwidg), 1 );
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
cfwin = curwidg;

// combo pour fftsize
curwidg = gtk_label_new("FFT size");
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
curwidg = gtk_combo_box_text_new();
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "512" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "1024" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "2048" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "4096" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "8192" );
gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(curwidg), "16384" );
gtk_combo_box_set_active( GTK_COMBO_BOX(curwidg), 4 );
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
cfsiz = curwidg;

// entry pour fft stride
curwidg = gtk_label_new("FFT stride");
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 160, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "1024" );
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
estri = curwidg;

// spin button pour BPST bin_per_semi_tone
curwidg = gtk_label_new("Binxels Per SemiTone");
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
curwidg = gtk_spin_button_new_with_range( 2.0, 20.0, 1.0 );
gtk_spin_button_set_value( GTK_SPIN_BUTTON(curwidg), 10.0 );
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );
bbpst = curwidg;
// gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(widget) );

/* simples boutons */
curwidg = gtk_button_new_with_label ("recompute");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( recomp_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vspp ), curwidg, FALSE, FALSE, 0 );

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 720, 400 );
panneau.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( hspe ), curwidg, TRUE, TRUE, 0 );
sarea = curwidg;

// slider pour le gain spectrogramme
GtkAdjustment * curadj;
			//value,lower,upper,step_increment,page_increment,page_size);
curadj = GTK_ADJUSTMENT( gtk_adjustment_new( 1.0, 0.5, 10.0, 0.1, 1.0, 0 ));
g_signal_connect( curadj, "value_changed",
		  G_CALLBACK( k_call ), (gpointer)glo );
curwidg = gtk_hscale_new(curadj);
gtk_scale_set_digits( GTK_SCALE(curwidg), 1 );
gtk_box_pack_start( GTK_BOX(vspe), curwidg, FALSE, FALSE, 0 );
adjk = curadj;

// container pour les fichiers
curwidg = gtk_vbox_new( FALSE, 30 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("Files") );
vfil = curwidg;

/* simples boutons */
curwidg = gtk_button_new_with_label ("Load File");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( load_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

curwidg = gtk_button_new_with_label ("reset layout");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( reset_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

curwidg = gtk_button_new_with_label ("layout W");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( layout_W_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

curwidg = gtk_button_new_with_label ("reset spectro");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( reset_S_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

curwidg = gtk_button_new_with_label ("compute spectro");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( compute_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( vfil ), curwidg, FALSE, FALSE, 0 );

// container pour les I/O (audio et midi)
curwidg = gtk_vbox_new( FALSE, 10 );
gtk_notebook_append_page( GTK_NOTEBOOK( nmain ), curwidg, gtk_label_new("I/O ports") );
vpor = curwidg;


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

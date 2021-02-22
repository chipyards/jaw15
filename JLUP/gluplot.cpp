#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
// #include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// #include "modpop.h"

using namespace std;
#include <string>
#include <vector>

#include "jluplot.h"
#include "gluplot.h"

/** ======================= C-style callbacks ================================ */

// capture du focus pour les fonctionnement des bindkeys
static gboolean gpanel_enter( GtkWidget * widget, GdkEventCrossing * event, gpanel * p )
{
// printf("entered!\n");
gtk_widget_grab_focus( widget );
return FALSE;	// We leave a chance to others
}

// evenement bindkey
static gboolean gpanel_key( GtkWidget * widget, GdkEventKey * event, gpanel * p )
{
p->key( event );
return TRUE;	// We've handled the event, stop processing
}

static gboolean gpanel_click( GtkWidget * widget, GdkEventButton * event, gpanel * p )
{
p->clic( event );
return TRUE;	// We've handled the event, stop processing
}

static gboolean gpanel_motion( GtkWidget * widget, GdkEventMotion * event, gpanel * p )
{
p->motion( event );
return TRUE;	// We've handled it, stop processing
}

static gboolean gpanel_wheel( GtkWidget * widget, GdkEventScroll * event, gpanel * p )
{
p->wheel( event );
return TRUE;	// We've handled it, stop processing
}

static gboolean gpanel_expose( GtkWidget * widget, GdkEventExpose * event, gpanel * p )
{
// printf("expozed\n");
p->expose();
return FALSE;	// MAIS POURQUOI ???
}

static gboolean gpanel_configure( GtkWidget * widget, GdkEventConfigure * event, gpanel * p )
{
p->configure();
return TRUE;	// MAIS POURQUOI ???
}

/* callback pour la fenetre modal du plot PDF
   il faut intercepter le delete event pour que si l'utilisateur ferme la fenetre
   on revienne de la fonction pdf_modal_layout() sans engendrer de destroy signal.
   A cet effet ce callback doit rendre TRUE (ce que la fonction gtk_main_quit() ne fait pas).
 */
static gint gpanel_pdf_modal_delete( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_main_quit();
return (TRUE);
}

static void gpanel_pdf_ok_button( GtkWidget *widget, gpanel * p )
{
p->pdf_ok_call();
}

static void menu1_full( GtkWidget *widget, gpanel * p )
{
// printf("Full clic %08x\n", p->selected_strip );
if	( p->selected_strip & ( CLIC_MARGE_INF | CLIC_ZOOMBAR ) )
	{
	p->fullM();
	p->force_repaint = 1;
	// gtk_widget_queue_draw( p->widget );
	}
else if ( p->selected_strip & CLIC_MARGE_GAUCHE )
	{
	p->bandes.at(p->selected_strip & (~CLIC_MARGE))->fullN();
	p->force_repaint = 1;
	// gtk_widget_queue_draw( p->widget );
	}
}

static void menu1_zoomin( GtkWidget *widget, gpanel * p )
{
if	( p->selected_strip & ( CLIC_MARGE_INF | CLIC_ZOOMBAR ) )
	{
	p->zoomXbyK( 0.5 );
	}
else if ( p->selected_strip & CLIC_MARGE_GAUCHE )
	{
	strip * b = p->bandes.at(p->selected_strip & (~CLIC_MARGE));
	b->zoomYbyK( 0.5 );
	}
p->force_repaint = 1;
// gtk_widget_queue_draw( p->widget );
}

static void menu1_zoomout( GtkWidget *widget, gpanel * p )
{
if	( p->selected_strip & ( CLIC_MARGE_INF | CLIC_ZOOMBAR ) )
	{
	p->zoomXbyK( 2.0 );
	}
else if ( p->selected_strip & CLIC_MARGE_GAUCHE )
	{
	strip * b = p->bandes.at(p->selected_strip & (~CLIC_MARGE));
	b->zoomYbyK( 2.0 );
	}
p->force_repaint = 1;
// gtk_widget_queue_draw( p->widget );
}

static gboolean gzoombar_click( GtkWidget * widget, GdkEventButton * event, gzoombar * z )
{
z->clic( event );
return TRUE;	// We've handled the event, stop processing
}

static gboolean gzoombar_motion( GtkWidget * widget, GdkEventMotion * event, gzoombar * z )
{
z->motion( event );
return TRUE;	// We've handled it, stop processing
}

static gboolean gzoombar_wheel( GtkWidget * widget, GdkEventScroll * event, gzoombar * z )
{
z->wheel( event );
return TRUE;	// We've handled it, stop processing
}

static gboolean gzoombar_expose( GtkWidget * widget, GdkEventExpose * event, gzoombar * z )
{
// printf("expozed\n");
z->expose();
return FALSE;	// MAIS POURQUOI ???
}

static gboolean gzoombar_configure( GtkWidget * widget, GdkEventConfigure * event, gzoombar * z )
{
z->configure();
return TRUE;	// MAIS POURQUOI ???
}

void gzoombar_zoom( void * z, double k0, double k1 )
{
((gzoombar *)z)->zoom( k0, k1 );
}

/** ===================== gpanel initalization methods =============================== */

GtkWidget * gpanel::layout( int w, int h )
{
widget = gtk_drawing_area_new ();

// ajuster la drawing area aux dimensions voulues
gtk_widget_set_size_request( widget, w, h );

/* Drawing Area Signals  */

GTK_WIDGET_SET_FLAGS( widget, GTK_CAN_FOCUS );
g_signal_connect( widget, "expose_event", G_CALLBACK(gpanel_expose), this );
g_signal_connect( widget, "configure_event", G_CALLBACK(gpanel_configure), this );
g_signal_connect( widget, "button_press_event",   G_CALLBACK(gpanel_click), this );
g_signal_connect( widget, "button_release_event", G_CALLBACK(gpanel_click), this );
g_signal_connect( widget, "motion_notify_event", G_CALLBACK(gpanel_motion), this );
g_signal_connect( widget, "scroll_event", G_CALLBACK( gpanel_wheel ), this );
g_signal_connect( widget, "key_press_event", G_CALLBACK( gpanel_key ), this );
g_signal_connect( widget, "key_release_event", G_CALLBACK( gpanel_key ), this );
g_signal_connect( widget, "enter_notify_event", G_CALLBACK( gpanel_enter ), this );

// Ask to receive events the drawing area doesn't normally subscribe to
gtk_widget_set_events ( widget, gtk_widget_get_events(widget)
			| GDK_ENTER_NOTIFY_MASK
			| GDK_KEY_PRESS_MASK
			| GDK_KEY_RELEASE_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_SCROLL_MASK
			| GDK_POINTER_MOTION_MASK
//			| GDK_POINTER_MOTION_HINT_MASK
		      );
drag.mode = nil;
selected_key = 0;

menu1_x = mkmenu1("X AXIS");
menu1_y = mkmenu1("Y AXIS");
// on desactive ici le double buffer pour le controler plus finement
// avec gdk_window_begin_paint_region() et gdk_window_end_paint()
gtk_widget_set_double_buffered( widget, FALSE );

return widget;
}

void gpanel::clic_callback_register( void (*fon)(double,double,void*), void * data )
{
clic_call_back = fon; call_back_data = data;
}

void gpanel::key_callback_register( void (*fon)(int,void*), void * data )
{
key_call_back = fon; call_back_data = data;
}

// methode automatique, alloue ou re-alloue le pad s'il est trop petit
// prend soin aussi de son cairo persistant
void gpanel::drawpad_resize()
{
int w = fdx;
int h = fdy;
if	( drawpad )
	{
	int oldw, oldh;
	gdk_drawable_get_size( drawpad, &oldw, &oldh );
	if	( ( oldw < w ) || ( oldh < h ) )
		{
		g_object_unref( drawpad );
		drawpad = NULL;
		}
	}
if	( drawpad == NULL )
	{
	drawpad = gdk_pixmap_new( NULL, w, h, 24 );
	if	( drawpad )	printf("alloc offscreen drawable %d x %d Ok\n", w, h );
	else			printf("ERROR alloc offscreen drawable %d x %d\n", w, h );
	}
if	( offcai )
	cairo_destroy( offcai );
offcai = gdk_cairo_create( drawpad );
cairo_set_line_width( offcai, 0.5 );
}

/** ===================== gpanel drawing methods =============================== */

// copier le drawpad entier sur l'ecran (cai ne sert qu'en mode B2)
void gpanel::drawpad_compose( cairo_t * cai )
{
if	( drawab )
	{	// mode B1
	if	( gc == NULL )
		gc = gdk_gc_new( drawab );
	gdk_draw_drawable( drawab, gc, drawpad, 0, 0, 0, 0, fdx, fdy );
	// printf("drawn drawab !\n");
	}
else	{	// mode B2
	// copier le drawpad sur la drawing area, methode Cairo
	// origine Y est (encore) en haut du strip...
	gdk_cairo_set_source_pixmap( cai, drawpad, 0.0, 0.0 );
	// cairo_set_source_rgb( cai, 0.0, 0.6, 0.6 );
	cairo_rectangle( cai, 0.0, 0.0, double(fdx), double(fdy) );
	cairo_fill(cai);
	}
}

// copier une petite zone du drawpad sur l'ecran (cai ne sert qu'en mode B2)
void gpanel::cursor_zone_clean( cairo_t ** cair )
{
if	( drawab )
	{	// mode B1
	if	( gc == NULL )
		gc = gdk_gc_new( drawab );
	gdk_draw_drawable( drawab, gc, drawpad,
		mx-1+(int)floor(xdirty), 0,		// src XY
		mx-1+(int)floor(xdirty), 0,		// dest XY
		3, fdy );			// WH
	}
else	{	// mode B2
	// copier le drawpad sur la drawing area, methode Cairo
	// origine Y est (encore) en haut du strip...
	if	( *cair == NULL )
		*cair = gdk_cairo_create( widget->window );
	gdk_cairo_set_source_pixmap( *cair, drawpad, 0.0, 0.0 );
	// cairo_set_source_rgb( cai, 0.0, 0.6, 0.6 );
	cairo_rectangle( *cair, (double)(mx-1)+floor(xdirty), 0.0, 3.0, double(fdy) );
	cairo_fill(*cair);
	}
}

// cette methode automatique fait du dessin vectoriel sur le drawpad.
// elle agit en fonction des flags force_redraw au niveau panel et strip.
// eventuellement elle ne fait rien.
// typiquement force_redraw est mis en cas de zoom ou pan ou modif data
void gpanel::draw()
{
unsigned int ib, strip_redraws;
// faut-il un redraw partiel ?
if	( force_redraw == 0 )
	{
	strip_redraws = 0;
	for	( ib = 0; ib < bandes.size(); ib++ )
		if	( ( bandes.at(ib)->force_redraw ) && ( bandes.at(ib)->visible ) )
			++strip_redraws;
	}
// tracer sur le drawpad
if	( ( force_redraw ) || ( strip_redraws ) )
	{
	drawpad_resize();	// ne fait rien s'il n'y a pas de changement de taille
	// preparer font a l'avance
	cairo_select_font_face( offcai, "Courier", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size( offcai, 12.0 );
	cairo_set_line_width( offcai, 0.5 );
	// draw the curves
	cairo_save( offcai );
	cairo_translate( offcai, mx, 0 );
	if	( force_redraw )
		{
		// fill the background JUST FOR CHECK THAT THEY PAINT EVERYWHERE
		// cairo_set_source_rgb( offcai, 1, 0, 0 );
		// cairo_paint(offcai);	// paint the complete clip area
		for	( ib = 0; ib < bandes.size(); ib++ )
			{
			if	( bandes.at(ib)->visible )
				{
				bandes.at(ib)->draw( offcai );
				cairo_translate( offcai, 0, bandes.at(ib)->fdy );
				}
			}
		force_redraw = 0;
		// printf("panel full draw end\n");
		}
	else	{
		for	( ib = 0; ib < bandes.size(); ib++ )
			{
			if	( bandes.at(ib)->visible )
				{
				if	( bandes.at(ib)->force_redraw )
					bandes.at(ib)->draw( offcai );
				// N.B. strip::force_redraw est RAZ par strip::draw
				cairo_translate( offcai, 0, bandes.at(ib)->fdy );
				}
			}
		// printf("panel selective draw end\n");
		}
	cairo_restore( offcai );
	xcursor = -1.0;	// il y a pu avoir zoom ou pan X, eviter de redessiner le curseur au meme X
	}
}

// cette methode automatique met a jour la "drawing area" dite "ecran" (une zone du frame buffer en fait)
//	- si c'est suffisant elle va seulement mettre a jour le play cursor directement sur l'ecran
//	  en copiant une minuscule zone de drawpad pour effacer l'ancien curseur
//	- si c'est demande par force_repaint ou par une activite de drag elle va d'abord mettre a jour
//	  le panel entier sur l'ecran en copiant le drawpad.
//	  le drawpad aura eventuellement ete regenere par la methode draw()
//	- s'il y a drag en cours elle va dessiner par dessus tout un motif fantome
// l'action curseur est effectuee en single buffer si elle est seule
void gpanel::paint()
{
// special profileur
++paint_cnt;
cairo_t * cair = NULL;

if	( drag.mode != nil )
	force_repaint = 1;

if	( force_repaint )
	{
	// entrer en mode double-buffer
	gdk_window_begin_paint_region( widget->window, laregion );
	cair = gdk_cairo_create( widget->window );
	cairo_set_line_width( cair, 0.5 );

	// paint the panel
	if	( offscreen_flag == 0 )
		{
		printf("gpanel fallback to panel::draw\n");
		// fill the background (RED just for test)
		// cairo_set_source_rgb( cair, 1, 0, 0 );
		// cairo_paint(cair);	// paint the complete clip area
		// preparer font a l'avance (panel::draw ne le fait pas !)
		cairo_select_font_face( cair, "Courier", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size( cair, 12.0 );
		panel::draw(cair);
		}
	else	{
		// draw() decidera automatiquement s'il faut refaire du trace vectoriel sur le drawpad
		draw();
		// mais on copie toujours le drawpad
		drawpad_compose( cair );
		}
	// nothing dirty anymore, et curseur a disparu
	xdirty = -1.0;

	// possibly draw the selection phantom rectangle (decidera automatiquement)
	drag.draw( cair );
	}

// possibly clean the time cursor zone (possiblement en single-buffer)
if	( ( xdirty >= 0.0 ) && ( drawpad ) )
	{	// copier une petite zone du drawpad sur l'ecran
	cursor_zone_clean( &cair );
	xdirty = -1.0;
	}
// possibly draw the time cursor (possiblement en single-buffer)
if	( xcursor >= 0.0 )
	{
	double x = xcursor + (double)mx;
	if	( cair == NULL )
		cair = gdk_cairo_create( widget->window );
	cairo_set_source_rgba( cair, 0.5, 0.5, 0.5, 1.0 );
	cairo_set_line_width( cair, 0.5 );
	cairo_move_to( cair, x, 0.0 );
	cairo_line_to( cair, x, fdy );
	cairo_stroke(cair);
	xdirty = xcursor;
	}

if	( cair )
	cairo_destroy(cair);

if	( force_repaint )
	{
	// on quitte le mode double-buffer
	gdk_window_end_paint( widget->window );
	force_repaint = 0;
	}
}

// methode expose pour repondre a un expose event. On force repaint car on n'a
// aucune info sur ce qui est dirty
void gpanel::expose()
{
force_repaint = 1;
paint();
}

void gpanel::configure()
{
int ww, wh;
gdk_drawable_get_size( widget->window, &ww, &wh );
printf("gpanel configuzed %d x %d\n", ww, wh );
resize( ww, wh );
// mettre a jour la region qui sert pour begin_paint
if	( laregion )
	gdk_region_destroy( laregion );
laregion = gdk_drawable_get_clip_region( widget->window );
force_repaint = 1; fflush(stdout);
}

/** ===================== gpanel event handling methods =============================== */

void gpanel::key( GdkEventKey * event )
{
if	( event->type == GDK_KEY_PRESS )
	{
	if	( selected_key == 0 )	// pour filtrer les repetitions automatiques
		{
		int v = event->keyval;
		if	( ( v < GDK_Shift_L ) || ( v > GDK_Alt_R ) )	// see gdk/gdkkeysyms.h
			{	// fragile detection des modifiers ( event->is_modifier est bogus )
			selected_key = v;
			/* info verbeuse
			printf("Key h=%04x v=%04x (%04x) \"%s\" press\n",
				event->hardware_keycode, event->keyval, event->state, event->string );
			if	( ( selected_key >= GDK_F1 ) && ( selected_key <= GDK_F12 ) )
				printf("F%d\n", selected_key + 1 - GDK_F1 );
			*/
			// bindkeys locales
			if	( selected_key == 'f' )
				{
				fullMN(); force_repaint = 1;
				// gtk_widget_queue_draw( widget );
				}
			// autres, transmises a l'appli
			else	{
				if	( key_call_back )
					key_call_back( selected_key, call_back_data );
				}
			}
		}
	}
else if ( event->type == GDK_KEY_RELEASE )
	{
 	// printf("Key releas v=%04x\n", selected_key );
	selected_key = 0;
	}
}

void gpanel::clic( GdkEventButton * event )
{
double x, y;
x = event->x; y = event->y;
if	( event->type == GDK_BUTTON_PRESS )
	{
	drag.x0 = x;
	drag.y0 = y;
	drag.x1 = x;
	drag.y1 = y;
	if	( event->button == 1 )
		{
		if	( selected_key == ' ' )
			drag.mode = pan;
		else	drag.mode = select_zone;
		}
	else if	( event->button == 3 )
		{
		drag.mode = zoom;
		}
	}
else if	( event->type == GDK_BUTTON_RELEASE )
	{
	// ici on insere traitement de la fenetre draguee : action clic ou action select_zone ou action zoom
	if	( ( abs( drag.x0 - drag.x1 ) < MIN_DRAG ) && ( abs( drag.y0 - drag.y1 ) < MIN_DRAG ) )
		{	// action clic sans drag
		int istrip; double M, N;
		istrip = clicMN( x, y, &M, &N );
		if	( event->button == 1 )
			{
			char utbuf[32];	// buffer pour scientout
			char vtbuf[32];	// buffer pour scientout
			if	( istrip < 0 )
				printf("clic hors graphique (%d)\n", istrip );
			else	{
				if	( ( istrip & CLIC_MARGE ) == 0 )
					{	// clic dans une courbe
					if	( clic_call_back )
						clic_call_back( M, N, call_back_data );
					else	{
						// ce coeff 0.002 suggere une resolution 500 fois plus fine que le tick, Ok ?
						scientout( utbuf, M, 0.002 * tdq );
						scientout( vtbuf, N, 0.002 * bandes[istrip]->tdr );
						printf("clic strip %d [%s:%s]\n", istrip, utbuf, vtbuf );
						}
					}
				else	{
					if	( istrip & CLIC_MARGE_GAUCHE )
						printf("clic marge gauche strip %d\n", istrip & ~CLIC_MARGE );
					else if	( istrip & CLIC_MARGE_INF )
						{
						scientout( utbuf, M, 0.002 * tdq );
						printf("clic marge inf M = %s\n", utbuf );
						}
					}
				}
			}
		else if	( event->button == 3 )
			{
			if	( istrip >= 0 )
				{
				selected_strip = istrip;
				if	( istrip & CLIC_MARGE_INF )
					gtk_menu_popup( (GtkMenu *)menu1_x, NULL, NULL, NULL, NULL,
							event->button, event->time );
				else if	( istrip & CLIC_MARGE_GAUCHE )
					{
					gtk_menu_popup( (GtkMenu *)menu1_y, NULL, NULL, NULL, NULL,
							event->button, event->time );
					}
				}
			}
		drag.mode = nil;
		}
	else	{	// action drag
		int istrip0, istrip1; double X0, X1, Y0, Y1;
		istrip0 = clicXY( drag.x0, drag.y0, &X0, &Y0 );
		istrip1 = clicXY( drag.x1, drag.y1, &X1, &Y1 );
		if	( ( istrip0 >= 0 ) && ( istrip1 >= 0 ) )
			{
			switch	( drag.mode )
				{
				case nil : break;
				case zoom :
					if	( !( istrip0 & CLIC_MARGE ) )
						{
						istrip0 &= (~CLIC_MARGE);
						istrip1 &= (~CLIC_MARGE);
						// zoom X (toujours)
						if	( X1 >= X0 )
							zoomX( X0, X1 );
						else	zoomX( X1, X0 );
						/* zoom vertical si on est reste sur le meme strip */
						if	( istrip0 == istrip1 )
							{	// zoom Y sur 1 strip
							if	( Y1 >= Y0 )
								bandes.at(istrip0)->zoomY( Y0, Y1 );
							else	bandes.at(istrip0)->zoomY( Y1, Y0 );
							}
						//*/
						}
					break;
				case select_zone :
					printf("selected from %d<%g:%g> to %d<%g:%g>\n",
						istrip0, X0, Y0, istrip1, X1, Y1 );
					break;
				case pan :
					if	( istrip0 == istrip1 )
						{
						istrip0 &= (~CLIC_MARGE);
						bandes.at(istrip0)->panY( Y0 - Y1 );
						panX( X0 - X1 );
						}
					break;
				}
			}
		drag.mode = nil;
		force_repaint = 1;
		// gtk_widget_queue_draw( widget );
		}
	}
}

void gpanel::motion( GdkEventMotion * event )
{
GdkModifierType state = (GdkModifierType)event->state;

if	( ( state & GDK_BUTTON1_MASK ) || ( state & GDK_BUTTON3_MASK ) )
	{
	drag.x1 = event->x;
	drag.y1 = event->y;
	queue_flag = 1;		// attention on compte sur la fonction idle de l'appli pour queuter le draw
	// gtk_widget_queue_draw( widget );
	}
}

void gpanel::wheel( GdkEventScroll * event )
{
int istrip; double X, Y;
istrip = clicXY( event->x, event->y, &X, &Y );
if	( istrip & CLIC_MARGE_INF )
	{
	if	( event->direction == GDK_SCROLL_DOWN )
		{
		if	( event->state & GDK_CONTROL_MASK )
			panXbyK( -0.05 );
		else	zoomXbyK( 0.9 );
		}
	if	( event->direction == GDK_SCROLL_UP )
		{
		if	( event->state & GDK_CONTROL_MASK )
			panXbyK( 0.05 );
		else	zoomXbyK( 1.11 );
		}
	force_repaint = 1;
	// gtk_widget_queue_draw( widget );
	}
else if	( istrip & CLIC_MARGE_GAUCHE )
	{
	strip * b = bandes.at(istrip & (~CLIC_MARGE));
	if	( event->direction == GDK_SCROLL_DOWN )
		{
		if	( event->state & GDK_CONTROL_MASK )
			b->panYbyK( -0.02 );
		else	b->zoomYbyK( 0.9 );
		}
	if	( event->direction == GDK_SCROLL_UP )
		{
		if	( event->state & GDK_CONTROL_MASK )
			b->panYbyK( 0.02 );
		else	b->zoomYbyK( 1.11 );
		}
	force_repaint = 1;
	// gtk_widget_queue_draw( widget );
	}
else	{
	// strip * b = bandes.at(istrip & (~CLIC_MARGE));	// pour science
	if	( event->direction == GDK_SCROLL_DOWN )
		{
		// b->panYbyK( -0.02 );	// pour science
		panXbyK( -0.02 );	// pour audio
		}
	if	( event->direction == GDK_SCROLL_UP )
		{
		// b->panYbyK( 0.02 );	// pour science
		panXbyK( 0.02 );	// pour audio
		}
	force_repaint = 1;
	// gtk_widget_queue_draw( widget );
	}
}

/** ===================== png service methods =================================== */

void gpanel::png_save_drawpad( const char * fnam )
{
static GdkPixbuf * dumpix = NULL;	// pixbuf reutilisable! evite memory leak!
dumpix = gdk_pixbuf_get_from_drawable( dumpix, drawpad, NULL, 0, 0, 0, 0, -1, -1 );
if	( dumpix == NULL )
	{ printf("failed gdk_pixbuf_get_from_drawable\n"); fflush(stdout); return; }
if  	( gdk_pixbuf_save( dumpix, fnam, "png", NULL, NULL ) )
	printf("Ok ecriture copie d'ecran %s\n", fnam );
else	printf("oops, echec ecriture copie d'ecran %s\n", fnam );
fflush(stdout);
}

/** ===================== pdf service methods =================================== */

// fonction bloquante
void gpanel::pdf_modal_layout( GtkWidget * mainwindow )
{
if ( bandes.size() == 0 )
   return;

// petit dialogue pour ce pdf
GtkWidget * curwin;
GtkWidget * curbox;
GtkWidget * curbox2;
GtkWidget * curwidg;

curwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
/* ATTENTION c'est serieux : modal veut dire que la fenetre devient la
   seule a capturer les evenements ( i.e. les autres sont bloquees ) */
gtk_window_set_modal( GTK_WINDOW(curwin), TRUE );
gtk_window_set_position( GTK_WINDOW(curwin), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwin), GTK_WINDOW(mainwindow) );
gtk_window_set_type_hint( GTK_WINDOW(curwin), GDK_WINDOW_TYPE_HINT_DIALOG );

gtk_window_set_title( GTK_WINDOW(curwin), "sortie fichier PDF" );
gtk_container_set_border_width( GTK_CONTAINER(curwin), 20 );

gtk_signal_connect( GTK_OBJECT(curwin), "delete_event",
                    GTK_SIGNAL_FUNC( gpanel_pdf_modal_delete ), NULL );
wchoo = curwin;

// boite verticale
curbox = gtk_vbox_new( FALSE, 20 );
gtk_container_add( GTK_CONTAINER( curwin ), curbox );

// label
curwidg = gtk_label_new( "Description :" );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, FALSE, FALSE, 0);

// entree editable
curwidg = gtk_entry_new_with_max_length(64);
gtk_widget_set_usize( curwidg, 600, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "Bugiganga" );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, FALSE, FALSE, 0 );
edesc = curwidg;

// label
curwidg = gtk_label_new( "Nom de fichier :" );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, FALSE, FALSE, 0);


curwidg = gtk_file_chooser_widget_new( GTK_FILE_CHOOSER_ACTION_SAVE );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0 );
gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(curwidg), "C:\\tmp");
gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(curwidg), "pipu.pdf" );
fchoo = curwidg;

// boite horizontale
curbox2 = gtk_hbox_new( FALSE, 5 );
gtk_box_pack_start( GTK_BOX( curbox ), curbox2, FALSE, FALSE, 0);

/* le bouton ok */
curwidg = gtk_button_new_with_label (" Ok ");
gtk_box_pack_end( GTK_BOX( curbox2 ), curwidg, FALSE, FALSE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gpanel_pdf_ok_button ), (gpointer)this );

/* le bouton abort */
curwidg = gtk_button_new_with_label (" Annuler ");
gtk_box_pack_end( GTK_BOX( curbox2 ), curwidg, FALSE, FALSE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_widget_show_all( curwin );

/* on est venu ici alors qu'on est deja dans 1 boucle gtk_main
   alors donc on en imbrique une autre. Le prochain appel a
   gtk_main_quit() fera sortir de cell-ci (innermost)
 */
gtk_main();
gtk_widget_destroy( curwin );
}


void gpanel::pdf_ok_call()
{
char * fnam = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(fchoo) );
if ( fnam )
   {
   unsigned int iban;
   for	( iban = 0; iban < bandes.size(); iban++ )
	bandes[iban]->optcadre = 1; // ici demo fond blanc + cadre en couleur
   int retval = pdfplot( fnam, gtk_entry_get_text( GTK_ENTRY(edesc) ) );
   printf("retour panel::pdfplot %s : %d\n", fnam, retval );
   for	( iban = 0; iban < bandes.size(); iban++ )
	bandes[iban]->optcadre = 0; // ici demo
   }
// remettre aux dimensions de la drawing area
int ww, wh;
gdk_drawable_get_size( widget->window, &ww, &wh );
resize( ww, wh );
gtk_main_quit();
}

/** ===================== context menus =================================== */

GtkWidget * gpanel::mkmenu1( const char * title )
{
GtkWidget * curmenu;
GtkWidget * curitem;

curmenu = gtk_menu_new ();    // Don't need to show menus

curitem = gtk_menu_item_new_with_label(title);
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Full");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( menu1_full ), (gpointer)this );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Zoom in");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( menu1_zoomin ), (gpointer)this );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Zoom out");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( menu1_zoomout ), (gpointer)this );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
gtk_widget_show ( curitem );

return curmenu;
}
/** ===================== ghost_drag methods ===================================== */

void ghost_drag::zone( cairo_t * cair )
{
cairo_rectangle( cair, x0, y0, x1 - x0, y1 - y0 );
cairo_fill(cair);
}

void ghost_drag::box( cairo_t * cair )
{
cairo_rectangle( cair, x0, y0, x1 - x0, y1 - y0 );
cairo_stroke(cair);
}

void ghost_drag::line( cairo_t * cair )
{
cairo_move_to( cair, x0, y0 );
cairo_line_to( cair, x1, y1 );
cairo_stroke(cair);
}

// automatique
void ghost_drag::draw( cairo_t * cair )
{
switch	( mode )
	{
	case nil : break;
	case select_zone :
		cairo_set_source_rgba( cair, 0.0, 0.8, 1.0, 0.3 );
		zone( cair );
		break;
	case zoom :
		cairo_set_source_rgba( cair, 0.0, 0.0, 0.0, 1.0 );
		box(cair);
		break;
	case pan :
		cairo_set_source_rgba( cair, 0.0, 0.0, 0.0, 1.0 );
		line(cair);
		break;
	}
}

/** ===================== gzoombar methods ====================================== */

GtkWidget * gzoombar::layout( int w )
{
widget = gtk_drawing_area_new ();

// ajuster la drawing area aux dimensions voulues
gtk_widget_set_size_request( widget, w, 32 );

/* Drawing Area Signals  */

g_signal_connect( widget, "expose_event", G_CALLBACK(gzoombar_expose), this );
g_signal_connect( widget, "configure_event", G_CALLBACK(gzoombar_configure), this );
g_signal_connect( widget, "button_press_event",   G_CALLBACK(gzoombar_click), this );
g_signal_connect( widget, "button_release_event", G_CALLBACK(gzoombar_click), this );
g_signal_connect( widget, "motion_notify_event", G_CALLBACK(gzoombar_motion), this );
g_signal_connect( widget, "scroll_event", G_CALLBACK( gzoombar_wheel ), this );

// Ask to receive events the drawing area doesn't normally subscribe to
gtk_widget_set_events ( widget, gtk_widget_get_events(widget)
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_SCROLL_MASK
			| GDK_POINTER_MOTION_MASK
//			| GDK_POINTER_MOTION_HINT_MASK
		      );
return widget;
}

void gzoombar::expose()
{
// printf("expozed\n");

cairo_t * cair = gdk_cairo_create(widget->window);

// fill the background
cairo_set_source_rgb( cair, 0.88, 0.88, 1 );
cairo_paint(cair);	// paint the complete clip area

// preparer font a l'avance
// cairo_select_font_face( cair, "Courier", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
// cairo_set_font_size( cair, 12.0 );
// cairo_set_line_width( cair, 0.5 );

// draw the bar
cairo_set_source_rgb( cair, 0.66, 0.66, 1 );
cairo_rectangle( cair, x0, 4.0, x1 - x0, 24.0 );
cairo_fill(cair);
if	( opcode )
	{
	cairo_set_source_rgb( cair, 0.0, 0.0, 0.0 );
	cairo_rectangle( cair, x0f, 4.0, x1f - x0f, 24.0 );
	cairo_stroke(cair);
	}
cairo_destroy(cair);
}

void gzoombar::configure()
{
int wh;
gdk_drawable_get_size( widget->window, &ww, &wh );
ndx = (double)ww - ( 2.0 * xm );
if	( panneau )
	{
	double fullspan = panneau->fullmmax - panneau->fullmmin;
	double k0 = ( panneau->MdeX(0.0) 		  - panneau->fullmmin ) / fullspan;
	double k1 = ( panneau->MdeX((double)panneau->ndx) - panneau->fullmmin ) / fullspan;
	zoom( k0, k1 );
	}
printf("gzoombar configuzed %d x %d\n", ww, wh ); fflush(stdout);
}

void gzoombar::clic( GdkEventButton * event )
{
double x = event->x;
// double y = event->y;
if	( event->type == GDK_BUTTON_PRESS )
	{
	if	( event->button == 1 )
		{
		xc = x;
		x0f = x0; x1f = x1;
		opcode = op_select( x );
		printf("op %c\n", (opcode < ' ')?(opcode+'0'):(opcode) );
		}
	else if	( event->button == 3 )
		{
		xc = x;
		x0f = x0; x1f = x1;
		opcode = 'Z';
		}
	}
else if	( event->type == GDK_BUTTON_RELEASE )
	{
	if	( x == xc )
		{		// action clic sans drag
		if	( panneau )
			{
			if	( event->button == 1 )
				{
				double m, kx2m;
				kx2m = (panneau->fullmmax - panneau->fullmmin)/ndx;
				m = kx2m * (x-xm);
				printf("zoombar clic M=%g\n", m );
				}
			else if ( event->button == 3 )
				{
				panneau->selected_strip = CLIC_ZOOMBAR;
				gtk_menu_popup( (GtkMenu *)panneau->menu1_x, NULL, NULL, NULL, NULL,
							event->button, event->time );
				}
			}
		opcode = 0;
		}
	else	{		// action drag
		x0 = x0f; x1 = x1f;
		opcode = 0;
		gtk_widget_queue_draw( widget );
		if	( panneau )
			{
			double mmin, mmax, kx2m;
			kx2m = (panneau->fullmmax - panneau->fullmmin)/ndx;
			mmin = kx2m * (x0-xm);
			mmin += panneau->fullmmin;
			mmax = kx2m * (x1-xm);
			mmax += panneau->fullmmin;
			panneau->zoomM( mmin, mmax );
			panneau->force_repaint = 1;
			// gtk_widget_queue_draw( panneau->widget );
			}
		}
	}
}

void gzoombar::motion( GdkEventMotion * event )
{
GdkModifierType state = (GdkModifierType)event->state;

if	( ( state & GDK_BUTTON1_MASK ) || ( state & GDK_BUTTON3_MASK ) )
	{
	// printf("drag %6g:%6g\n", event->x, event->y );
	update( event->x );
	gtk_widget_queue_draw( widget );
	}
}

void gzoombar::wheel( GdkEventScroll * event )
{
if	( panneau )
	{
	if	( event->direction == GDK_SCROLL_DOWN )
		{
		panneau->zoomXbyK( 0.9 );
		}
	if	( event->direction == GDK_SCROLL_UP )
		{
		panneau->zoomXbyK( 1.1 );
		}
	panneau->force_repaint = 1;
	// gtk_widget_queue_draw( panneau->widget );
	}
}

// choisir l'operation en fonction du lieu du clic
int gzoombar::op_select( double x )
{
if	( ( x < ( x0 - xm ) ) || ( x > ( x1 + xm ) ) )
	return 0;
if	( ( x1 - x0 ) > xlong )
	{			// barre "longue"
	if	( x < ( x0 + xm ) ) return 'L';
	if	( x > ( x1 - xm ) ) return 'R';
	return 'M';
	}
else	{			// barre "courte"
	if	( x < x0 ) return 'L';
	if	( x > x1 ) return 'R';
	return 'M';
	}
}

// calculer la nouvelle position de la barre (fantome)
void gzoombar::update( double x )
{
switch	( opcode )
	{
	case 'L' :	// mettre a jour x0f
		x0f = x;
		if	( x0f > ( x1f - dxmin ) )	// butee droite
			x0f = ( x1f - dxmin );
		if	( x0f < xm )			// butee gauche
			x0f = xm;
		break;
	case 'M' :	// translater x0f et x1f
		x0f = x0 + ( x - xc );
		x1f = x1 + ( x - xc );
		if	( x0f < xm )			// butee gauche
			{ x0f = xm; x1f = x0f + ( x1 - x0 );  }
		if	( x1f > ( ndx + xm ) )		// butee droite
			{ x1f = ndx + xm; x0f = x1f - ( x1 - x0 ); }
		break;
	case 'R' :	// mettre a jour x1f
			x1f = x;
		if	( x1f < ( x0f + dxmin ) )	// butee gauche
			x1f = ( x0f + dxmin );
		if	( x1f > ( ndx + xm ) )		// butee droite
			x1f = ndx + xm;
		break;
	case 'Z' :
		if	( x < xm )			// butee gauche
			x = xm;
		if	( x > ( ndx + xm ) )		// butee droite
			x = ndx + xm;
		if	( x > xc )
			{ x0f = xc; x1f = x; }
		else	{ x1f = xc; x0f = x; }
		if	( ( x1f - x0f ) < dxmin )
			x1f = x0f + dxmin;
		break;
	}
}

// mettre a jour la barre en fonction d'un cadrage normalise venant d'un autre widget
void gzoombar::zoom( double k0, double k1 )
{
// printf("gzoombar::zoom( %g, %g )\n", k0, k1 );
x0 = xm + ndx * k0;
x1 = xm + ndx * k1;
gtk_widget_queue_draw( widget );
}
using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
// #include <cairo-pdf.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layers.h"
#include "strips.h"



// alloue ou re-alloue le pad s'il est trop petit
// prend soin aussi de son cairo persistant
void strip_dp::drawpad_resize()
{
int w = parent->ndx;
int h = ndy;
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
// translater l'origine Y en bas de la zone utile des courbes
// c'est un service que strip rend normalement aux layers
cairo_translate( offcai, 0, h );
}

void strip_dp::draw( cairo_t * cai )
{
if	( offscreen_flag == 0 )
	{
	printf("strip_dp fallback to strip::draw\n");
	strip::draw(cai);
	return;
	}

// printf("strip_dp::begin draw\n");
// y a-t-il eu zoom ou pan ?
if	(
	( old_q0 != parent->q0 ) ||
	( old_kq != parent->kq ) ||
	( old_r0 != r0 ) ||
	( old_kr != kr )
	)
	{
	dp_dirty_flag = 1;
	old_q0 = parent->q0;
	old_kq = parent->kq;
	old_r0 = r0;
	old_kr = kr;
	}
// preparer le drawpad
if	( dp_dirty_flag )
	{
	drawpad_resize();
	// fill the background
	cairo_set_source_rgb( offcai, bgcolor.dR, bgcolor.dG, bgcolor.dB );
	// cairo_set_source_rgb( offcai, 0.6, 0.6, 0.0 );
	cairo_paint(offcai);	// paint the complete clip area
	// printf("paint the drawpad\n");

	// tracer le reticule y sur le drawpad : la grille de barres horizontales
	cairo_set_source_rgb( offcai, lncolor.dR, lncolor.dG, lncolor.dB );
	reticule_Y( offcai );

	// tracer le reticule x sur le drawpad : la grille de barres verticales
	reticule_X( offcai );

	// tracer les courbes sur le drawpad
	int i;
	for ( i = ( courbes.size() - 1 ); i >= 0; i-- )
	    {
	    courbes.at(i)->ylabel = ( 20 * i ) + 20;
	    courbes.at(i)->draw( offcai );
	    }
	dp_dirty_flag = 0;
	}

unsigned int ddx;
ddx = parent->ndx;

if	( drawab )
	{
	/* special B1 */
	if	( gc == NULL )
		gc = gdk_gc_new( drawab );
	//						destx  desty
	gdk_draw_drawable( drawab, gc, drawpad, 0, 0, parent->mx, 0, ddx, ndy );
	// printf("drawn drawab !\n");
	//*/
	}
else	{
	// copier le drawpad sur la drawing area, methode Cairo
	// origine Y est (encore) en haut du strip...
	gdk_cairo_set_source_pixmap( cai, drawpad, 0.0, 0.0 );
	// cairo_set_source_rgb( cai, 0.0, 0.6, 0.6 );
	cairo_rectangle( cai, 0.0, 0.0, double(ddx), double(ndy) );
	cairo_fill(cai);
	}

// remettre le cairo en couleur unie (noir pour les textes)
cairo_set_source_rgb( cai, 0.0, 0.0, 0.0 );
// translater l'origine Y en bas de la zone utile des courbes
cairo_save( cai );
cairo_translate( cai, 0, ndy );

// les textes de l'axe X
if	( optX )
	gradu_X( cai );
// les textes de l'axe Y
gradu_Y( cai );

cairo_restore( cai );
// printf("strip_dp::end draw\n");
}


using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
// #include <cairo-pdf.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_rgb.h"

// layer_rgb : une image RGB telle qu'un spectrogramme (classe derivee de layer_base)
// application typique :
//	U est en FFT-runs
//	V est en bins logarithmises (une fraction de demi-ton)

// les methodes qui sont virtuelles dans la classe de base
double layer_rgb::get_Umin()
{ return (double)0; }
double layer_rgb::get_Umax()
{ return (double)gdk_pixbuf_get_width(spectropix); }
double layer_rgb::get_Vmin()
{ return (double)0; }
double layer_rgb::get_Vmax()
{ return (double)gdk_pixbuf_get_height(spectropix); }

// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_rgb::draw( cairo_t * cai )
{
cairo_matrix_t saved_matrix;
// dimensions de la fenetre visible dans l'espace XY et dans l'espace UV
double dx = (double)parent->parent->ndx;
double dy = (double)parent->ndy;
// double du = dx / ku;
// double dv = dy / kv;
// u0 et v0 : coord de l'origine de la fenetre visible (coin inf. gauche) dans l'espace UV
// printf("ku:kv = %g:%g, u0:v0 = %g:%g\n", ku, kv, u0, v0 ); fflush(stdout);
// placer cairo dans l'espace UV
cairo_get_matrix( cai, &saved_matrix );	// different de cairo_save() qui sauve aussi le pattern
cairo_scale( cai, ku, -kv );		// a partir de la plus besoin de changer le signe des Y
cairo_translate( cai, -u0, -v0 );
// la transformation va etre appliquee au pattern (la texture)
gdk_cairo_set_source_pixbuf( cai, spectropix, 0.0, 0.0 );
// gerer le cas ou la texture ne remplit pas tout le rectangle (par defaut CAIRO_EXTEND_NONE)
// CAIRO_EXTEND_REPEAT, CAIRO_EXTEND_REFLECT, CAIRO_EXTEND_PAD
// cairo_pattern_set_extend( cairo_get_source(cai), CAIRO_EXTEND_PAD );

/* motif de test *
double a0, b0, da, db;	// en unites UV
a0 = 1.0 * 44100.0 / 1024.0;	// 1 seconde, exprimee en fftstrides (U)
da = 2.0 * a0;			// 2 secondes
b0 = (50.0 - 28.0) * 10.0;	// midinote 50 exprimee en binxels (V) (base MI grave 28)
db = 10.0 * 10.0;		// 10 demi-tons
cairo_set_source_rgb( cai, 0.0, 1.0, 0.0 );
cairo_set_line_width( cai, 4.0 );
cairo_rectangle( cai, a0, b0, da, db );
cairo_stroke( cai );
//*/

// retour a l'espace XY-cairo du strip (affecte les traces vectoriels mais PAS le pattern)
cairo_set_matrix( cai, &saved_matrix );
// afficher le spectre dans le rectangle de clip
cairo_rectangle( cai, 0.0, -dy, dx, dy  );
cairo_fill( cai );

/* test du rectangle de clip *
cairo_set_source_rgb( cai, 1.0, 0.0, 0.0 );
cairo_set_line_width( cai, 4.0 );
cairo_rectangle( cai, 0.0, -dy, dx, dy  );
cairo_stroke( cai );
//*/
}

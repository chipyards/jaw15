using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_f_param.h"

// layer_f_param : une courbe parametrique en float (classe derivee de layer_base)


// chercher le premier point X >= 0
int layer_f_param::goto_U( double U0 )
{
curi = 0;
while	( curi < qu )
	{
	if	( U[curi++] >= U0 )
		return 0;
	}
return -1;
}

// get XY then post increment
int layer_f_param::get_pi( double & rU, double & rV )
{
if ( curi >= qu )
   return -1;
rU = (double)U[curi]; rV = (double)V[curi]; ++curi;
return 0;
}

// chercher les Min et Max
void layer_f_param::scan()
{
if	( qu )
	{
	Umin = Umax = U[0];
	Vmin = Vmax = V[0];
	}
int i; double u, v;
for	( i = 1; i < qu; i++ )
	{
	u = U[i];
	if ( u < Umin ) Umin = u;
	if ( u > Umax ) Umax = u;
	v = V[i];
	if ( v < Vmin ) Vmin = v;
	if ( v > Vmax ) Vmax = v;
	}
// printf("%g < V < %g\n", Vmin, Vmax );
}


// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_f_param::draw( cairo_t * cai )
{
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
cairo_show_text( cai, label.c_str() );

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double tU, tV, curx, cury, maxx;
if ( goto_U( u0 ) )
   return;
maxx = (double)(parent->parent->ndx);
if	( get_pi( tU, tV ) )
	return;
curx =  XdeU( tU );			// on a le premier point ( tU, tV )
cury = -YdeV( tV );			// signe - ici pour Cairo
cairo_move_to( cai, curx, cury );
while	( get_pi( tU, tV ) == 0 )
	{
	curx =  XdeU( tU );		// les transformations
	cury = -YdeV( tV );
	if	( curx > maxx )
		cairo_move_to( cai, curx, cury );
	else	cairo_line_to( cai, curx, cury );
	}
cairo_stroke( cai );
}

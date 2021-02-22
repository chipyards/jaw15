using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
// #include <cairo-pdf.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_u16.h"

// layer_u16 : une courbe a pas uniforme en unsigned short (classe derivee de layer_base)

// chercher le premier point X >= X0
int layer_u16::goto_U( double U0 )
{
curi = (int)ceil(U0);
if	( curi < 0 )
	curi = 0;
if	( curi < qu )
	return 0;
else	return -1;
}

void layer_u16::goto_first()
{
curi = 0;
}

// get XY then post increment
int layer_u16::get_pi( double & rU, double & rV )
{
if ( curi >= qu )
   return -1;
rU = (double)curi; rV = (double)V[curi]; ++curi;
return 0;
}

// chercher les Min et Max
void layer_u16::scan()
{
if	( qu )
	{
	Vmin = Vmax = V[0];
	}
int i; double v;
for	( i = 1; i < qu; i++ )
	{
	v = V[i];
	if ( v < Vmin ) Vmin = v;
	if ( v > Vmax ) Vmax = v;
	}
// printf("%g < V < %g\n", Vmin, Vmax );
}

// layer_u16 : les methodes qui sont virtuelles dans la classe de base
double layer_u16::get_Umin()
{ return (double)0; }
double layer_u16::get_Umax()
{ return (double)(qu-1); }
double layer_u16::get_Vmin()
{ return (double)Vmin; }
double layer_u16::get_Vmax()
{ return (double)Vmax; }

// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_u16::draw( cairo_t * cai )
{
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
cairo_show_text( cai, label.c_str() );

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double tU, tV, curx, cury, maxx;
if ( goto_U( u0 ) )
   return;
maxx = (double)(parent->parent->ndx);

if	( style == 0 )
	{				// style par defaut : courbe classique
	if	( get_pi( tU, tV ) )
   		return;
	curx =  XdeU( tU );			// on a le premier point ( tU, tV )
	cury = -YdeV( tV );			// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );		// les transformations
		cury = -YdeV( tV );
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		}
	}
else if	( style == 1 )
	{				// style spectre en barres verticales
	double zeroy = -YdeV( 0.0 );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );		// les transformations
		cury = -YdeV( tV );
		cairo_move_to( cai, curx, zeroy );	// special spectro en barres
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		}
	}
cairo_stroke( cai );
}

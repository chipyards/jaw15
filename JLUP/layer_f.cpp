using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
// #include <cairo-pdf.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_f.h"

// layer_f : une courbe a pas uniforme en float (classe derivee de layer_base)
// supporte 3 styles :
// 0 : style par defaut : courbe classique
// 1 : style spectre en barres verticales
// 2 : courbe classique en dB


// chercher le premier point X >= X0
int layer_f::goto_U( double U0 )
{
curi = (int)ceil(U0);
if	( curi < 0 )
	curi = 0;
if	( curi < qu )
	return 0;
else	return -1;
}

void layer_f::goto_first()
{
curi = 0;
}

// get XY then post increment
int layer_f::get_pi( double & rU, double & rV )
{
if ( curi >= qu )
   return -1;
rU = (double)curi; rV = (double)V[curi]; ++curi;
return 0;
}

// chercher les Min et Max
void layer_f::scan()
{
if	( qu )
	{
	Vmin = Vmax = V[0];
	}
int i; double v;
if	( style != 2 )
	{
	for ( i = 1; i < qu; i++ )
	    {
	    v = V[i];
	    if ( v < Vmin ) Vmin = v;
	    if ( v > Vmax ) Vmax = v;
	    }
	}
else	{
	double k0dB = 1.0 / ( 32767.0 * ( qu - 1 ) );	// t.q. sinewave max <==> 0dB
	Vmin = -20.0; Vmax = 0.0;
	for ( i = 1; i < qu; i++ )
	    {
	    v = 20.0 * log10( V[i] * k0dB );
	    if ( v < Vmin ) Vmin = v;
	    if ( v > Vmax ) Vmax = v;
	    }
	}
// printf("%g < V < %g\n", Vmin, Vmax );
}

// layer_f : les methodes qui sont virtuelles dans la classe de base
double layer_f::get_Umin()
{ return (double)0; }
double layer_f::get_Umax()
{ return (double)(qu-1); }
double layer_f::get_Vmin()
{ return (double)Vmin; }
double layer_f::get_Vmax()
{ return (double)Vmax; }

// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_f::draw( cairo_t * cai )
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
else if	( style == 2 )
	{				// courbe classique en dB
	double k0dB = 1.0 / ( 32767.0 * ( qu - 1 ) );	// t.q. sinewave max <==> 0dB
	if	( get_pi( tU, tV ) )			// qu - 1 = fftsize / 2
   		return;
	curx =  XdeU( tU );
	cury = -YdeV( 20.0 * log10( tV * k0dB ) );	// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );		// les transformations
		cury = -YdeV( 20.0 * log10( tV * k0dB ) );
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		}
	}
cairo_stroke( cai );
}

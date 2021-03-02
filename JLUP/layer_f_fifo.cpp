using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_f_fifo.h"

// layer_f_fifo : une courbe a pas uniforme en float (classe derivee de layer_base)
// supporte 2 styles :
// 0 : style par defaut : courbe classique
// 1 : style spectre en barres verticales

// ajouter un point
void layer_f_fifo::push( double Ve )
{
unsigned int i = maskfifo() & (unsigned int)Ue;
V[i] = Ve;
Ue += 1;
if	( Ub < ( Ue - (long long)(1<<log_qfifo) ) )
	Ub = Ue - (long long)(1<<log_qfifo);
}

// chercher les Min et Max
void layer_f_fifo::scan()
{
Vmin = FLT_MAX;
Vmax = -FLT_MAX;

long long curU = Ub;
float v;
unsigned int i;

while	( curU < Ue )
	{
	i = maskfifo() & (unsigned int)curU;
	v = V[i];
	if ( v < Vmin ) Vmin = v;
	if ( v > Vmax ) Vmax = v;
	curU += 1;
	}
// printf("%g < V < %g\n", Vmin, Vmax );
}

// layer_f_fifo : les methodes qui sont virtuelles dans la classe de base
double layer_f_fifo::get_Umin()
{ return (double)Ub; }
double layer_f_fifo::get_Umax()
{ return (double)Ue; }
double layer_f_fifo::get_Vmin()
{ return (double)Vmin; }
double layer_f_fifo::get_Vmax()
{ return (double)Vmax; }

// dessin (ses dimensions dx et dy sont lues chez les parents)
// limite a l'intervalle [Ub, Ue[
void layer_f_fifo::draw( cairo_t * cai )
{
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
cairo_show_text( cai, label.c_str() );

long long curU;
double curV;
unsigned int i;

// chercher le premier point U >= u0
curU = (long long)ceil(u0);
if	( curU >= Ue )
	return;
if	( curU < Ub )
	curU = Ub;

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double curx=0, cury;
double maxx = (double)(parent->parent->ndx);

if	( style == 0 )		// style par defaut : courbe classique
	{
	i = maskfifo() & (unsigned int)curU;
	curV = V[i];				// on a le premier point ( curU, curV )
	curx =  XdeU( (double)curU );		// on le transforme
	cury = -YdeV( curV );			// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	curU += 1;
	while	( ( curx < maxx ) && ( curU < Ue ) )
		{
		i = maskfifo() & (unsigned int)curU;
		curV = V[i];
		curx =  XdeU( (double)curU );
		cury = -YdeV( curV );
		cairo_line_to( cai, curx, cury );
		curU += 1;
		}
	}
else if	( style == 1 )
	{				// style spectre en barres verticales
	double zeroy = -YdeV( 0.0 );
	while	( ( curx < maxx ) && ( curU < Ue ) )
		{
		i = maskfifo() & (unsigned int)curU;
		curV = V[i];
		curx =  XdeU( (double)curU );
		cury = -YdeV( curV );
		cairo_move_to( cai, curx, zeroy );
		cairo_line_to( cai, curx, cury );
		curU += 1;
		}
	}
cairo_stroke( cai );
}

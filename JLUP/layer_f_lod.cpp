using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include <gtk/gtk.h>
// #include <cairo-pdf.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"
#include "layer_f_lod.h"

// layer_f_lod : une courbe a pas uniforme en float (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)

void lod_f::allocMM( size_t size )	// allouer buffers min et max
{
min = (float *)malloc( 2 * size * sizeof(float) );
max = min + size;
//printf("alloc  %08x : %08x, size %d\n", (unsigned int)min, (unsigned int)max, size );
}

// allouer et calculer les LODs - un LOD est une fonction enveloppe representable par des barres verticales
//	klod1 = premiere decimation, utilisee pour passer de l'audio a la premiere enveloppe
//	klod2 = decimations ulterieures
//	maxwin = plus grande largeur de fenetre typiquement rencontree
// on cree des lods de plus en plus petits jusqu'a ce que
// la taille passe en dessous de klod2 * maxwin, alors c'est fini.
int layer_f_lod::make_lods( unsigned int klod1, unsigned int klod2, unsigned int maxwin )
{
unsigned int lodsize;
unsigned int i;		// indice source
unsigned int j;		// sous-indice decimation
unsigned int k;		// indice destination
float min=0, max=0;
lod_f * curlod, * prevlod;
// lods.reserve(16);
// ------------------------------ premiere decimation
lodsize = qu / klod1;
if	( lodsize <= maxwin )
	{ scan(); return 0; }	// meme pas besoin de lod dans ce cas
lods.push_back( lod_f() );
curlod = &lods.back();
curlod->kdec = klod1;
curlod->qc = lodsize;
// printf("lod 0 : k = %6d size = %d\n", curlod->kdec, curlod->qc );
curlod->allocMM( lodsize );
if	( ( curlod->min == NULL ) || ( curlod->max == NULL ) )
	return -1;
i = j = k = 0;
if	( qu )
	Vmin = Vmax = V[0];
while	( i < ( (unsigned int)qu - 1 ) )
	{
	if	( V[i] < Vmin ) Vmin = V[i];	// remplace scan()
	else if	( V[i] > Vmax ) Vmax = V[i];
	if	( j == 0 )
		min = max = V[i];
	else	{
		if	( V[i] < min ) min = V[i];
		else if	( V[i] > max ) max = V[i];
		}
	i += 1; j += 1;
	if	( j == klod1 )
		{		// c'est aussi le premier pt de la prochaine decimation...
		if	( V[i] < min ) min = V[i];	// c'est plus esthetique, c'est tout
		else if	( V[i] > max ) max = V[i];
		curlod->min[k] = min;
		curlod->max[k] = max;
		k += 1;
		if	( k >= lodsize )
			break;
		j = 0;
		}
	}
if	( k < lodsize )
	{
	curlod->min[k] = min;
	curlod->max[k] = max;
	++k;
	}
// printf("lod 0 : finu size = %d(%d)\n", curlod->qc, k );
// printf("fini @ %08x : %08x, size %d\n", (unsigned int)curlod->min,
//					   (unsigned int)curlod->max, curlod->qc );
// ------------------------------ decimations suivantes
while	( ( lodsize = lodsize / klod2 ) > maxwin )
	{
	lods.push_back( lod_f() );
	// prevlod = curlod;				// marche PO curlod est invalide
	prevlod = &lods.at( lods.size() - 2 );		// APRES le push_back sinon t'es DEAD
	curlod = &lods.back();
	curlod->kdec = prevlod->kdec * klod2;
	curlod->qc = lodsize;
	// printf("lod %d : k = %6d size = %d\n", (int)lods.size()-1, curlod->kdec, curlod->qc );
	curlod->allocMM( lodsize );
	if	( ( curlod->min == NULL ) || ( curlod->max == NULL ) )
		return -1;
	i = j = k = 0;
	while	( i < (unsigned int)prevlod->qc )
		{
		if	( j == 0 )
			{ min = prevlod->min[i]; max = prevlod->max[i]; }
		else	{
			if	( prevlod->min[i] < min ) min = prevlod->min[i];
			if	( prevlod->max[i] > max ) max = prevlod->max[i];
			}
		i += 1; j += 1;
		if	( j == klod2 )
			{
			curlod->min[k] = min;
			curlod->max[k] = max;
			k += 1;
			if	( k >= lodsize )
				break;
			j = 0;
			}
		}
	if	( k < lodsize )
		{
		curlod->min[k] = min;
		curlod->max[k] = max;
		++k;
		}
	// printf("lod   : fini size = %d(%d)\n", curlod->qc, k );
	}
return 0;
}

void layer_f_lod::scan()
{
if	( qu )
	Vmin = Vmax = V[0];
for	( int i = 1; i < qu; ++i )
	{
	if	( V[i] < Vmin ) Vmin = V[i];
	else if	( V[i] > Vmax ) Vmax = V[i];
	}
}

// chercher le premier point X >= X0
int layer_f_lod::goto_U( double U0 )
{
curi = (int)ceil(U0);
if	( curi < 0 )
	curi = 0;
if	( curi < qu )
	return 0;
else	return -1;
}

void layer_f_lod::goto_first()
{
curi = 0;
}

// get XY then post increment
int layer_f_lod::get_pi( double & rU, double & rV )
{
if ( curi >= qu )
   return -1;
rU = (double)curi; rV = (double)V[curi]; ++curi;
return 0;
}

// les methodes qui sont virtuelles dans la classe de base
void layer_f_lod::refresh_proxy()
{
ilod = -2;
layer_base::refresh_proxy();
}

double layer_f_lod::get_Umin()
{ return (double)0; }
double layer_f_lod::get_Umax()
{ return (double)(qu-1); }
double layer_f_lod::get_Vmin()
{ return (double)Vmin; }
double layer_f_lod::get_Vmax()
{ return (double)Vmax; }

// choisir le LOD optimal (ilod) en fonction du zoom horizontal c'est a dire ( u1 - u0 )
void layer_f_lod::find_ilod()
{
double u1, maxx, spp=100000.0;

maxx = (double)(parent->parent->ndx);
u1 = UdeX( maxx );
// commencer par le LOD le plus grossier, affiner jusqu'a avoir assez de samples
// pour un bon effet visuel
ilod = lods.size() - 1;
while	( ilod >= 0 )
	{
	spp = ( u1 - u0 ) / ( lods.at(ilod).kdec * maxx );	// samples par pixel
	if	( spp >= spp_max )
		break;
	ilod -= 1;
	}
if	( ilod < 0 )
	spp = ( u1 - u0 ) / maxx;
// printf("choix lod %d, spp = %g\n", ilod, spp );
}

// dessin partiel de tU0 a tU1
void layer_f_lod::draw( cairo_t * cai, double tU0, double tU1 )
{
// printf("layer_f_lod::begin draw\n");
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_set_line_width( cai, linewidth );
if	( tU0 == u0 )
	{
	cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
	cairo_show_text( cai, label.c_str() );
	}

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double tU, tV, curx, cury;

if	( ilod < -1 )
	find_ilod();

if	( ilod < 0 )
	{			// affichage de courbe classic
	if ( goto_U( tU0 ) )
	   return;
	if ( get_pi( tU, tV ) )
	   return;
	// on a le premier point ( tU, tV )
	curx =  XdeU( tU );			// les transformations
	cury = -YdeV( tV );			// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	int cnt = 0;
	while ( get_pi( tU, tV ) == 0 )
	   {
	   curx =  XdeU( tU );		// les transformations
	   cury = -YdeV( tV );
	   cairo_line_to( cai, curx, cury );
	   if ( tU >= tU1 )	// anciennement if ( curx >= maxx )
	      break;
	   if	( ++cnt >= 4000 )
		{
		cairo_stroke( cai );
		// printf("courbe %d lines\n", cnt );
		cnt = 0;
		cairo_move_to( cai, curx, cury );
		}
	   }
	if	( cnt )
		cairo_stroke( cai );
	// printf("courbe %d lines\n", cnt );
	}
else	{			// affichage enveloppe
	lod_f * curlod = &lods.at(ilod);
	int qc = curlod->qc;
	int k = curlod->kdec;
	int i0, i1, i;		// indices source
	i0 = (int)ceil(tU0) / k;
	i1 = (int)floor(tU1) / k;
	if	( i0 < 0 ) i0 = 0;
	if	( i1 > ( qc - 1 ) ) i1 = qc - 1;
	for	( i = i0; i < i1; ++i )
		{
		curx =  XdeU( i * k );		// les transformations
		cury = -YdeV( curlod->min[i] );
		cairo_move_to( cai, curx, cury );
		cury = -YdeV( curlod->max[i] );
		cairo_line_to( cai, curx, cury );
		// cury2 = -v2y( curlod->min[i] );	// ainsi normalement cury2 > cury
		// cury  = -v2y( curlod->max[i] );
		// if	( cury2 < ( cury + linewidth ) ) // epaisseur ligne horizontale
		//	cury2 = cury + linewidth;	 // coherente avec les verticales
		// cairo_move_to( cai, curx, cury );
		// cairo_line_to( cai, curx, cury2 );
		}
	cairo_stroke( cai );
	// printf("enveloppe %d lines\n", i1 - i0 );
	}
// printf("end layer_f_lod::draw\n");
}

// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_f_lod::draw( cairo_t * cai )
{
double u1 = UdeX( (double)(parent->parent->ndx) );
draw( cai, u0, u1 );
// printf("d lod %d\n", ilod ); fflush(stdout);
}

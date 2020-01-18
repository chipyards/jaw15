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

// layer_s16_lod : une courbe a pas uniforme en signed 16 bits (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)
// l'allocation mémoire est securité zéro

void lod::allocMM( size_t size )	// ebauche de service d'allocation
{
min = (short *)malloc( 2 * size * sizeof(short) );
max = min + size;
//printf("alloc  %08x : %08x, size %d\n", (unsigned int)min, (unsigned int)max, size );
}

// allouer et calculer les LODs - un LOD est une fonction enveloppe representable par des barres verticales
//	klod1 = premiere decimation, utilisee pour passer de l'audio a la premiere enveloppe
//	klod2 = decimations ulterieures
//	minwin = plus petite largeur de fenetre normalement supportee
// on cree des lods de plus en plus petits jusqu'a ce que
// la taille passe en dessous de klod2 * minwin, alors c'est fini.
int layer_s16_lod::make_lods( unsigned int klod1, unsigned int klod2, unsigned int minwin )
{
unsigned int lodsize;
unsigned int i;		// indice source
unsigned int j;		// sous-indice decimation
unsigned int k;		// indice destination
short min=0, max=0;
lod * curlod, * prevlod;
// lods.reserve(16);
// ------------------------------ premiere decimation
lodsize = qu / klod1;
if	( lodsize <= minwin )
	return 0;		// meme pas besoin de lod dans ce cas
lods.push_back( lod() );
curlod = &lods.back();
curlod->kdec = klod1;
curlod->qc = lodsize;
printf("lod 0 : k = %6d size = %d\n", curlod->kdec, curlod->qc );
curlod->allocMM( lodsize );
if	( ( curlod->min == NULL ) || ( curlod->max == NULL ) )
	return -1;
i = j = k = 0;
while	( i < ( (unsigned int)qu - 1 ) )
	{
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
//if	( k != lodsize )
//	return -10;
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
while	( ( lodsize = lodsize / klod2 ) > minwin )
	{
	lods.push_back( lod() );
	// prevlod = curlod;				// marche PO curlod est invalide
	prevlod = &lods.at( lods.size() - 2 );		// APRES le push_back sinon t'es DEAD
	curlod = &lods.back();
	curlod->kdec = prevlod->kdec * klod2;
	curlod->qc = lodsize;
	printf("lod %d : k = %6d size = %d\n", lods.size()-1, curlod->kdec, curlod->qc );
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
	if	( k != lodsize )
		return -11;
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

// allouer la memoire
void layer_s16_lod::allocV( size_t size )
{
V = (short *)malloc( size * sizeof(short) );
if	( V != NULL )
	qu = size;
else	qu = 0;
}

// chercher le premier point X >= X0
int layer_s16_lod::goto_U( double U0 )
{
curi = (int)ceil(U0);
if	( curi < 0 )
	curi = 0;
if	( curi < qu )
	return 0;
else	return -1;
}

void layer_s16_lod::goto_first()
{
curi = 0;
}

// get XY then post increment
int layer_s16_lod::get_pi( double & rU, double & rV )
{
if ( curi >= qu )
   return -1;
rU = (double)curi; rV = (double)V[curi]; ++curi;
return 0;
}

// layer_s16 : les methodes qui sont virtuelles dans la classe de base
void layer_s16_lod::refresh_proxy()
{
ilod = -2;
layer_base::refresh_proxy();
}

double layer_s16_lod::get_Umin()
{ return (double)0; }
double layer_s16_lod::get_Umax()
{ return (double)(qu-1); }
double layer_s16_lod::get_Vmin()
{ return (double)-32767; }
double layer_s16_lod::get_Vmax()
{ return (double)32767; }

// choisir le LOD optimal (ilod) en fonction du zoom horizontal c'est a dire ( u1 - u0 )
void layer_s16_lod::find_ilod()
{
double u1, maxx, spp=100000.0;

maxx = (double)(parent->parent->ndx);
u1 = UdeX( maxx );
// commencer par le LOD le plus grossier, affiner jusqu'a avoir assez de samples
// pour un bon effet visuel
ilod = lods.size() - 1;
while	( ilod >= 0 )
	{
	spp = ( u1 - u0 ) / ( lods.at(ilod).kdec * maxx );		// samples par pixel
	if	( spp >= spp_max )
		break;
	ilod -= 1;
	}
if	( ilod < 0 )
	spp = ( u1 - u0 ) / maxx;
printf("choix lod %d, kdec = %d, spp = %g\n",
	ilod, (ilod>=0)?(lods.at(ilod).kdec):(1), spp );
}


// dessin partiel de tU0 a tU1
void layer_s16_lod::draw( cairo_t * cai, double tU0, double tU1 )
{
// printf("layer_s16_lod::begin draw\n");
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
	lod * curlod = &lods.at(ilod);
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
		}
	cairo_stroke( cai );
	// printf("enveloppe %d lines\n", i1 - i0 );
	}
// printf("end layer_s16_lod::draw\n");
}

// dessin (ses dimensions dx et dy sont lues chez les parents)
void layer_s16_lod::draw( cairo_t * cai )
{
double u1 = UdeX( (double)(parent->parent->ndx) );
draw( cai, u0, u1 );
printf("d lod %d\n", ilod ); fflush(stdout);
/*
double u, du;				// test
du = 0.1 * ( u1 - u0 );			// test
u = u0;					// test
while	( u < u1 )			// test
	{				// test
	draw( cai, u, u+du );		// test
	printf("d %g to %g\n", u, u+du );// test
	u += du;			// test
	}				// test
fflush(stdout);
*/
}

// layer_f : une courbe a pas uniforme en float (classe derivee de layer_base)
// layer_f : les methodes ajoutes a la classe de base

// allouer la memoire
void layer_f::allocV( size_t size )
{
V = (float *)malloc( size * sizeof(float) );
if	( V != NULL )
	qu = size;
else	qu = 0;
}

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

// layer_rgb : une image RGB telle qu'un spectrogramme (classe derivee de layer_base)
// U est en FFT-runs
// V est en bins logarithmises (une fraction de demi-ton)

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

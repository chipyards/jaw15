#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>

#include "fir.h"
#include "imago.h"


// lire pix1
int imago::read( const char * fnam )
{
if	( pix1 )
	gdk_pixbuf_unref( pix1 );
pix1 = gdk_pixbuf_new_from_file( fnam, NULL);
if	( pix1 == NULL )
	return 1;
return 0;
}

// sauver pix2
int imago::save_png( const char * fnam )
{
if	( pix2 == NULL )
	return 2;
if	( gdk_pixbuf_save( pix2, fnam, "png", NULL, NULL ) )
	{ printf("saved %s\n", fnam ); fflush(stdout); }
else	{ printf("NOT saved %s\n", fnam ); fflush(stdout); return 1; }
return 0;
}

// filtrage 2D , une composante à la fois, chacune en 2 etapes orthogonales
void imago::filter( fir * zefir )
	{
	lefir = zefir;
// src pixbuf
	unsigned int sw, sh, sch, sstride;
	unsigned char * sdata;
	sw = gdk_pixbuf_get_width( pix1 );
	sh = gdk_pixbuf_get_height( pix1 );
	sch = gdk_pixbuf_get_n_channels( pix1 );
	sstride = gdk_pixbuf_get_rowstride( pix1 );
	sdata = gdk_pixbuf_get_pixels( pix1 );
// dst pixbuf
	unsigned int dw, dh, dch, dstride;
	unsigned char * ddata;
	dw = sw;
	dh = sh;
	dch = 3;
	pix2 = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, dw, dh );
	// gdk se debrouille pour calculer le stride et allouer data
	dstride = gdk_pixbuf_get_rowstride( pix2 );
	ddata = gdk_pixbuf_get_pixels( pix2 );
	if	( ddata == NULL )
		{ printf("gdk_pixbuf_new() failed\n"); exit(1); }
// deux buffers en double pour ping-pong
	if	( Abuf ) free( Abuf );
	Abuf = (double *)malloc( sw * sh * sizeof(double) );
	if	( Abuf == NULL )
		{ printf("malloc fails\n"); exit(1); }
	if	( Bbuf ) free( Bbuf );
	Bbuf = (double *)malloc( sw * sh * sizeof(double) );
	if	( Bbuf == NULL )
		{ printf("malloc fails\n"); exit(1); }
// boucle des components R, G, B
	for	( int ic = 0; ic < 3; ic++ )
		{
		// copier le plan src dans Abuf
		unsigned int di = 0;
		for	( unsigned int y = 0; y < sh; ++y )
			{
			for	( unsigned int x = 0; x < sw; ++x )
				{
				unsigned int a = y * sstride + x * sch + ic;
				Abuf[di++] = double(sdata[a]-128);	// 0.0 pour gris median
				}
			}
		// filtrer horizontalement de Abuf vers Bbuf
		filter_one_planeH( Bbuf, Abuf, sw, sh );
		// filtrer verticalement de Bbuf vers Abuf
		filter_one_planeV( Abuf, Bbuf, sw, sh );
		// copier Abuf dans le plan dest
		unsigned int si = 0;
		for	( unsigned int y = 0; y < dh; ++y )
			{
			for	( unsigned int x = 0; x < dw; ++x )
				{
				unsigned int a = y * dstride + x * dch + ic;
				int val = 128 + (int)round(Abuf[si++]);	// 128 pour gris median
				if	( val < 0 ) val = 0;
				if	( val > 255 ) val = 255;
				ddata[a] = val;
				}
			}
		}
	printf("image filtering done...\n");
	}

// resampling 2D , une composante à la fois, chacune en 2 etapes orthogonales
// le coeff de resampling Kr est applique tel quel, alors W et H de l'image de sortie
// sont approximes par defaut
// Kr > 1 ==> image plus petite (decimation)
void imago::resamp( fir * zefir )
	{
	lefir = zefir;
// src pixbuf
	unsigned int sw, sh, sch, sstride;
	unsigned char * sdata;
	sw = gdk_pixbuf_get_width( pix1 );
	sh = gdk_pixbuf_get_height( pix1 );
	sch = gdk_pixbuf_get_n_channels( pix1 );
	sstride = gdk_pixbuf_get_rowstride( pix1 );
	sdata = gdk_pixbuf_get_pixels( pix1 );
// dst pixbuf
	unsigned int dw, dh, dch, dstride;
	unsigned char * ddata;
	double ddw = double(sw) / lefir->Kr;
	double ddh = double(sh) / lefir->Kr;
	dw = (unsigned int)floor(ddw);
	dh = (unsigned int)floor(ddh);
	printf("output dimensions %g x %g rounded to %d x %d\n", ddw, ddh, dw, dh );
	dch = 3;
	pix2 = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, dw, dh );
	// gdk se debrouille pour calculer le stride et allouer data
	dstride = gdk_pixbuf_get_rowstride( pix2 );
	ddata = gdk_pixbuf_get_pixels( pix2 );
	if	( ddata == NULL )
		{ printf("gdk_pixbuf_new() failed\n"); exit(1); }
// 3 buffers en double, pour 1 composante
	if	( Abuf ) free( Abuf );
	Abuf = (double *)malloc( sw * sh * sizeof(double) );
	if	( Abuf == NULL )
		{ printf("malloc fails\n"); exit(1); }
	if	( Bbuf ) free( Bbuf );
	Bbuf = (double *)malloc( dw * sh * sizeof(double) );
	if	( Bbuf == NULL )
		{ printf("malloc fails\n"); exit(1); }
	if	( Cbuf ) free( Bbuf );
	Cbuf = (double *)malloc( dw * dh * sizeof(double) );
	if	( Cbuf == NULL )
		{ printf("malloc fails\n"); exit(1); }
// correction pour decimation (abaissement de la bande passante) :
	if	( lefir->Kr > 1 )
		{ lefir->dA /= lefir->Kr; lefir->pispan *= lefir->Kr; }
	lefir->init_window();
// boucle des components R, G, B
	for	( int ic = 0; ic < 3; ic++ )
		{
		// copier le plan src dans Abuf
		unsigned int di = 0;
		for	( unsigned int y = 0; y < sh; ++y )
			{
			for	( unsigned int x = 0; x < sw; ++x )
				{
				unsigned int a = y * sstride + x * sch + ic;
				Abuf[di++] = double(sdata[a]-128);	// 0.0 pour gris median
				}
			}
		// resampler horizontalement de Abuf vers Bbuf
		resamp_one_planeH( Bbuf, Abuf, dw, sw, sh );
		// filtrer verticalement de Bbuf vers Abuf
		resamp_one_planeV( Cbuf, Bbuf, dw, dh, sh );
		// copier Cbuf dans le plan dest
		unsigned int si = 0;
		for	( unsigned int y = 0; y < dh; ++y )
			{
			for	( unsigned int x = 0; x < dw; ++x )
				{
				unsigned int a = y * dstride + x * dch + ic;
				int val = 128 + (int)round(Cbuf[si++]);	// 128 pour gris median
				if	( val < 0 ) val = 0;
				if	( val > 255 ) val = 255;
				ddata[a] = val;
				}
			}
		}
	printf("image resampling done...\n");
	}

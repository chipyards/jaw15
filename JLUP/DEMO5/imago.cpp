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
				// int val = 128 + (int)round(Bbuf[si++]);	// 128 pour gris median
				if	( val < 0 ) val = 0;
				if	( val > 255 ) val = 255;
				ddata[a] = val;
				}
			}
		}
	printf("image filtering done...\n");
	}

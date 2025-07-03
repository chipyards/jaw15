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
w1 = gdk_pixbuf_get_width(pix1);
h1 = gdk_pixbuf_get_height(pix1);
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

// traitement pix1 -> pix2 derive de SCANMAN
int imago::filter( fir * zefir )
	{
	// GdkPixbuf * tmp1;	// image tmp
	// pix2 = gdk_pixbuf_copy( pix1 );	// faire une copie pour ne pas ecraser pix1
	// printf("copy done...");
	// pix2 = gdk_pixbuf_rotate_simple( pix1, (GdkPixbufRotation)90 );
	// printf("rotation done...");

// src pixbuf
	unsigned int sw, sh, sch, sstride,
		     sx, sy, sa;
	unsigned char * sdata;
	sw = gdk_pixbuf_get_width( pix1 );
	sh = gdk_pixbuf_get_height( pix1 );
	sch = gdk_pixbuf_get_n_channels( pix1 );
	sstride = gdk_pixbuf_get_rowstride( pix1 );
	sdata = gdk_pixbuf_get_pixels( pix1 );
// dst pixbuf
	unsigned int dw, dh, dch, dstride,
		     dx, dy, da;
	unsigned char * ddata;
	dw = sw;
	dh = sh;
	dch = 3;
	pix2 = gdk_pixbuf_new( GDK_COLORSPACE_RGB, 0, 8, dw, dh );
	// gdk se debrouille pour calculer le stride et allouer data
	dstride = gdk_pixbuf_get_rowstride( pix2 );
	ddata = gdk_pixbuf_get_pixels( pix2 );
	if	( ddata == NULL )
		return -1;
// buffers pour filtrage
	unsigned int span = dw;
	if	( span < dh ) span = dh;
	if	( span < sw ) span = sw;
	if	( span < sh ) span = sh;
	double * Sbuf = (double *)malloc( span * sizeof(double) );
	if	( Sbuf == NULL )
		return -2;
// process
	int lumy;
	/* scan ligne par ligne */
	for	( dy = 0; dy < dh; ++dy )
		{
		sy = dy;
		// remplissage buffer source
		for	( sx = 0; sx < sw; ++sx )
			{
			sa = sy * sstride + sx * sch;
			lumy = sdata[sa] + sdata[sa+1]+ sdata[sa+2];
			lumy /= 3;
			Sbuf[sx] = double(lumy-128);	// 0.0 pour gris median
			}
		// filtrage
		// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
		double K = zefir->getK0();
		int j, j0, k;
		double sum;
		j0 = (zefir->qfir-1)/ 2;	// qfir est impair (sauf si A0 != 0.0)
		for	( dx = 0; dx < dw; ++dx )
			{
			sum = 0.0;
			for	( j = 0; j < (int)zefir->qfir; ++j )
				{
				k = dx + j - j0 ;
				if	( ( k >= 0 ) && ( k < (int)sw ) )
					sum += Sbuf[k] * zefir->FIRbuf[j];
				}
			int isum = 128 + round( sum * K );
			if	( isum < 0 ) isum = 0;
			if	( isum > 255 ) isum = 255;
			da = dy * dstride + dx * dch;
			ddata[da] = (unsigned char)isum;
			ddata[da+1] = ddata[da+2] = ddata[da];
			}
		printf("y"); fflush(stdout);
		}
	//*/
	/* scan colonne par colonne *
	for	( dx = 0; dx < dw; ++dx )
		{
		sx = dx;
		// remplissage buffer source
		for	( sy = 0; sy < sh; ++sy )
			{
			sa = sy * sstride + sx * sch;
			lumy = sdata[sa] + sdata[sa+1]+ sdata[sa+2];
			lumy /= 3;
			Sbuf[sy] = double(lumy-128);	// 0.0 pour gris median
			}
		// filtrage
		// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
		double K = zefir->getK0();
		int j, j0, k;
		double sum;
		j0 = (zefir->qfir-1)/ 2;	// qfir est impair (sauf si A0 != 0.0)
		for	( dy = 0; dy < dh; ++dy )
			{
			sum = 0.0;
			for	( j = 0; j < (int)zefir->qfir; ++j )
				{
				k = dy + j - j0 ;
				if	( ( k >= 0 ) && ( k < (int)sh ) )
					sum += Sbuf[k] * zefir->FIRbuf[j];
				}
			int isum = 128 + round( sum * K );
			if	( isum < 0 ) isum = 0;
			if	( isum > 255 ) isum = 255;
			da = dy * dstride + dx * dch;
			ddata[da] = (unsigned char)isum;
			ddata[da+1] = ddata[da+2] = ddata[da];
			}
		printf("x"); fflush(stdout);
		}
	//*/
	printf("image filtering done...\n");
	return 0;
	}

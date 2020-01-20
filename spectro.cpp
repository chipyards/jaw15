#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fftw3.h"

#include "spectro.h"

/** FFT window functions ======================================================= */

// precalcul de fenetre pour FFT type raised cosine
// couvre les classiques hann, hamming, blackman, blackmanharris, cf spectro.h
void spectro::window_precalc( double a0, double a1, double a2, double a3 )
{
double m = M_PI / fftsize;	// on doit diviser par ffftsize ou (fftsize-1) ??????
for	( unsigned int i = 0; i < fftsize; ++i )
	{
        window[i] = a0
		- a1 * cos( 2 * m * i )
		+ a2 * cos( 4 * m * i )
		- a3 * cos( 6 * m * i );
	}
}

void spectro::window_dump()
{
for	( unsigned int i = 0; i < fftsize; ++i )
	printf("%g\n", window[i] );
}

/** FFTW3 functions ======================================================== */

// allouer les buffers pour la fft
// on alloue large, selon FFTSIZEMAX, pour pouvoir changer fftsize a chaud
// fftw3 recommande d'utiliser leur fonction fftwf_malloc() plutot que malloc() ou []
int spectro::alloc_fft()
{
// verifications preliminaires
if	( fftsize > FFTSIZEMAX )	return -1;
if	( fftstride < 32 )		return -2;
unsigned int size;
// allouer buffer pour entree fft
if	( fftinbuf == NULL )
	{
	size = FFTSIZEMAX;
	fftinbuf = (float *)fftwf_malloc( size * sizeof(float) );	// pour reel float
	if	( fftinbuf == NULL )
		return -20;
	printf("fftw alloc %d floats, align %04X\n", size, (unsigned int)fftinbuf & 0x0FFFF );
	}
// allouer buffer pour sortie fft
if	( fftoutbuf == NULL )
	{
	size = ( (FFTSIZEMAX/2) + 1 ) * 2;
	fftoutbuf = (float *)fftwf_malloc( size * sizeof(float) );	// pour complex float
	if	( fftoutbuf == NULL )
		return -21;
	printf("fftw alloc %d floats, align %04X\n", size, (unsigned int)fftoutbuf & 0x0FFFF );
	}
// preparer le plan FFTW
p = fftwf_plan_dft_r2c_1d( fftsize, fftinbuf, (fftwf_complex*)fftoutbuf, FFTW_MEASURE );
if	( p == NULL )
	return -30;
printf("FFTW plan ready\n");
return 0;
}

// calcul des parametres derives
void spectro::parametrize( unsigned int fsamp, unsigned int qsamples )
{
// les dimensions du spectre
W = ( ( qsamples - fftsize ) / fftstride ) + 1;		// le nombre de fenetres fft
if	( fmax > (fsamp/2) )
	fmax = (fsamp/2);
H = ( fmax * fftsize ) / fsamp;				// le nombre de bins effectivement utilises
}

// allouer ou re-allouer buffer pour spectre fini
int spectro::alloc_WH()
{
unsigned int newsize = W * H;	// en pixels
if	( spectre == NULL )
	{
	spectre = (unsigned short *)malloc( newsize * sizeof(short int) );
	if	( spectre == NULL )
		return -30;
	allocatedWH = newsize;
	printf("spectre alloc %u pixels (%ux%u)\n", newsize, W, H );
	}
else if	( newsize > allocatedWH )
	{
	spectre = (unsigned short *)realloc( spectre, newsize * sizeof(short int) );
	if	( spectre == NULL )
		return -31;
	allocatedWH = newsize;
	printf("spectre realloc %u pixels (%ux%u)\n", newsize, W, H );
	}
return 0;
}


/** top actions ========================================================== */

// enchaine toutes les initialisations sauf fill_palette()
int spectro::init( unsigned int fsamp, unsigned int qsamples )
{
unsigned int retval;
// preparer la fenetre
window_hann();

// preparer le travail de fftw3
retval = alloc_fft();
if	( retval )
	return retval;

// preparer le spectre
parametrize( fsamp, qsamples );
retval = alloc_WH();
if	( retval )
	return retval;
return 0;
}

// enfin !
void spectro::compute( float * src )
{
unsigned int a, j;
float k, kn, mag, maxmag = 0.0;
unsigned short u;
umax = 0;
k = 65536.0;	// le but de ce coeff est d'avoir des magnitudes compatibles avec une conversion en u16
		// ici on traite de l'audio normalise [-1.0, 1.0]
// k = 1.0;	// coeff de JAW04b qui traite de l'audio s16
k *= (2.0/(float)fftsize);	// correction DFT classique, selon JLN
for	( unsigned int icol = 0; icol < W; ++icol )
	{
	// fenetrage sur fftinbuf
	a = icol * fftstride;
	for	( j = 0; j < fftsize; ++j )
		fftinbuf[j] = src[a++] * window[j];
	// fft de fftinbuf vers fftoutbuf
	fftwf_execute(p);
	// calcul magnitudes sur place (fftoutbuf) et application facteur k
	a = 0;
	for	( j = 0; j < H; ++j )
		{
		mag = k * hypotf( fftoutbuf[a], fftoutbuf[a+1] );
		a += 2;
		fftoutbuf[j] = mag;
		if	( mag > maxmag )
			maxmag = mag;
		}
	// normalisation
	kn = 65535.0 / maxmag;
	// conversion en unsigned short int pour palettisation
	a = icol * H;
	for	( j = 0; j < H; ++j )
		{
		u = (unsigned short)floor( kn * fftoutbuf[j] );
		if	( u > umax )
			umax = u;
		spectre[a++] = u;
		}
	}
printf("max magnitude %g --> u16 %u\n", maxmag, umax );
}

// iend est l'index a partir duquel on est sature au max
void spectro::fill_palette( unsigned int iend )
{
unsigned int i, mul = ( 1 << 24 ) / iend;
unsigned char val = 0;
// 2 zones
unsigned int izon = iend/2;
for	( i = 0; i < izon; ++i )
	{
	val = ( i * mul ) >> 16;
	palR[i] = 0;
	palG[i] = val;
	palB[i] = val/4;
	}
for	( i = izon; i < iend; ++i )
	{
	val = ( i * mul ) >> 16;
	palR[i] = val;
	palG[i] = val/4;
	palB[i] = 0;
	}
// completer la zone de saturation
memset( palR + iend, palR[i-1], 65536 - iend );
memset( palG + iend, palG[i-1], 65536 - iend );
memset( palB + iend, palB[i-1], 65536 - iend );
// special verif echelle verticale
// palR[0xFFFF] = 0; palG[0xFFFF] = palB[0xFFFF] = 0xFF;
}

// conversion en style GDK pixbuf
// N.B. spectro ne connait pas GDK mais est compatible avec le style
// utilise la palette interne du spectro
void spectro::spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels )
{
unsigned int x, y, destadr, srcadr, i;
for	( y = 0; y < H; y++ )
	{
	// si on veut afficher le pixbuf directement avec GDK, il faut gerer Y+ vers le bas (origine en haut)
	// destadr = ( (H-1) - y ) * RGBstride;
	// si on veut afficher le pixbuf avec JLUPLOT, il faut gerer Y+ vers le haut (origine en bas)
	destadr = y * RGBstride;
	srcadr = y;
	for	( x = 0; x < W; x++ )
		{
		i = spectre[srcadr];
		RGBdata[destadr]   = palR[i];
		RGBdata[destadr+1] = palG[i];
		RGBdata[destadr+2] = palB[i];
		destadr += channels;
		srcadr += H;
		}
	}
}


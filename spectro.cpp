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

/** log spectrum functions ================================================= */

double spectro::log_fis( double fid )
{
return( relog_fbase * exp2( relog_opp * fid ) );
}

// calcul des parametres derives
void spectro::parametrize( unsigned int fsamp, unsigned int qsamples )
{
// les parametres internes pour le resampling log
relog_opp = 1.0 / (double)( bpst * 12 );
relog_fbase = midi2Hz( midi0 ) ;			// en Hz
relog_fbase /= ( (double)fsamp / (double)fftsize );	// en pitch spectro
// les dimensions du spectre log
W = ( ( qsamples - fftsize ) / fftstride ) + 1;		// le nombre de fenetres fft
H = octaves * 12 * bpst;				// le nombre de frequences apres resamp
}

// precalcul des points de resampling pour convertir le spectre en echelle log
// remplir log_resamp en fonction de relog_opp et relog_fbase
void spectro::log_resamp_precalc()
{
int id;		// index destination, dans le spectre resample
int iis0, iis1;	// indexes source bruts
int ndec;	// nombre de points sujets a decimation
double fis0;	// index source fractionnaire borne inferieure, correspondant à id - 0.5
double fis1;	// index source fractionnaire borne superieure, correspondant à id + 0.5
double fis;	// index source fractionnaire median, correspondant à id
fis0 = log_fis( -0.5 );		// pour id = 0
for	( id = 0; id < (int)H; ++id )
	{
	fis1 = log_fis( double(id) + 0.5 );
	iis0 = 1 + (int)floor( fis0 );		// exclusive ceiling
	iis1 = (int)floor( fis1 );		// inclusive floor
	if	( iis1 >= (int)fftsize )
		{ log_resamp[id].decimflag = -1; break; }	// internal error
	ndec = ( iis1 - iis0 ) + 1;
	if	( ndec >= 1 )
		{				// decimation
		log_resamp[id].decimflag = 1;
		log_resamp[id].is0 = iis0;
		log_resamp[id].is1 = iis1;
		}
	else if	( ndec == 0 )
		{				// interpolation
		log_resamp[id].decimflag = 0;
		log_resamp[id].is0 = iis1;
		log_resamp[id].is1 = iis0;
		fis = log_fis( double(id) );
		log_resamp[id].k1 = fis - floor(fis);
		if	( floor(fis) != (double)log_resamp[id].is0 )
			log_resamp[id].decimflag = -2;	// internal error
		log_resamp[id].k0 = 1.0 - log_resamp[id].k1;
		}
	else	log_resamp[id].decimflag = -3;		// internal error
	fis0 = fis1;	// pour le prochain
	}
}

void spectro::log_resamp_dump()
{
printf("echelle %g pix/sm\n", ( 1.0 / relog_opp ) / 12.0 );
printf("base %g\n", relog_fbase );
for	( int id = 0; id < (int)H; ++id )
	{
	printf("%4d { %7g [ %5d ( %7g ) %5d ] %7g } ", id,
		log_fis( id - 0.5 ), log_resamp[id].is0, log_fis( id ),
				     log_resamp[id].is1, log_fis( id + 0.5 ) );
	if	( log_resamp[id].decimflag == 1 )
		printf("D %d\n", log_resamp[id].is1 - log_resamp[id].is0 + 1 );
	else if	( log_resamp[id].decimflag == 0 )
		printf("I { %g %g }\n", log_resamp[id].k0, log_resamp[id].k1 );
	else	{
		printf("log_resamp internal error %d\n", log_resamp[id].decimflag );
		break;
		}
	}
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
	printf("spectre alloc %d pixels\n", newsize );
	}
else if	( newsize > allocatedWH )
	{
	spectre = (unsigned short *)realloc( spectre, newsize * sizeof(short int) );
	if	( spectre == NULL )
		return -31;
	allocatedWH = newsize;
	printf("spectre realloc %d pixels\n", newsize );
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

// preparer le resampling log
parametrize( fsamp, qsamples );
log_resamp_precalc();
retval = alloc_WH();
if	( retval )
	return retval;
return 0;
}

// enfin !
void spectro::compute( float * src )
{
unsigned int a, j;
float k, mag, maxmag = 0.0;
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
	// calcul magnitudes sur place (fftoutbuf)
	a = 0;
	for	( j = 0; j <= ( fftsize / 2 ); ++j )
		{
		mag = hypotf( fftoutbuf[a], fftoutbuf[a+1] );
		a += 2;
		fftoutbuf[j] = mag;
		if	( mag > maxmag )
			maxmag = mag;
		}
	/* resampling (lineaire, provisoire !) fftoutbuf vers fftinbuf *
	a = 0;
	for	( j = 0; j < H; ++j )
		{
		a += 1;
		fftinbuf[j] = fftoutbuf[a] * k;
		}
	//*/
	// resampling log, de fftoutbuf vers fftinbuf
	logpoint *p; float peak;
	for	( j = 0; j < H; ++j )
		{
		p = &log_resamp[j];
		if	( p->decimflag == 1 )	// decimation par valeur pic
			{
			peak = 0.0;
			for	( a = p->is0; a <= (unsigned int)p->is1; ++a )
				{
				if	( fftoutbuf[a] > peak )
					peak = fftoutbuf[a];
				}
			}
		else if	( p->decimflag == 0 )	// interpolation lineaire
			{
			peak = fftoutbuf[p->is0] * p->k0 + fftoutbuf[p->is1] * p->k1;
			}
		else	peak = 0.0;
		fftinbuf[j] = peak * k;
		}
	// conversion en unsigned short int pour palettisation
	a = icol * H;
	for	( j = 0; j < H; ++j )
		{
		u = (unsigned short)fftinbuf[j];
		if	( u > umax )
			umax = u;
		spectre[a++] = u;
		}
	/* bricolage pour verif echelle verticale : trace une ligne horizontale pour chaque octave */
	int ech = (int)(1.0 / relog_opp);
	a = icol * H;
	for	( j = 0; j < H; j+= ech )
		if	( spectre[a+j] < 200 )
			spectre[a+j] = 0xFFFF;
	//*/
	}
printf("max magnitude %g --> u16 %u\n", maxmag * k, umax );
}

void spectro::fill_palette( unsigned int iend )
{
unsigned int mul = ( 1 << 24 ) / iend;
unsigned char val = 0;
for	( unsigned int i = 0; i < iend; ++i )
	{
	val = ( i * mul ) >> 16;
	palR[i] = val;
	palG[i] = val;
	palB[i] = 69;
	}
// completer la zone de saturation
memset( palR + iend, val, 65536 - iend );
memset( palG + iend, val, 65536 - iend );
memset( palB + iend, val, 65536 - iend );
// special verif graduations
palR[0xFFFF] = 0; palG[0xFFFF] = palB[0xFFFF] = 0xFF;
}

// conversion en style GDK pixbuf
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


// utility functions
double midi2Hz( int midinote )
{	// knowing that A4 = 440 Hz = note 69
return 440.0 * pow( 2.0, ( ( midinote - 69 ) / 12.0 ) );
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fftw3.h"

#include "spectro.h"

/** FFT window functions ======================================================= */

// precalcul de fenetre pour FFT type raised cosine
// couvre les classiques 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
// rend la moyenne
double spectro::window_precalc( int ft_window )
{
double a0, a1, a2, a3, moy;
switch	( ft_window )
	{
	case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
	case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
	case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
	case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
	default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
	}
moy = 0.0;
double m = M_PI / fftsize;	// on doit diviser par ffftsize ou (fftsize-1) ??????
for	( unsigned int i = 0; i < fftsize; ++i )
	{
        window[i] = a0
		- a1 * cos( 2 * m * i )
		+ a2 * cos( 4 * m * i )
		- a3 * cos( 6 * m * i );
	moy += window[i];
	}
return ( moy / fftsize );
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
// c'est la raison pour laquelle on ne les a pas mis en statique
int spectro::alloc_fft()
{
// verifications preliminaires
if	( fftsize > FFTSIZEMAX )	return -1;
if	( fftstride < 32 )		return -2;
unsigned int size, i = 0;
// allouer buffer pour entree fft
for	( i = 0; i < qthread; ++i )
	{
	if	( fftinbuf[i] == NULL )
		{
		size = FFTSIZEMAX;
		fftinbuf[i] = (float *)fftwf_malloc( size * sizeof(float) );	// pour reel float
		if	( fftinbuf[i] == NULL )
			return -20;
		printf("fftw alloc %d floats\n", size );
		}
	// allouer buffer pour sortie fft
	if	( fftoutbuf[i] == NULL )
		{
		size = ( (FFTSIZEMAX/2) + 1 ) * 2;
		fftoutbuf[i] = (float *)fftwf_malloc( size * sizeof(float) );	// pour complex float
		if	( fftoutbuf[i] == NULL )
			return -21;
		printf("fftw alloc %d floats\n", size );
		}
	// preparer le plan FFTW
	plan[i] = fftwf_plan_dft_r2c_1d( fftsize, fftinbuf[i], (fftwf_complex*)fftoutbuf[i], FFTW_MEASURE );
	if	( plan[i] == NULL )
		return -30;
	printf("FFTW plan ready\n");
	}
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
// precalcul de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
window_avg = window_precalc( window_type );
// window_dump();

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

// un thread pour executer la FFT sur un sous-ensemble de colonnes 
void * computekernel( void * kdata )
{
// extraire 2 donnees de kdata :
unsigned int id = ((kernelblock *)kdata)->id;
spectro * ceci = ((kernelblock *)kdata)->lespectro;
if	( id > ceci->qthread )
	return NULL;
// variables locales
float * fftin;		// buffer pour entree fft reelle
float * fftout;		// buffer pour sortie fft complexe
unsigned int a, j;
unsigned short u;

fftin = ceci->fftinbuf[id];
fftout = ceci->fftoutbuf[id];
ceci->umax_part[id] = 0;

for	( unsigned int icol = id; icol < ceci->W; icol += ceci->qthread )
	{
	// fenetrage sur fftin
	a = icol * ceci->fftstride;
	if	( ceci->src2 )
		{
		for	( j = 0; j < ceci->fftsize; ++j )
			{
			fftin[j] = ( (float)ceci->src1[a] + (float)ceci->src2[a] ) * ceci->window[j];
			++a;
			}
		}
	else	{
		for	( j = 0; j < ceci->fftsize; ++j )
			fftin[j] = (float)ceci->src1[a++] * ceci->window[j];
		}
	// fft de fftin vers fftout
	fftwf_execute( ceci->plan[id] );
	// calcul magnitudes sur place (fftout)
	a = 0;
	for	( j = 0; j <= ( ceci->fftsize / 2 ); ++j )
		{
		fftout[j] = hypotf( fftout[a], fftout[a+1] );
		a += 2;
		}
	// resampling log, de fftout vers fftin
	logpoint *p; float peak;
	for	( j = 0; j < ceci->H; ++j )
		{
		p = &(ceci->log_resamp[j]);
		if	( p->decimflag == 1 )	// decimation par valeur pic
			{
			peak = 0.0;
			for	( a = p->is0; a <= (unsigned int)p->is1; ++a )
				{
				if	( fftout[a] > peak )
					peak = fftout[a];
				}
			}
		else if	( p->decimflag == 0 )	// interpolation lineaire
			{
			peak = fftout[p->is0] * p->k0 + fftout[p->is1] * p->k1;
			}
		else	peak = 0.0;
		fftin[j] = peak;
		}
	// conversion en u16 avec application du facteur d'echelle k, pour palettisation ulterieure
	a = icol * ceci->H;
	for	( j = 0; j < ceci->H; ++j )
		{
		u = (unsigned short)( ceci->k * fftin[j] );
		if	( u > ceci->umax_part[id] )
			ceci->umax_part[id] = u;
		ceci->spectre[a++] = u;
		}
	}

return NULL;
}


// enfin !
void spectro::compute( short * srcA, short * srcB )
{
kernelblock kdata;
kdata.id = 0;
kdata.lespectro = this;

this->src1 = srcA;
this->src2 = srcB;

// facteur d'echelle en vue conversion en u16
// un signal sinus d'amplitude max sur une frequence multiple de fsamp/fftsize atteint le plafond
// un signal carre peut depasser - c'est un cas theorique (alors on observe un visuel de "solarise")
// N.B. il n'est pas necessaire de normaliser les WAV, il suffit de renseigner wav_peak
k = (2.0/(float)fftsize);	// correction DFT classique, selon JLN, pour signal sinus
k /= window_avg;		// correction fenetre
k /= wav_peak;		// 1.0 si wav en float normalisee, 32767.0 si audio 16 bits, ou adapte 
k *= 65535.0;

if	( qthread = 1 )
	computekernel( (void *)&kdata );
else	{
	}

umax = 0;
for	( unsigned i = 0; i < qthread; ++i )
	{
	if	( umax_part[i] > umax )
		umax = umax_part[i];
	}
printf("max binxel umax %u/65535\n", umax );
}

// conversion en style GDK pixbuf
// N.B. spectro ne connait pas GDK mais est compatible avec le style
// utilise la palette connue du spectro
void spectro::spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels )
{
unsigned int x, y, destadr, srcadr, i;
unsigned char * palR = pal;
unsigned char * palG = palR + 65536;
unsigned char * palB = palG + 65536;
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

void spectro::specfree( int deep )	// free all allocated memory
{
if	(spectre) free(spectre);
allocatedWH = 0; spectre = NULL;
// unsigned char * pal : n'est pas alloue par spectro, gere a l'exterieur;
// window[] : statique
// ces buffers ont une taille fixe, ils sont non static en raison des
// des contraites d'alignement de fftw.
if	( deep )
	{
	for	( int i = 0; i < 8; i++ )
		{		
		if ( fftinbuf[i] )  { fftwf_free(fftinbuf[i]);  fftinbuf[i] = NULL;  }
		if ( fftoutbuf[i] ) { fftwf_free(fftoutbuf[i]); fftoutbuf[i] = NULL; }
		}
	}
// les plans fftw
for	( int i = 0; i < 8; i++ )
	fftwf_destroy_plan(plan[i]);
// fftwf_cleanup();	// pas ici, c'est commun a tous les spectros
printf("spectre and fft cleaned\n"); fflush(stdout);
}

// utility functions
double midi2Hz( int midinote )
{	// knowing that A4 = 440 Hz = note 69
return 440.0 * pow( 2.0, ( ( midinote - 69 ) / 12.0 ) );
}

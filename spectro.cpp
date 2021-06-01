#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>

#include "spectro.h"
#include <pthread.h>

/** FFT window functions ======================================================= */

// precalcul de fenetre pour FFT type raised cosine
// couvre les classiques 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
// rend la moyenne
double spectro::window_precalc( unsigned int fft_size )
{
double a0, a1, a2, a3, moy;
switch	( window_type )
	{
	case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
	case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
	case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
	case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
	default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
	}
moy = 0.0;			// on doit diviser par fftsize ( non par (fftsize-1) ), alors 
double m = M_PI / fft_size;	// le dernier echantillon n'est pas egal au premier mais au second, c'est normal
for	( unsigned int i = 0; i < fft_size; ++i )
	{
        window[i] = a0
		- a1 * cos( 2 * m * i )
		+ a2 * cos( 4 * m * i )
		- a3 * cos( 6 * m * i );
	moy += window[i];
	}
return ( moy / fft_size );
}

void spectro::window_dump()
{
for	( unsigned int i = 0; i < fftsize2D; ++i )
	printf("%g\n", window[i] );
}

/** FFTW3 functions ======================================================== */

// allouer les buffers pour la fft
// on alloue large, selon FFTSIZEMAX, pour pouvoir changer fftsize a chaud
// pour i=0 on alloue FFTSIZEHUGE pour les besoins de spectre1D
// fftw3 recommande d'utiliser leur fonction fftwf_malloc() plutot que malloc() ou []
// c'est la raison pour laquelle on ne les a pas mis en statique
int spectro::alloc_fft_inout_bufs()
{
unsigned int size, i = 0;
// allouer buffer pour entree fft
for	( i = 0; i < qthread; ++i )
	{
	if	( fftinbuf[i] == NULL )
		{
		size = ( i ? FFTSIZEMAX : FFTSIZEHUGE );
		fftinbuf[i] = (float *)fftwf_malloc( size * sizeof(float) );	// pour reel float
		if	( fftinbuf[i] == NULL )
			return -20;
		printf("fftw alloc %d floats\n", size );
		}
	// allouer buffer pour sortie fft
	if	( fftoutbuf[i] == NULL )
		{
		size = ( i ? (FFTSIZEMAX+2) : (FFTSIZEHUGE+2) );	// ( (FFTSIZEMAX/2) + 1 ) * 2;
		fftoutbuf[i] = (float *)fftwf_malloc( size * sizeof(float) );	// pour complex float
		if	( fftoutbuf[i] == NULL )
			return -21;
		printf("fftw alloc %d floats\n", size );
		}
	}
return 0;
}

/** log spectrum functions ================================================= */

double spectro::log_fis( double fid )
{
return( relog_fbase * exp2( relog_opp * fid ) );
}

// precalcul des points de resampling pour convertir le spectre en echelle log
// remplir log_resamp en fonction de relog_opp et relog_fbase
void spectro::log_resamp_precalc( unsigned int fsamp, unsigned int fft_size )
{
int id;		// index destination, dans le spectre resample
int iis0, iis1;	// indexes source bruts
int ndec;	// nombre de points sujets a decimation
double fis0;	// index source fractionnaire borne inferieure, correspondant à id - 0.5
double fis1;	// index source fractionnaire borne superieure, correspondant à id + 0.5
double fis;	// index source fractionnaire median, correspondant à id
// les parametres internes pour le resampling log
relog_opp = 1.0 / (double)( bpst * 12 );
relog_fbase = midi2Hz( midi0 ) ;			// en Hz
relog_fbase /= ( (double)fsamp / (double)fft_size );	// en pitch spectro

fis0 = log_fis( -0.5 );		// pour id = 0
for	( id = 0; id < (int)H; ++id )
	{
	fis1 = log_fis( double(id) + 0.5 );
	iis0 = 1 + (int)floor( fis0 );		// exclusive ceiling
	iis1 = (int)floor( fis1 );		// inclusive floor
	if	( iis1 >= (int)fft_size )
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
if	( spectre2D == NULL )
	{
	spectre2D = (unsigned short *)malloc( newsize * sizeof(short int) );
	if	( spectre2D == NULL )
		return -30;
	allocatedWH = newsize;
	printf("spectre2D alloc %u pixels (%ux%u)\n", newsize, W, H );
	}
else if	( newsize > allocatedWH )
	{
	spectre2D = (unsigned short *)realloc( spectre2D, newsize * sizeof(short int) );
	if	( spectre2D == NULL )
		return -31;
	allocatedWH = newsize;
	printf("spectre2D realloc %u pixels (%ux%u)\n", newsize, W, H );
	}
return 0;
}


/** top actions ========================================================== */

// enchaine toutes les initialisations sauf fill_palette() qui est externe
int spectro::init2D( unsigned int fsamp, unsigned int qsamples )
{
unsigned int retval;

//** bornage robuste des parametres
if	( fftsize2D > FFTSIZEMAX )
	fftsize2D = FFTSIZEMAX;
if	( fftstride < 32 )
	fftstride = 32;		// surtout pour eviter fftstride=0
if	( ( bpst & 1 ) == 0 )
	bpst |= 1;
if	( bpst > BPSTMAX )
	bpst = BPSTMAX;
if	( bpst < 1 )
	bpst = 1;
if	( octaves > OCTAMAX )
	octaves = OCTAMAX;
if	( qthread > QTH )
	qthread = QTH;

//** precalculs
W = ( ( qsamples - fftsize2D ) / fftstride ) + 1;		// le nombre de fenetres fft
if	( disable_log )
	{
	unsigned int ftop = 3000;	// frequ. max arbitraire mais < fsamp / 2
	H = ( ftop * fftsize2D ) / fsamp;
	}
else	H = octaves * 12 * bpst;				// le nombre de frequences apres resamp

window_avg = window_precalc( fftsize2D );
// window_dump();
log_resamp_precalc( fsamp, fftsize2D );

//** allocations memoire
// preparer les buffers specifiques pour runs fftw3
retval = alloc_fft_inout_bufs();
if	( retval )
	return retval;
// allouer ou re-allouer le spectre2D
retval = alloc_WH();
if	( retval )
	return retval;

//** preparer les plan de travail de fftw3
for	( unsigned int i = 0; i < qthread; ++i )
	{
	if	( plan[i] )
		fftwf_destroy_plan(plan[i]);
	// ici on met FFTW_MEASURE car on a l'intention de reutiliser ce plan de nombreuses fois
	plan[i] = fftwf_plan_dft_r2c_1d( fftsize2D, fftinbuf[i], (fftwf_complex*)fftoutbuf[i], FFTW_MEASURE );
	if	( plan[i] == NULL )
		return -30;
	}

return 0;
}

// enchaine toutes les initialisations
int spectro::init1D( unsigned int fsamp )
{
//** bornage robuste des parametres
if	( fftsize1D > FFTSIZEHUGE )
	fftsize1D = FFTSIZEHUGE;
if	( ( bpst & 1 ) == 0 )
	bpst |= 1;
if	( bpst > BPSTMAX )
	bpst = BPSTMAX;
if	( bpst < 1 )
	bpst = 1;
if	( octaves > OCTAMAX )
	octaves = OCTAMAX;

//** precalculs
H = octaves * 12 * bpst;				// le nombre de frequences apres resamp

window_avg = window_precalc( fftsize1D );
// window_dump();
log_resamp_precalc( fsamp, fftsize1D );

//** allocations memoire
// preparer les buffers specifiques pour runs fftw3
if	( ( fftinbuf[0] == NULL ) || ( fftoutbuf[0] == NULL ) )
	return -1;

//** preparer le plan de travail de fftw3
if	( plan[0] )
	fftwf_destroy_plan(plan[0]);
// ici on met FFTW_ESTIMATE car on n'a pas l'intention de reutiliser ce plan de nombreuses fois
plan[0] = fftwf_plan_dft_r2c_1d( fftsize1D, fftinbuf[0], (fftwf_complex*)fftoutbuf[0], FFTW_ESTIMATE );
if	( plan[0] == NULL )
	return -30;
return 0;
}

// un thread pour executer la FFT sur un sous-ensemble de colonnes 
static void * computekernel( void * kdata )
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
		for	( j = 0; j < ceci->fftsize2D; ++j )
			{
			fftin[j] = ( (float)ceci->src1[a] + (float)ceci->src2[a] ) * ceci->window[j];
			++a;
			}
		}
	else	{
		for	( j = 0; j < ceci->fftsize2D; ++j )
			fftin[j] = (float)ceci->src1[a++] * ceci->window[j];
		}
	// fft de fftin vers fftout
	fftwf_execute( ceci->plan[id] );
	// calcul magnitudes sur place (fftout)
	a = 0;
	for	( j = 0; j <= ( ceci->fftsize2D / 2 ); ++j )
		{
		fftout[j] = hypotf( fftout[a], fftout[a+1] );
		a += 2;
		}
	if	( ceci->disable_log )
		{
		for	( j = 0; j < ceci->H; ++j )
			fftin[j] = fftout[j];
		}
	else	{
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
		}
	// conversion en u16 avec application du facteur d'echelle k, pour palettisation ulterieure
	a = icol * ceci->H;
	for	( j = 0; j < ceci->H; ++j )
		{
		u = (unsigned short)( ceci->k * fftin[j] );
		if	( u > ceci->umax_part[id] )
			ceci->umax_part[id] = u;
		ceci->spectre2D[a++] = u;
		}
	}

return NULL;
}


// execution de 1 ou plusieurs computekernel en parallele
// il faut avoir deja fait spectro::init2D() - ceci n'est pas verifie
int spectro::compute2D( short * srcA, short * srcB )
{
kernelblock kdata[QTH];

this->src1 = srcA;
this->src2 = srcB;

// facteur d'echelle en vue conversion en u16
// un signal sinus d'amplitude max sur une frequence multiple de fsamp/fftsize2D atteint le plafond
// un signal carre peut depasser - c'est un cas theorique (alors on observe un visuel de "solarise")
// N.B. il n'est pas necessaire de normaliser les WAV, il suffit de renseigner wav_peak
k = (2.0/(float)fftsize2D);	// correction DFT classique, selon JLN, pour signal sinus
k /= window_avg;		// correction fenetre
k /= wav_peak;		// 1.0 si wav en float normalisee, 32767.0 si audio 16 bits, ou adapte 
k *= 65535.0;

// le choeur des coeurs
if	( qthread == 1 )
	{
	kdata[0].id = 0;
	kdata[0].lespectro = this;
	computekernel( (void *)&kdata[0] );
	}
else	{
	pthread_t zeth[QTH];
	int retval;
	// demarrer les threads
	for	( unsigned i = 0; i < qthread; ++i )
		{
		kdata[i].id = i;
		kdata[i].lespectro = this;
		retval = pthread_create( &zeth[i], NULL, computekernel, (void *)&kdata[i] );
		if	( retval )
			return -1;
		}
	// attendre leurs fins
	for	( unsigned i = 0; i < qthread; ++i )
		{
		retval = pthread_join( zeth[i], NULL );
		if	( retval )
			return -2;
		}
	}

// la synthese des umax
umax = 0;
for	( unsigned i = 0; i < qthread; ++i )
	{
	if	( umax_part[i] > umax )
		umax = umax_part[i];
	}
printf("max binxel umax %u/65535\n", umax );
return 0;
}

// calcul d'un spectre1D ponctuel (1 seul run)
// ne fonctionne que si spectrogramme 2D a deja ete parametre et calcule sur src1[]
int spectro::compute1D( unsigned int isamp_center, unsigned int qsamp )
{
// variables locales
float * fftin;		// buffer pour entree fft reelle
float * fftout;		// buffer pour sortie fft complexe
unsigned int a, j;
unsigned short u;

fftin =  fftinbuf[0];	// on utilise les buffers du thread zero
fftout = fftoutbuf[0];

// bornage
if	( qsamp < fftsize1D )
	return -1;
if	( isamp_center < (fftsize1D/2) )
	isamp_center = fftsize1D/2;		// fftsize1D est pair, bien sur
if	( ( isamp_center + (fftsize1D/2) ) > qsamp )
	isamp_center = qsamp - (fftsize1D/2);

printf("ready to compute spectre 1D fftsize1D=%u, @[%u,%u]\n",
	fftsize1D, isamp_center - (fftsize1D/2), isamp_center + (fftsize1D/2) ); fflush(stdout);
// copie des samples sur fftin avec fenetrage et monoisation si necessaire
a = isamp_center - (fftsize1D/2);
if	( src2 )
	{
	for	( j = 0; j < fftsize1D; ++j )
		{
		fftin[j] = ( (float)src1[a] + (float)src2[a] ) * window[j];
		++a;
		}
	}
else	{
	for	( j = 0; j < fftsize1D; ++j )
		fftin[j] = (float)src1[a++] * window[j];
	}

// fft de fftin vers fftout
fftwf_execute( plan[0] );

// calcul magnitudes sur place (fftout)
a = 0;
for	( j = 0; j <= ( fftsize1D / 2 ); ++j )
	{
	fftout[j] = hypotf( fftout[a], fftout[a+1] );
	a += 2;
	}
// resampling log, de fftout vers fftin
logpoint *p; float peak;
for	( j = 0; j < H; ++j )
	{
	p = &(log_resamp[j]);
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
k = (2.0/(float)fftsize1D);	// correction DFT classique, selon JLN, pour signal sinus
k /= window_avg;		// correction fenetre
k /= wav_peak;		// 1.0 si wav en float normalisee, 32767.0 si audio 16 bits, ou adapte 
k *= 65535.0;
// conversion en u16 avec application du facteur d'echelle k
for	( j = 0; j < H; ++j )
	{
	u = (unsigned short)( k * fftin[j] );
	spectre1D[j] = u;
	}
return 0;
}


// conversion spectre2D en style GDK pixbuf, en utilisant la palette connue du spectro
// N.B. spectro ne connait pas GDK mais est compatible avec la disposition pixbuf
// spectre2D est scrute par ligne (bien que dispose par colonnes) pour supporter cette disposition.
// la question du signe de Y:
	// si on veut afficher le pixbuf directement avec GDK, il faut gerer Y+ vers le bas (origine en haut)
	// destadr = ( (H-1) - y ) * RGBstride;
	// si on veut afficher le pixbuf avec JLUPLOT, il faut gerer Y+ vers le haut (origine en bas)
	// destadr = y * RGBstride;
// la question du piano-roll en filigrane
	// il faut convertir y (aka v) en midinote (aka n) pour decider noir ou blanc
	// V = ( n - n0 ) * kn <==> n = n0 + v/bpst avec n0 = midi0 - 0.5/bpst
	// midinote = midi0 + round( y/bpst - 0.5/bpst ) = midi0 + round( ( y - 0.5 ) / bpst )
	// cette formule peut donner un offset trompeur si bpst est pair
void spectro::spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels )
{
unsigned int x, y, destadr, srcadr, i;
unsigned char * palR = pal;
unsigned char * palG = palR + 65536;
unsigned char * palB = palG + 65536;
int midinote;
const char * blacknotes;
if	( disable_log )
	blacknotes = "000000000000";
else	blacknotes = "010100101010";	// midinote = 0 est un Do
for	( y = 0; y < H; y++ )
	{
	destadr = y * RGBstride;
	srcadr = y;
	midinote = midi0 + int( round( ( double(y) - 0.5 ) / double(bpst) ) );
	if	( blacknotes[ midinote % 12 ] & 1 )
		{
		for	( x = 0; x < W; x++ )
			{
			i = spectre2D[srcadr];
			RGBdata[destadr]   = palR[i];
			RGBdata[destadr+1] = palG[i];
			if	( palB[i] < palG[i] )	// tres provisoire
				RGBdata[destadr+2] = palB[i];
			else	RGBdata[destadr+2] = 0;
			destadr += channels;
			srcadr += H;
			}
		}
	else	{
		for	( x = 0; x < W; x++ )
			{
			i = spectre2D[srcadr];
			RGBdata[destadr]   = palR[i];
			RGBdata[destadr+1] = palG[i];
			RGBdata[destadr+2] = palB[i];
			destadr += channels;
			srcadr += H;
			}
		}
	}
}

void spectro::specfree( int deep )	// free all allocated memory
{
if	( spectre2D )
	free(spectre2D);
allocatedWH = 0; spectre2D = NULL;
// unsigned char * pal : n'est pas alloue par spectro, gere a l'exterieur;
// window[] : statique
// ces buffers ont une taille fixe, ils sont non static en raison des
// des contraites d'alignement de fftw.
if	( deep )
	{
	for	( int i = 0; i < QTH; i++ )
		{		
		if ( fftinbuf[i] )  { fftwf_free(fftinbuf[i]);  fftinbuf[i] = NULL;  }
		if ( fftoutbuf[i] ) { fftwf_free(fftoutbuf[i]); fftoutbuf[i] = NULL; }
		}
	}
// les plans fftw
for	( int i = 0; i < QTH; i++ )
	if	( plan[i] )
		{ fftwf_destroy_plan(plan[i]); plan[i] = NULL; }
// fftwf_cleanup();	// pas ici, c'est commun a tous les spectros
printf("spectres and fft cleaned\n"); fflush(stdout);
}


// utility functions
double midi2Hz( int midinote )
{	// knowing that A4 = 440 Hz = note 69
return 440.0 * pow( 2.0, ( ( midinote - 69 ) / 12.0 ) );
}

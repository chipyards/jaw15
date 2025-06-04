#include <stdio.h>
#include <math.h>

#include "fir.h"
#include "demo5_coeff.h"

const char * window_name[] = {
	"rectangle", "hann", "hamming", "blackman", "blackmanharris", "", "", "", 
	"kaiser (castro fast)", "kaiser (castro mid_qual)", "kaiser (castro high_qual)" };

int fir::generate()
{
// preparation parametres
unsigned int halfqfir = 0;
const double * castroeffs;
if	( ( window_type == 8 ) || ( window_type == 9 ) )
	{
	unsigned int cycles;
	if	( window_type == 8 )
		{
		castroeffs = fastest_coeffs.coeffs;
		halfqfir = sizeof(fastest_coeffs.coeffs) / sizeof(double);
		cycles = 8;	// selon comments de Castro
		castro_inc = fastest_coeffs.increment;
		}
	else if	( window_type == 9 )
		{
		castroeffs = slow_mid_qual_coeffs.coeffs;
		halfqfir = sizeof(slow_mid_qual_coeffs.coeffs) / sizeof(double);
		cycles = 21;	// selon comments de Castro
		castro_inc = slow_mid_qual_coeffs.increment;
		}
	qfir = ( halfqfir * 2 ) - 1;
	qpis = 4 * cycles;
	pispan = (double)(qfir-1) / (double)qpis;
	// NB il y a 2 manieres de calculer pispan d'une table de Castro,
	// (sans considerer l'intervalle entre les zeros, qui sont invisibles si pispan n'est pas entier)
	// methode 1 : (qfir - 1)/ qpis : plus logique, mais Castro donne qpis seulement en commentaire (cycles)
	// methode 2 : on profite du scaling applique aux coeffs par Castro 
	double pispan_bis = double(castro_inc) / castroeffs[0];
	snprintf( description, sizeof(description), "FIR %u coeffs, pispan %g (%g), qpis %d, window %d %s",
		qfir, pispan, pispan_bis, qpis, window_type, window_name[window_type] );
	printf("%s\n", description );
	}
else	{
	double dqfir = 1.0 + (double)qpis * pispan;
	qfir = round(dqfir);
	snprintf( description, sizeof(description), "FIR %u coeffs, pispan %g, qpis %d, window %d %s",
		qfir, pispan, qpis, window_type, window_name[window_type] );
	printf("%s\n", description );
	// N.B. qfir doit etre entier et impair, c'est assuré si pispan est entier vu que qpis est pair
	// si pispan est fractionnaire, il doit etre calcule pour que pispan * qpis soit entier et pair
	double fract_qfir = fabs( dqfir - (double)qfir );
	if	( fract_qfir > 1e-5 )
		printf("warning : residu qfir = %g\n", fract_qfir );
	if	( ( qfir & 1 ) == 0 )
		printf("warning : qfir not odd\n");
	}
// ouf, ici qfir est enfin stable

// allouer buffers
if	( FIRbuf == NULL )
	FIRbuf = (double *)malloc( qfir * sizeof(double) );
if	( FIRbuf == NULL )
	{ printf("malloc failed\n"); return -1; }
if	( window_type < 8 )
	{
	if	( FENbuf == NULL )
		FENbuf = (double *)malloc( qfir * sizeof(double) );
	if	( FENbuf == NULL )
		{ printf("malloc failed\n"); return -1; }
	}
else	FENbuf = NULL;

// calcul coeffs
if	( window_type < 8 )
	{				// sinc et fenetre classique
	// calcul fenetre (imitee de spectro::window_precalc() de JAW15)
	double a0, a1, a2, a3;
	// please add Lanczos https://en.wikipedia.org/wiki/Lanczos_resampling
	switch	( window_type )		// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
		{
		case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
		case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
		case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
		case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
		default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
		}
	double m = 2.0 * M_PI / (qfir-1);
	// le sommet de la fenetre est a l'angle m * (qfir-1)/2 = PI 
	for	( unsigned int i = 0; i < qfir; ++i )
		{
		FENbuf[i] = a0
			- a1 * cos(     m * i )
			+ a2 * cos( 2 * m * i )
			- a3 * cos( 3 * m * i );
		}
	double k = M_PI / pispan;	// = m * (qpis/2) 
	double x;
	// le "sommet" du sinc est a i = (qfir-1)/2 => x = 0
	for	( int i = 0; i < (int)qfir; ++i )
		{
		x = k * ( double( i - int((qfir-1)/2) ) );
		FIRbuf[i] = FENbuf[i] * mysinc( x );
		}
	}
else if	( ( window_type == 8 ) || ( window_type == 9 ) )
	{
	// halfqfir = (qfir+1)/2 est la taille d'une "moitie" de RI fournie par Castro
	// ce n'est pas la moitie de qfir (qui est impair), les 2 "moities" se recouvrent sur le coeff central
	// qui est a (qfir-1)/2 = halfqfir - 1
	for	( unsigned int i = 0; i < halfqfir; ++i )
		{	// le "sommet" du sinc est a halfqfir-1, on l'ecrit 2 fois (c'est pas grave ;-)
		double c = castroeffs[i];
		FIRbuf[halfqfir-1+i] = c;	// remplir de halfqfir-1 a qfir-1 inclus
		FIRbuf[halfqfir-1-i] = c;	// remplir de halfqfir-1 a 0 inclus
		}
	}
else	{ qfir = 0; return -666; }

// mode passe_bande : multiplier le FIR par une sinusoide pour translater la reponse frequentielle
if	( band_center > 0.0 )
	{
	double k = M_PI * band_center / pispan;
	double x;
	// le "sommet" du sinc est a i = (qfir-1)/2 => x = 0 => cos(x) = 1
	for	( int i = 0; i < (int)qfir; ++i )
		{
		x = k * ( double( i - int((qfir-1)/2) ) );
		FIRbuf[i] *= cos( x ); 
		}	// ainsi on preserve le sommet du sinc
	printf("bande translatee de %g x Fc\n", band_center );
	}
fflush(stdout);
return 0;
}

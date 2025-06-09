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
	if	( classic == 0 )
		{		// generalized style
		if	( fabs(A0) >= dA )
			{ printf("A0 too big %g vs %g\n", A0, dA ); return -43; }
		general_fir_init();
		}
	else	{		// legacy style
		double dqfir = 1.0 + (double)qpis * pispan;
		qfir = round(dqfir);
		// N.B. qfir doit etre entier et impair, c'est assuré si pispan est entier vu que qpis est pair
		// si pispan est fractionnaire, il doit etre calcule pour que pispan * qpis soit entier et pair
		double fract_qfir = fabs( dqfir - (double)qfir );
		if	( fract_qfir > 1e-5 )
			printf("warning : residu qfir = %g\n", fract_qfir );
		if	( ( qfir & 1 ) == 0 )
			printf("warning : qfir not odd\n");
		}
	snprintf( description, sizeof(description), "FIR %s, %u coeffs, qpis %d, dA %g (pispan %g), A0 %g, window %d %s",
		((classic)?("classic "):("")), qfir, qpis, dA, pispan, A0, window_type, window_name[window_type] );
	printf("%s\n", description );
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
	{
	if	( classic == 0 )
		{
		general_fir();
		}
	else	{
		classic_fir();
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
if	( rB > 0.0 )
	{ printf("band-pass disabled in this version\n"); return -111; }
	/*
	{
	double x;
	// le "sommet" du sinc est a i = (qfir-1)/2 => x = 0 => cos(x) = 1
	for	( int i = 0; i < (int)qfir; ++i )
		{
		x = rB * ( double( i - int((qfir-1)/2) ) );
		FIRbuf[i] *= cos( x ); 
		}	// ainsi on preserve le sommet du sinc
	printf("bande translatee de %g rd/samp\n", rB );
	}
	*/

fflush(stdout);
return 0;
}

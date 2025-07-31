#include <stdio.h>
#include <math.h>

#include "fir.h"

const char * window_name[] = {
	"rectangle", "hann", "hamming", "blackman", "blackmanharris", "lanczos", "kaiser interpol (castro fast)", "kaiser interpol (castro mid_qual)", 
	"", "", "", "Kaiser analytic", "cubic spline" };

int fir::generate()
{
// preparation parametres
if	( fabs(A0) >= dA )
	{ printf("A0 too big %g vs %g\n", A0, dA ); return -43; }

general_fir_init();
if	( window_type == 11 )
	snprintf( description, sizeof(description), "FIR, %u coeffs, qpis %d, dA %g (pispan %g), relA0 %g, rB %g, window %d Kaiser analytic beta=%g",
		qfir, qpis, dA, pispan, A0/dA, rB, window_type, Kbeta );		
else	snprintf( description, sizeof(description), "FIR, %u coeffs, qpis %d, dA %g (pispan %g), relA0 %g, rB %g, window %d %s",
		qfir, qpis, dA, pispan, A0/dA, rB, window_type, window_name[window_type] );
printf("%s\n", description );
// ouf, ici qfir est enfin stable

// allouer buffers
if	( FIRbuf == NULL )
	FIRbuf = (double *)malloc( qfir * sizeof(double) );
if	( FIRbuf == NULL )
	{ printf("malloc failed\n"); return -1; }
if	( FENbuf == NULL )
	FENbuf = (double *)malloc( qfir * sizeof(double) );
if	( FENbuf == NULL )
	{ printf("malloc failed\n"); return -1; }

// calcul coeffs
if	( firmode == ANALYTIC )
	general_fir();
else	{
	if	( firmode != CASTROL )
		{
		deftable.data = (double *)malloc( (deftable.qtable+1) * sizeof(double) );
		if	( deftable.data == NULL )
			{ printf("malloc failed\n"); return -1; }
		general_fir_table( &deftable );
		}
	general_fir_interpol();
	}
fflush(stdout);
return 0;
}

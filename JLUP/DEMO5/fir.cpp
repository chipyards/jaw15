#include <stdio.h>
#include <math.h>

#include "fir.h"

const char * window_name[] = {
	"rectangle", "hann", "hamming", "blackman", "blackmanharris", "lanczos", "kaiser interpol (castro fast)", "kaiser interpol (castro mid_qual)", 
	"kaiser raw (castro fast)", "kaiser raw (castro mid_qual)", "kaiser (castro high_qual)", "Kaiser analytic" };

int fir::generate()
{
// preparation parametres
if	( ( window_type == 8 ) || ( window_type == 9 ) )
	{
	const castrable * cas = &castroz[window_type-8];
	qfir = ( cas->qtable * 2 ) - 1;
	qpis = cas->qpis;
	castro_inc = cas->incr;
	pispan = (double)(qfir-1) / (double)qpis;
	// NB il y a 2 manieres de calculer pispan d'une table de Castro,
	// (la 3eme serait l'intervalle entre zeros consecutifs, mais ils sont invisibles si pispan n'est pas entier)
	// methode 1 : (qfir - 1)/ qpis : plus logique, mais Castro donne qpis seulement en commentaire (cycles)
	// methode 2 : on profite du scaling applique aux coeffs par Castro (ce n'est pas aussi fiable)
	double pispan_bis = double(castro_inc) / cas->table[0];
	snprintf( description, sizeof(description), "FIR %u coeffs, pispan %.14f (%g), qpis %d, window %d %s",
		qfir, pispan, pispan_bis, qpis, window_type, window_name[window_type] );
	printf("%s\n", description );
	}
else	{
	if	( fabs(A0) >= dA )
		{ printf("A0 too big %g vs %g\n", A0, dA ); return -43; }
	general_fir_init();
	if	( window_type == 11 )
		snprintf( description, sizeof(description), "FIR, %u coeffs, qpis %d, dA %g (pispan %g), A0 %g, rB %g, window %d Kaiser analytic beta=%g",
			qfir, qpis, dA, pispan, A0/dA, rB, window_type, Kbeta );		
	else	snprintf( description, sizeof(description), "FIR, %u coeffs, qpis %d, dA %g (pispan %g), A0 %g, rB %g, window %d %s",
			qfir, qpis, dA, pispan, A0/dA, rB, window_type, window_name[window_type] );
	printf("%s\n", description );
	}
// ouf, ici qfir est enfin stable

// allouer buffers
if	( FIRbuf == NULL )
	FIRbuf = (double *)malloc( qfir * sizeof(double) );
if	( FIRbuf == NULL )
	{ printf("malloc failed\n"); return -1; }
if	( ( window_type < 8 ) || ( window_type > 10 ) )
	{
	if	( FENbuf == NULL )
		FENbuf = (double *)malloc( qfir * sizeof(double) );
	if	( FENbuf == NULL )
		{ printf("malloc failed\n"); return -1; }
	}
else	FENbuf = NULL;

// calcul coeffs
if	( ( window_type < 8 ) || ( window_type > 10 ) )
	{
	if	( ( window_type < 6 ) || ( window_type > 10 ) )
		general_fir();
	else	general_fir_castro();
	}
else if	( ( window_type == 8 ) || ( window_type == 9 ) )
	{
	const castrable * cas = &castroz[window_type-8];
	// cas->qtable = (qfir+1)/2 est la taille d'une "moitie" de RI fournie par Castro
	// ce n'est pas la moitie de qfir (qui est impair), les 2 "moities" se recouvrent sur le coeff central
	// qui est a (qfir-1)/2 = cas->qtable - 1
	for	( int i = 0; i < cas->qtable; ++i )
		{	// le "sommet" du sinc est a halfqfir-1, on l'ecrit 2 fois (c'est pas grave ;-)
		double c = cas->table[i];
		FIRbuf[cas->qtable-1+i] = c;	// remplir de cas->qtable-1 a qfir-1 inclus
		FIRbuf[cas->qtable-1-i] = c;	// remplir de cas->qtable-1 a 0 inclus
		}
	}
else	{ qfir = 0; return -666; }

fflush(stdout);
return 0;
}

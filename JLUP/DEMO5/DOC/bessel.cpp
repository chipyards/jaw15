/*
 g++ -o bessel.exe -Wall -O3 bessel.cpp
 */
#include <stdio.h>
#include <math.h>
#include "../../../cli_parse.h"


// Modified Bessel function of 1st kind, order zero, for Kaiser
// - dans le cas normal (not modified), apres avoir diverge tant que k < x/2,
//   la somme converge, donc le nombre d'iterations requises augmente avec x !
//   Même ainsi la precision diminue fortement pour les grandes valeurs de x,
//   car on calcule des petites differences de grands nombre pendant l'etape divergente.
// - dans le cas modified, l'alternance de signe est supprimee, la somme diverge toujours
//   mais en ralentissant, et l'asymptote x->inf (s'il y en a) n'est plus horizontale.
double mJ0( double x, int N )
{
double x2, J0, oldJ0, facto, puiss;
oldJ0 = J0 = 1.0; x2 = x / 2;
facto = 1.0;     /* soit k! pour k=0 */
puiss = 1.0;     /* soit (x/2)^^k pour k=0 */
int k;
for	( k = 1; k < N; k++ )
	{
	facto *= k; puiss *= x2;
	double dJ;
	dJ = puiss / facto;
	dJ *= dJ;
	printf("k=%2d -> %g/%g -> %g\n", k, puiss, facto, dJ );
	#ifdef BESSEL_NORMAL
	if	( k & 1 )	// k impair
		J0 -= dJ;
	else	J0 += dJ;
	#else
	J0 += dJ;		// "modified Bessel"
	#endif
	if	( J0 == oldJ0 )	// auto-stop
		break;		// dJ became too small to have any effect
	oldJ0 = J0;
	};
printf("x=%g last k=%d\n", x, k );
return( J0 );
}

int main( int argc, char *argv[] )
{
cli_parse * lepar = new cli_parse( argc, (const char **)argv, "N" );
const char * val;
double x = 0.0;
int N = 50;

if	( ( val = lepar->get( 'N' ) ) )	N = atoi( val );
if	( ( val = lepar->get( '@' ) ) )	x = strtod( val, NULL );

double y = mJ0( x, N );
printf("mJO(%g) = %g\n", x, y );


return 0;
}
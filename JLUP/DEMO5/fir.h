#include "demo5_coeff.h"

typedef struct {
	int qpis;	// 4 * cycles
	int qtable;	// sommet inclus
	const double * table;
	int incr;
	} castrable;

// data from demo5_coeff.h
const castrable castroz[] = {
	{ 4*8, sizeof(fastest_coeffs.coeffs) / sizeof(double), fastest_coeffs.coeffs, fastest_coeffs.increment },
	{ 4*21, sizeof(slow_mid_qual_coeffs.coeffs) / sizeof(double), slow_mid_qual_coeffs.coeffs, slow_mid_qual_coeffs.increment }
	};

class fir {
public:
int classic;		// mode classic
double pispan;		// taille de PI dans la reponse impulsionnelle
unsigned int qpis;	// nombre de fois pi dans le sinc de la RI, dit "nombre de zeros
unsigned int castro_inc;// increment unitaire dans la reponse impulsionnelle pour Castro (i.e. Kaiser)
unsigned int qfir;	// taille de la reponse impulsionnelle ( cnt_left + cnt_right )
unsigned int cnt_left;	// part de qfir a gauche du coeff de ref (exclus)
unsigned int cnt_right;	// part de qfir a droite du coeff de ref (inclus)
double A0;		// decalage angulaire du coeff "central" -dA < A0 < dA
double dA;		// increment angulaire (rd/samp)
int window_type;	// type de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris, 8 et 9 = Castro
double a0;		// coeff pour calcul fenetres 0..4
double a1;
double a2;
double a3;
double Kbeta;		// specifique Kaiser, beta = PI * alpha
double mJ0Kbeta;	// pre-calcul de mJ0(Kbeta)
double * FENbuf;	// fenetre
double * FIRbuf;	// impulse response
double rB;		// passe-bande : reponse translatee (rd/samp)

char description[128];

// constructeur
fir() : classic(0), pispan(1.0), qpis(4), qfir(1), cnt_left(0), cnt_right(0), A0(0.0), dA(0.0),
	window_type(0), a0(1), a1(0), a2(0), a3(0), Kbeta(M_PI*2.55),
	FENbuf(NULL), FIRbuf(NULL), rB(0.0) { mJ0Kbeta = mJ0( Kbeta ); };

// methodes

// calcul de coefficients en fonction de l'angle A, pour RI "continue"
// A = 0 au centre 
double mysinc( double A ) {
	if	( fabs(A) < 1e-6 )
		return 1.0;
	return ( sin(A) / A );
	};

// interpolation sur table pour winw_type = 6 ou 7
double myCastro( double A ) {
	const castrable * cas = &castroz[window_type-6];
	double A_half_span = M_PI * (cas->qpis/2);
	double k = double(cas->qtable-1) / A_half_span;
	A = fabs(A);	// symetrie pour pas cher !
	double dindex = k * A;
	// interpolation
	double di = floor(dindex);
	double df = dindex - di;
	int i = (int)di; 
	double C0, C1;
	C0 = cas->table[i++];
	if	( i < cas->qtable )
		C1 = cas->table[i];
	else	C1 = C0;
	double C = C0 + ( (C1-C0) * df ); 
	return C;
	}

// facteur cos pour filtre passe-bande
double mycos( double A, double rel_shift ) {
	return cos( A * rel_shift );
	};
 
// Modified Bessel function of 1st kind, order zero, for Kaiser
// - dans le cas normal (not modified), apres avoir diverge tant que k < x/2,
//   la somme converge, donc le nombre d'iterations requises augmente avec x !
//   Même ainsi la precision diminue fortement pour les grandes valeurs de x,
//   car on calcule des petites differences de grands nombre pendant l'etape divergente.
// - dans le cas modified, l'alternance de signe est supprimee, la somme diverge toujours
//   mais en ralentissant, et il n'y a plus d'asymptote horizontale pour x->inf.
//   Dans l'application Kaiser, il est normal d'avoir des mJ0 aussi elevees que 25000 (Kbeta=12.3)
double mJ0( double x )
{
double x2, J0, facto, puiss;
J0 = 1.0; x2 = x / 2;
facto = 1.0;     /* soit k! pour k=0 */
puiss = 1.0;     /* soit (x/2)^^k pour k=0 */
for	( int k = 1; k < 50; k++ )
	{
	facto *= k; puiss *= x2;
	double dJ;
	dJ = puiss / facto;
	// printf("k=%2d -> %g/%g = %g\n", k, puiss, facto, dJ );
	if	( dJ < 3e-8 )
		break;		// note : squared, it is 1e-15
	dJ *= dJ;
	#ifdef BESSEL_NORMAL
	if	( k & 1 )	// k impair
		J0 -= dJ;
	else	J0 += dJ;
	#else
	J0 += dJ;
	#endif
	};
// printf("mJO(%g) = %g\n", x, J0 );
return( J0 );
}

void init_window() {	// Lanczos needs nothing here, but Kaiser does
	switch	( window_type )		// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
		{
		case 0: a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	; break; // rect
		case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
		case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
		case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
		case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
		case 11: mJ0Kbeta = mJ0( Kbeta ); 					  break; // Kaiser analytic							
		default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
		}
	};
double mywindow( double khann, double A ) {
	// khann est la frequence relative du raised cos constituant la fenetre de Von Hann
	// normalement khann = 2 / qpis, sauf experimentation speciale
	// en effet le terme en a1 couvre 2 * PI pendant que A couvre qpis * PI
	double fen;
	if	( window_type == 5 )
		{
		A *= khann;
		fen = mysinc( A );		// Lanczos window (le coeff de lanczos est qpis/2)
		}
	else if	( window_type == 11 )
		{
		// Kaiser n'est pas la fonction d'un angle, mais de x relatif a sa demi-largeur
		// t.q. 0 <= x <= 1.0
		// OBS : comparaison avec la formule B=0.1102(q-8.7) vue dans la These de JL :
		//	Kbeta = 10.1 ok avec Castro fast,
		// 		12.3 ok avec Castro mid_qual
		double x = A / ( 0.5 * M_PI * qpis );
		double top = 1.0 - x*x;
		if	( top < 0.0 )	top = 0.0;	// mefiance cas limite
		else			top = sqrt( top );
		top = mJ0( Kbeta * top );
		fen = top / mJ0Kbeta;
		}
	else	{
		A *= khann;
		fen =	  a0			// (method from spectro::window_precalc() de JAW15)
			+ a1 * cos(     A )
			+ a2 * cos( 2 * A )
			+ a3 * cos( 3 * A );
		}
	return fen;
	};

// echantillonner une RI "conventionnelle", symetrique, avec un coeff au centre et un a chaque bout 
void classic_fir() {
	double dA = M_PI / pispan;
	init_window();
	double khann = 2.0 / qpis;
	// on part du sommet
	int topi = (qfir-1)/2;
	double A = 0.0;
	FENbuf[topi] = mywindow( khann, A );
	FIRbuf[topi] = FENbuf[topi];
	for	( int i = 1; i <= topi; ++i )
		{
		A += dA;
		double f = mywindow( khann, A );
		FENbuf[topi+i] = FENbuf[topi-i] = f;
		FIRbuf[topi+i] = FIRbuf[topi-i] = f * mysinc( A );
		}	
	};
// preparer general_fir(), pour avoir qfir pret pour alloc memoire
// Note 1 : A0 est le déplacement angulaire du sommet du sinc par rapport a un sample voisin
// 	pris comme référence (le plus proche, mais ce n'est pas obligé)
// 	A0 est compté positif si le sommet est à droite du sample de reference
// Note 2 : on va effectuer le filtrage en partant du sample de ref, en 2 fois, 
// 	du coté droit en incluant le sample de ref puis du gauche en excluant le sample de ref
// le filtrage n'a pas besoin de ce calcul preliminaire, on le fait pour avoir la longueur qfir
// en vue d'allouer le buffer pour stockage des coeffs
void general_fir_init() {
	if	( ( window_type >= 6 ) && ( window_type < 11 ) )
		{
		const castrable * cas = &castroz[window_type-6];
		qpis = cas->qpis;
		castro_inc = cas->incr;
		}
	double A_half_span = M_PI * (qpis/2);
	// methode 1 : calcul analytique
	double R = ( A_half_span + A0 ) / dA;
	double L = ( A_half_span - A0 - dA ) / dA;
	// printf("L=%.18f, R=%.18f\n", L, R );
	// pourquoi ceil ?
	//	- cas general : nombre de samples = nombre d'intervalles + 1
	//	- cas particulier R ou L entier : nombre de samples = nombre d'intervalles
	//	  car on ne veut pas le sample qui est sur le bord (aussi elimine par les boucles) 
	cnt_left  = (int)ceil( L );
	cnt_right = (int)ceil( R );
	printf("cnt_left=%d, cnt_right=%d\n", cnt_left, cnt_right );
	qfir = cnt_left + cnt_right;
	/* methode 2 : simulation des boucles du filtrage pour verif des cas limite
	Les cas limite sont les cas ou un sample est pile au bord de la fenetre, ou le coeff est 0.0
	Lors du filtrage, ces cas sont normalement elimines par ( A < A_half_span )
	Mais il arrive rarement que ce test rende true alors que theoriquement A == A_half_span
	en raison d'erreurs cumulees sur A += dA.
	exemple de reference : DEMO5/demo5 -Z 14 -w 4 -F 0.125
	Cela n'a aucun impact sur le filtrage (ajout d'un terme nul) mais pourrait causer un debordement
	du buffer FIRbuf, qu'on evite avec un test de securite dans general_fir()
	Une autre approche, stupidement couteuse, serait de calculer cnt_left et cnt_right par simulation
	comme ci-dessous. Mais on a observe que la simu donne les memes valeurs que la methode 1,
	donc en contradiction avec general_fir() dans les cas tels que l'exemple de ref. !!!!
	Explication : les boucles de simu sont optimisees pour que tous les calculs restent dans le FPU
	qui a une resolution de 80 bits alors que le type double est limite a 64 bits.
	En inserant des printf dans les boucles, on force la simu a avoir le meme comportement que
	general_fir(), c'est a dire inclusion des samples limites. idem avec volatile double A.
	Resolution finale : on garde la methode 1, plus efficace dans le cas general, et on accepte les
	"evitage debordements" dans les rares cas de fuite du test ( A < A_half_span )
	*/
	/*
	int sim_right = 0; double A = -A0;
	// printf("A_half_span %.18f\n", A_half_span );
	while	( A < A_half_span )
		{
		// printf("simu %.18f\n", A );
		A += dA; sim_right++;
		}
	int sim_left = 0; A = - A0 - dA;
	while	( A > (-A_half_span) )
		{
		// printf("simu %.18f\n", A );
		A -= dA; sim_left++;
		}
	printf("sim_left=%d, sim_right=%d\n", sim_left, sim_right );
	
	*/
	};	// general_fir_init()

// echantillonner une RI "generalisee", non symetrique si A0 != 0, Fc arbitraire 
int general_fir() {
	init_window();
	double khann = 2.0 / qpis;
	double A = -A0;		// angle of ref sample
	int i = cnt_left;
	double A_half_span = M_PI * (qpis/2);
	// right side (incl ref sample @ -A0)
	if	( rB == 0.0 )
		while	( A < A_half_span )
			{
			if	( i >= (int)qfir )
				{ printf("note: evitage debordement fir a droite\n"); break; }
			FENbuf[i] = mywindow( khann, A );
			FIRbuf[i] = FENbuf[i] * mysinc( A );
			A += dA; i++;
			}
	else	while	( A < A_half_span )
			{
			if	( i >= (int)qfir )
				{ printf("note: evitage debordement fir a droite\n"); break; }
			FENbuf[i] = mywindow( khann, A );
			FIRbuf[i] = FENbuf[i] * mysinc( A ) * mycos( A, rB );
			A += dA; i++;
			}
	int iend = i;
	// left side (excl ref sample @ -A0)
	A = - A0 - dA;
	i = cnt_left - 1;
	if	( rB == 0.0 )
		while	( A > (-A_half_span) ) 
			{
			if	( i < 0 )
				{ printf("note: evitage debordement fir a gauche\n"); break; }
			FENbuf[i] = mywindow( khann, A );
			FIRbuf[i] = FENbuf[i] * mysinc( A );
			A -= dA; i--;
			}
	else	while	( A > (-A_half_span) ) 
			{
			if	( i < 0 )
				{ printf("note: evitage debordement fir a gauche\n"); break; }
			FENbuf[i] = mywindow( khann, A );
			FIRbuf[i] = FENbuf[i] * mysinc( A ) * mycos( A, rB );
			A -= dA; i--;
			}
	// verifications
	if	( i != -1 )
		printf("suspect: anomalie gauche i = %d\n", i );
	if	( iend != (int)qfir )
		printf("suspect: anomalie droite cnt = %d vs %d\n", iend, qfir );
	return iend;
	};	// general_fir()

int general_fir_castro() {	// seulement pour window_type = 6 ou 7
	double A = -A0;		// angle of ref sample
	int i = cnt_left;
	double A_half_span = M_PI * (qpis/2);
	// right side (incl ref sample @ -A0)
	if	( rB == 0.0 )
		while	( A < A_half_span )
			{
			if	( i >= (int)qfir )
				{ printf("note: evitage debordement fir a droite\n"); break; }
			FENbuf[i] = 0.0;
			FIRbuf[i] = myCastro(A);
			A += dA; i++;
			}
	else	while	( A < A_half_span )
			{
			if	( i >= (int)qfir )
				{ printf("note: evitage debordement fir a droite\n"); break; }
			FENbuf[i] = 0.0;
			FIRbuf[i] = myCastro(A) * mycos( A, rB );
			A += dA; i++;
			}
	int iend = i;
	// left side (excl ref sample @ -A0)
	A = - A0 - dA;
	i = cnt_left - 1;
	if	( rB == 0.0 )
		while	( A > (-A_half_span) ) 
			{
			if	( i < 0 )
				{ printf("note: evitage debordement fir a gauche\n"); break; }
			FENbuf[i] = 0.0;
			FIRbuf[i] = myCastro(A);
			A -= dA; i--;
			}
	else	while	( A > (-A_half_span) ) 
			{
			if	( i < 0 )
				{ printf("note: evitage debordement fir a gauche\n"); break; }
			FENbuf[i] = 0.0;
			FIRbuf[i] = myCastro(A) * mycos( A, rB );
			A -= dA; i--;
			}
	// verifications
	if	( i != -1 )
		printf("suspect: anomalie gauche i = %d\n", i );
	if	( iend != (int)qfir )
		printf("suspect: anomalie droite cnt = %d vs %d\n", iend, qfir );
	return iend;
	};	// general_fir()

int generate();

};
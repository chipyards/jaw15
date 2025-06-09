
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
double * FENbuf;	// fenetre
double * FIRbuf;	// impulse response
double rB;		// passe-bande : reponse translatee (rd/samp)

char description[128];

// constructeur
fir() : classic(0), pispan(1.0), qpis(2), qfir(1), cnt_left(0), cnt_right(0), A0(0.0), dA(0.0),
	window_type(0), a0(1), a1(0), a2(0), a3(0),
	FENbuf(NULL), FIRbuf(NULL), rB(0.0) {};

// methodes

// calcul de coefficients en fonction de l'angle A, pour RI "continue"
// A = 0 au centre 
double mysinc( double A ) {
	if	( fabs(A) < 1e-6 )
		return 1.0;
	return ( sin(A) / A );
	};
void init_window() {	// please add Lanczos https://en.wikipedia.org/wiki/Lanczos_resampling
	switch	( window_type )		// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
		{
		case 1: a0 = 0.50	; a1 =  0.50	; a2 =  0.0	; a3 =  0.0	; break; // hann
		case 2: a0 = 0.54	; a1 =  0.46	; a2 =  0.0	; a3 =  0.0	; break; // hamming
		case 3: a0 = 0.42	; a1 =  0.50	; a2 =  0.08	; a3 =  0.0	; break; // blackman
		case 4: a0 = 0.35875	; a1 =  0.48829	; a2 =  0.14128	; a3 =  0.01168	; break; // blackmanharris
		default:a0 = 1.0	; a1 =  0.0	; a2 =  0.0	; a3 =  0.0	;        // rect
		}
	};
double mywindow( double khann, double A ) {
	// khann est la frequence relative du raised cos constituant la fenetre de Von Hann
	// normalement khann = 2 / qpis, sauf experimentation speciale
	// en effet le terme en a1 couvre 2 * PI pendant que A couvre qpis * PI
	double fen;
	A *= khann;
	fen =	  a0			// (method from spectro::window_precalc() de JAW15)
		+ a1 * cos(     A )
		+ a2 * cos( 2 * A )
		+ a3 * cos( 3 * A );
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
	double A_half_span = M_PI * (qpis/2);
	// methode 1 : calcul analytique
	double R = ( A_half_span + A0 ) / dA;
	double L = ( A_half_span - A0 - dA ) / dA;
	// printf("L=%.16f, R=%.16f\n", L, R );
	// pourquoi ceil ?
	//	- cas general : nombre de samples = nombre d'intervalles + 1
	//	- cas particulier R ou L entier : nombre de samples = nombre d'intervalles
	//	  car on ne veut pas le sample qui est sur le bord (aussi elimine par les boucles) 
	cnt_left  = (int)ceil( L );
	cnt_right = (int)ceil( R );
	printf("cnt_left=%d, cnt_right=%d\n", cnt_left, cnt_right );
	qfir = cnt_left + cnt_right;
	// methode 2 : simulation des boucles du filtrage (sera supprimee apres verif)
	int sim_right = 0; double A = -A0;
	while	( A < A_half_span )
		{ A += dA; sim_right++; }
	int sim_left = 0; A = - A0 - dA;
	while	( A > (-A_half_span) )
		{ A -= dA; sim_left++; }
	printf("sim_left=%d, sim_right=%d\n", sim_left, sim_right );
	};
// echantillonner une RI "generalisee", non symetrique si A0 != 0, Fc arbitraire 
int general_fir() {
	init_window();
	double khann = 2.0 / qpis;
	double A = -A0;		// angle of ref sample
	int i = cnt_left;
	double A_half_span = M_PI * (qpis/2);
	// right side (incl ref sample @ -A0)
	while	( A < A_half_span )
		{
		if	( i >= (int)qfir )
			{ printf("evitage debordement fir a droite\n"); break; }
		FENbuf[i] = mywindow( khann, A );
		FIRbuf[i] = FENbuf[i] * mysinc( A );
		A += dA; i++;
		}
	int iend = i;
	// left side (excl ref sample @ -A0)
	A = - A0 - dA;
	i = cnt_left - 1;
	while	( A > (-A_half_span) ) 
		{
		if	( i < 0 )
			{ printf("evitage debordement fir a gauche\n"); break; }
		FENbuf[i] = mywindow( khann, A );
		FIRbuf[i] = FENbuf[i] * mysinc( A );
		A -= dA; i--;
		}
	// verifications
	if	( i != -1 )
		printf("left side anomaly i = %d\n", i );
	if	( iend != (int)qfir )
		printf("right side anomaly cnt = %d vs %d\n", iend, qfir );
	return iend;
	};

int generate();

};
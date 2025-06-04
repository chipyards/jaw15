
class fir {
public:
double pispan;		// taille de PI dans la reponse impulsionnelle
unsigned int qpis;	// nombre de fois pi dans le sinc de la RI, dit "nombre de zeros
unsigned int castro_inc;// increment unitaire dans la reponse impulsionnelle pour Castro (i.e. Kaiser)
unsigned int qfir;	// taille de la reponse impulsionnelle 
double A0;		// decalage du centre de la RI
double dA;		// increment angulaire
int window_type;	// type de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris, 8 et 9 = Castro
double a0;		// coeff pour calcul fenetres 0..4
double a1;
double a2;
double a3;
double * FENbuf;	// fenetre
double * FIRbuf;	// impulse response
double band_center;	// passe-bande : reponse translatee par band_center * Fc

char description[128];

// constructeur
fir() : pispan(777), qpis(12), qfir(0), A0(0.0), dA(0.0), window_type(0), a0(1), a1(0), a2(0), a3(0),
	FENbuf(NULL), FIRbuf(NULL), band_center(0.0) {};

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
// echantillonner une RI "generalisee", non symetrique si A0 != 0, Fc arbitraire 
int general_fir( double A0, double dA ) {
	init_window();
	double khann = 2.0 / qpis;
	// on veut partir a proximite du sommet (normalement fabs(A0) < dA)
	double A_half_span = 0.5 * M_PI * qpis;
	// on a besoin de ce calcul seulement pour mettre le premier coeff a l'indice zero
	// si on filtrait directement on s'en passerait
	int cnt_left  = (int)floor( ( A_half_span + A0 ) / dA );
	// celui-ci juste pour verif
	int cnt_right = (int)floor( ( A_half_span - A0 ) / dA );
	// on part comme on a dit
	double A = A0;
	int i = cnt_left;
	// right side (incl A0)
	while	( A < A_half_span )
		{
		FENbuf[i] = mywindow( khann, A );
		FIRbuf[i] = FENbuf[i] * mysinc( A );
		A += dA; i++;
		}
	int iend = i;
	// left side (excl. A0)
	A = A0 - dA;
	i = cnt_left - 1;
	while	( A > (-A_half_span) ) 
		{
		if ( i < 0 ) break;	// securite pour le cas limite
		FENbuf[i] = mywindow( khann, A );
		FIRbuf[i] = FENbuf[i] * mysinc( A );
		A -= dA; i--;
		}
	// verifications
	if	( i != -1 )
		printf("left side anomaly i = %d\n", i );
	int expected_cnt = cnt_left + 1 + cnt_right;
	if	( iend != expected_cnt )
		printf("right side anomaly cnt = %d vs %d\n", iend, expected_cnt );
	return iend;
	};

int generate();

};
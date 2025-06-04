
class fir {
public:
double pispan;		// taille de PI dans la reponse impulsionnelle
unsigned int qpis;	// nombre de fois pi dans le sinc de la RI, dit "nombre de zeros
unsigned int castro_inc;// increment unitaire dans la reponse impulsionnelle pour Castro (i.e. Kaiser)
unsigned int qfir;	// taille de la reponse impulsionnelle 
int window_type;	// type de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris, 8 et 9 = Castro
double * FENbuf;	// fenetre
double * FIRbuf;	// impulse response

double band_center;	// passe-bande : reponse translatee par band_center * Fc

char description[128];

// constructeur
fir() : pispan(777), qpis(12), qfir(0), window_type(0), FENbuf(NULL), FIRbuf(NULL), band_center(0.0) {};

// methodes
double mysinc( double x ) {
	if	( fabs(x) < 1e-5 )
		return 1.0;
	return ( sin(x) / x );
	};
int generate();

};
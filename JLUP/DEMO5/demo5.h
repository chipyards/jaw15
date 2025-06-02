
#define VIEW_W 1200
#define VIEW_H 900

class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   nmain;
GtkWidget *   vpans;   
GtkWidget *     vpan1;
GtkWidget *       darea1;
GtkWidget *       zarea1;
GtkWidget *     darea2;
GtkWidget *   hbut;
// GtkWidget *     brun;
// GtkWidget *     braz;
GtkWidget *      edesc;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

int idle_id;		// id pour la fonction idle du timeout

unsigned int qbuf;	// taille de buffer
double pispan;		// taille de PI dans la reponse impulsionnelle
unsigned int qpis;	// nombre de fois pi dans le sinc de la RI, dit "nombre de zeros
unsigned int castro_inc;// increment unitaire dans la reponse impulsionnelle pour Castro (i.e. Kaiser)
unsigned int qfir;	// taille de la reponse impulsionnelle 
int window_type;	// type de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris, 8 et 9 = Castro
double band_center;	// passe-bande : reponse translatee par band_center * Fc
const char * ifnam;	// nom de fichier wav a filtrer
const char * ofnam;	// nom de fichier wav a sauver

double * Cbuf;	// reponse impulsionelle d'un filtre
double * Tbuf;	// reponse frequentielle obtenue par DFT
fftw_plan plan;

autobuf <float> Wbuf;	// audio brut a filtrer 
autobuf <float> Ybuf;	// audio apres filtrage
wavio wavp;		// objet audiofile pour lecture wav
char description[128];

// constructeur
glostru() : qbuf(1<<20), pispan(777), qpis(12), qfir(0), window_type(0), band_center(0.0),
	    ifnam(NULL), ofnam(NULL), Cbuf(NULL), Tbuf(NULL), plan(NULL) {};

// methodes
double mysinc( double x ) {
	if	( fabs(x) < 1e-5 )
		return 1.0;
	return ( sin(x) / x );
	};
int generate();
int audiofile_load( int verbose );
int audiofile_process();
int audiofile_save( int monosamplesize, int qchan );

void layout1();
void layout1W();
void layout2();

};


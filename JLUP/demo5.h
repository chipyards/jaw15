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
GtkWidget *     brun;
GtkWidget *     braz;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

int idle_id;		// id pour la fonction idle du timeout

unsigned int qbuf;	// taille de buffer
unsigned int pispan;	// taille de PI dans la reponse impulsionnelle
unsigned int qfir;	// taille de la reponse impulsionnelle 
int window_type;	// type de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
double band_center;	// passe-bande : reponse translatee par band_center * Fc
const char * ifnam;	// nom de fichier wav a filtrer

double * Cbuf;	// reponse impulsionelle d'un filtre
double * Tbuf;	// reponse frequentielle obtenue par DFT
fftw_plan plan;

autobuf <float> Wbuf;	// audio brut a filtrer 
wavio wavp;		// objet audiofile pour lecture wav

// constructeur
glostru() : qbuf(1<<20), pispan(1<<9), qfir(1<<13), window_type(0), band_center(0.0),
	    ifnam(NULL), Cbuf(NULL), Tbuf(NULL), plan(NULL) {};

// methodes
void window_precalc( double * window, unsigned int size );
int generate();
int audiofile_process( int verbose );

void layout1();
void layout1W();
void layout2();

};


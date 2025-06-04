
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

const char * ifnam;	// nom de fichier wav a filtrer
const char * ofnam;	// nom de fichier wav a sauver

unsigned int qFFT;	// taille de buffer FFT
double * FFTin;		// peut contenir RI suivi de nombreux zeros pour bonne FFT
double * FFTout;	// alors reponse frequentielle obtenue par DFT
fftw_plan plan;

autobuf <float> Wbuf;	// audio brut a filtrer 
autobuf <float> Ybuf;	// audio apres filtrage
wavio wavp;		// objet audiofile pour lecture wav

// constructeur
glostru() : ifnam(NULL), ofnam(NULL),
	    qFFT(1<<20), FFTin(NULL), FFTout(NULL), plan(NULL) {};

// methodes
// calcul FFT pour visu reponse frequentielle
int fft_on_FIR( unsigned int firsize, double * firbuf );
int audiofile_load( int verbose );
int audiofile_process();
int audiofile_save( int monosamplesize, int qchan );

void layout1();
void layout1W();
void layout2();

};


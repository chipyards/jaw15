#define FFTSIZEMAX 8192
#define HMAX 2048

// un point precalcule pour le reechantillonnage du spectre en echelle log
class logpoint {
public :
int decimflag;	// alors decimer de is0 a is1 inclus, sinon interpoler
int is0;	// premier indice source
int is1;	// second indice source
float k0;	// coeff d'interpolation cote is0
float k1;	// coeff d'interpolation cote is1
};

class spectro {
public :
unsigned int fftsize;		// en samples
unsigned int fftstride;		// en samples
unsigned int spectresize;	// allocated for spectre, en pixels
unsigned int W;			// nombre de colonnes ( env. nombre_samples / fftstride )
unsigned int H;			// nombre de frequences sur le spectre resample
unsigned short * spectre;	// spectre W * H , resample, pret a palettiser
unsigned int umax;		// valeur max vue dans spectre[]
float window[FFTSIZEMAX];	// fenetre pre-calculee
float * fftinbuf;		// buffer pour entree fft reelle
float * fftoutbuf;		// buffer pour sortie fft complexe
fftwf_plan p;			// le plan FFTW
unsigned char palR[65536];	// la palette 16 bits --> RGB
unsigned char palG[65536];
unsigned char palB[65536];
logpoint log_resamp[HMAX];	// parametres precalcules pour re-echantillonnage log
double log_opp;			// echelle spectre re-echantillonne en OPP (Octave Per Point) << 1
double log_fbase;		// frequence limite inferieure du spectre, exprimee en quantum de FFT
// constructeur
spectro() : fftsize(4096), fftstride(4096/8), spectresize(0), H(512), spectre(NULL), fftinbuf(NULL), fftoutbuf(NULL) {};
// methodes
void window_precalc( double a0, double a1, double a2, double a3 );
void window_hann() { window_precalc( 0.50, 0.50, 0.0, 0.0 ); };
void window_hamming() { window_precalc( 0.54, 0.46, 0.0, 0.0 ); };
void window_blackman() { window_precalc( 0.42, 0.50, 0.08, 0.0 ); };
void window_blackmanharris() { window_precalc( 0.35875, 0.48829, 0.14128, 0.01168 ); };
void window_dump();
double log_fis( double fid ) {
  return( log_fbase * exp2( log_opp * fid ) );
  }
void log_resamp_precalc();
void log_resamp_dump();
int init( unsigned int qsamples );
void compute( float * src );	// calcul spectre complet W colonnes x H lignes
void spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels );	// conversion en style GDK pixbuf
void fill_palette( unsigned int iend );
};


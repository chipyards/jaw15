/*
la classe spectro cree une representation intermediaire du spectrogramme d'un passage musical
sous forme d'un buffer 'spectre' de valeurs de 16 bits, destinees a servir d'index dans une LUT pour donner du RGB.

Le procede inspire de Sonic Visualizer utilise la FFT suivie d'une conversion log de l'axe F ( spectro::compute() ).

Ensuite la conversion en RGB ou colorisation peut se faire avec spectro::spectre2rgb(), possiblement directement
dans un GDK pixbuf (actuellement la colorisation traite 'spectre' entier)

Le buffer 'spectre' represente un ruban de hauteur H et longueur W
	- H est le nombre de "bins" apres passage en echelle log, on le fixe arbitrairement
	  exemple : 840 pour 7 octaves a 120 bins par octave
	- W est le nombre d'echantillons spectraux = nombres de runs FFT,
		- approximativement W = nombre total de samples audio / fftstride
		- plus precisement W = ( ( qsamples - fftsize ) / fftstride ) + 1
	  en effet le dernier run FFT commence au sample ((W-1)*fftstride) et finit au sample ((W-1)*fftstride)+(fftsize-1)
	  la condition d'adressage est (((W-1)*fftstride)+(fftsize-1)) < qsamples
		((W-1)*fftstride) < (qsamples - fftsize + 1)
		((W-1)*fftstride) <= (qsamples - fftsize)
		(W-1) <= ((qsamples - fftsize)/fftstride)	Ok avec division entiere
	- fftstride est typiquement une fraction de fftsize, par exemple 1/8 mais ce n'est pas oblige
H doit etre fourni par l'application, alors que W est calcule par spectro::init()
ATTENTION : les elements de 'spectre' sont ranges par colonne, non par ligne comme dans une image

Echelle verticale :
	- l'application doit fournir
		- log_opp = octaves par bin (octaves par pixel par abus de langage, log aussi abus car opp est deja log)
		  par exemple 1.0 / 120.0;	// 10 bins / demi-ton
		- log_fbase = frequence du bin le plus bas, exprimee en resolution FFT
		  par exemple F0 / ( sample_freq / fftsize ), avec F0 en Hz
	- question : ou est la limite entre interpolation et decimation ?
		- pitch fft = ( sample_freq / fftsize ) exemple 44100 / 8192 = 5.38 Hz
		- pitch spectre = f * ( pow( 2, opp ) - 1 ) exemple f * (pow(2,1/120)-1) = f * 0.0058
		  soit f = pitch / 0.0058 = 5.38/0.0058 = 927 Hz (Bb5)
	  interpretation : au dela de cette F, il y a moins de sample dans spectre que de bins FFT, donc perte d'info
	  mais sans consequence pour l'appli
*/
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
int midi0;			// frequence limite inferieure du spectre, exprimee en midinote
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

// utility functions
double midi2Hz( int midinote );

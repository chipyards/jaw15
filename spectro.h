/* spectro special Toyota Hybride ====================
cette version ne supporte pas le resampling log des frequences
la classe spectro cree une representation intermediaire du spectrogramme d'un signal
sous forme d'un buffer 'spectre' de valeurs de 16 bits, destinees a servir d'index dans une LUT pour donner du RGB.

Ensuite la conversion en RGB ou colorisation peut se faire avec spectro::spectre2rgb(), possiblement directement
dans un GDK pixbuf (actuellement la colorisation traite 'spectre' entier)

Le buffer 'spectre' represente un ruban de hauteur H et longueur W
	- H est le nombre de "bins" <= (fftsize/2)
	  exemple : fsamp = 44100Hz, fftsize = 16384, fmax = 2kHz, H = (fftsize/2)*(2k/22.050k) = 743
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

*/
#define FFTSIZEMAX 65536
#define HMAX 2048

class spectro {
public :
unsigned int fftsize;		// en samples
unsigned int fftstride;		// en samples
unsigned short * spectre;	// spectre W * H binxels, resample en log, pret a palettiser
unsigned int W;			// nombre de colonnes ( env. nombre_samples / fftstride )
unsigned int H;			// nombre de frequences (bins) sur le spectre retenu
unsigned int allocatedWH;	// W*H effectivement alloue
unsigned int fmax;		// limite de frequence pour partie visible du spectre (determine H)
unsigned int umax;		// valeur max vue dans spectre[]
unsigned char palR[65536];	// la palette 16 bits --> RGB
unsigned char palG[65536];
unsigned char palB[65536];
private:
float window[FFTSIZEMAX];	// fenetre pre-calculee
float * fftinbuf;		// buffer pour entree fft reelle
float * fftoutbuf;		// buffer pour sortie fft complexe
fftwf_plan p;			// le plan FFTW
public:
// constructeur
spectro() : fftsize(16384), fftstride(16384/4), spectre(NULL), allocatedWH(0), fmax(3000), fftinbuf(NULL), fftoutbuf(NULL) {};
// methodes


// FFT window functions
void window_precalc( double a0, double a1, double a2, double a3 );
void window_hann() { window_precalc( 0.50, 0.50, 0.0, 0.0 ); };
void window_hamming() { window_precalc( 0.54, 0.46, 0.0, 0.0 ); };
void window_blackman() { window_precalc( 0.42, 0.50, 0.08, 0.0 ); };
void window_blackmanharris() { window_precalc( 0.35875, 0.48829, 0.14128, 0.01168 ); };
void window_dump();
// FFTW3 functions
int alloc_fft();
// log spectrum functions
void parametrize( unsigned int fsamp, unsigned int qsamples );	// calcul des parametres derives
int alloc_WH();
// top actions
int init( unsigned int fsamp, unsigned int qsamples );
void compute( float * src );	// calcul spectre complet W colonnes x H lignes
void fill_palette( unsigned int iend );
void spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels ); // conversion compatible GDK pixbuf
};


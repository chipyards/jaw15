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
	  (approximativement W = nombre total de samples audio / fftstride)
	- fftstride est le pas temporel des iterations de calcul FFT, c'est la largeur d'un binxel en samples
	- fftsize est la largeur de fenetre FFT
	- fftsize >= fftstride
Changement de coordonnee :
	- le centre du binxel d'indice ibin est sur le sample d'indice isamp = fftsize/2 + (ibin * fftstride)
	  son bord gauche est a isamp0 = fftsize/2 + (ibin * fftstride) - (fftstride/2)
	  son bord droit  est a isamp1 = fftsize/2 + (ibin * fftstride) + (fftstride/2)
	  l'image s'etend de ( fftsize/2 - (fftstride/2) ) a ( fftsize/2 + ((W-1)*fftstride) + (fftstride/2) )
	  laissant deux petites marges a droite et gauche
	  d'ou les coeffs de transformation U = ibin, M = isamp pour l'affichage du spectrogramme RGB 
		km = 1.0 / fftstride
		m0 = 0.5 * (fftsize-fftstride)
	  et la transformation ibin(isamp) pour l'extraction d'un spectre ponctuel :
		ibin = ( isamp - m0 ) / fftstride, avec division entiere (ou floor), borne sur [0,W-1]
	- le calcul de W 
		le dernier run FFT finit au sample ((W-1)*fftstride) + fftsize - 1
			((W-1)*fftstride) + fftsize - 1	< qsamples
			((W-1)*fftstride) 	 	<= qsamples - fftsize
			  W-1				<= ( qsamples - fftsize ) / fftstride
		d'ou la formule pratique
			W = ( ( qsamples - fftsize ) / fftstride ) + 1 avec division entiere (par defaut)

ATTENTION : rappel : les elements de 'spectre' sont ranges par colonne, non par ligne comme dans une image

Echelle verticale :
	- l'application doit fournir
		- relog_opp = octaves par bin (octaves par pixel par abus de langage, log aussi abus car opp est deja log)
		  par exemple 1.0 / 120.0;	// 10 bins / demi-ton
		- relog_fbase = frequence du bin le plus bas, exprimee en resolution FFT
		  par exemple F0 / ( sample_freq / fftsize ), avec F0 en Hz
	- question : ou est la limite entre interpolation et decimation ?
		- pitch fft = ( sample_freq / fftsize ) exemple 44100 / 8192 = 5.38 Hz
		- pitch spectre = f * ( pow( 2, opp ) - 1 ) exemple f * (pow(2,1/120)-1) = f * 0.0058
		  soit f = pitch / 0.0058 = 5.38/0.0058 = 927 Hz (Bb5)
	  interpretation : au dela de cette F, il y a moins de sample dans spectre que de bins FFT, donc perte d'info
	  mais sans consequence pour l'appli
*/
#define FFTSIZEMAX 16384
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

#define PALSIZE (65536*3)

class spectro {
public :
unsigned int fftsize;		// en samples
unsigned int fftstride;		// en samples
double window_avg;		// moyenne de window[]
int window_type;		// 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
unsigned short * spectre;	// spectre W * H binxels, resample en log, pret a palettiser
unsigned int W;			// nombre de colonnes ( env. nombre_samples / fftstride )
unsigned int H;			// nombre de frequences (bins) sur le spectre resample
unsigned int allocatedWH;	// W*H effectivement alloue
unsigned int umax;		// valeur max vue dans spectre[]
unsigned char * pal;		// la palette 16 bits --> RGB, contient PALSIZE byte
unsigned int bpst = 10;		// binxel-per-semi-tone : resolution spectro log
unsigned int octaves;		// hauteur du spectre a partir de midi0
int midi0;			// frequence limite inferieure du spectre, exprimee en midinote
double wav_peak;		// pour facteur d'echelle avant conversion du spectre en u16
private:
float window[FFTSIZEMAX];	// fenetre pre-calculee
float * fftinbuf;		// buffer pour entree fft reelle
float * fftoutbuf;		// buffer pour sortie fft complexe
fftwf_plan p;			// le plan FFTW
logpoint log_resamp[HMAX];	// parametres precalcules pour re-echantillonnage log
double relog_opp;			// echelle spectre re-echantillonne en OPP (Octave Per Point) << 1
double relog_fbase;		// frequence limite inferieure du spectre, exprimee en quantum de FFT
public:
// constructeur
spectro() : fftsize(4096), fftstride(4096/8), spectre(NULL), allocatedWH(0), pal(NULL), wav_peak(32767.0),
	 fftinbuf(NULL), fftoutbuf(NULL), p(NULL) {};
// methodes

// FFT window functions
// precalcul de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
double window_precalc( int ft_window );
void window_dump();
// FFTW3 functions
int alloc_fft();
// log spectrum functions
double log_fis( double fid );
void parametrize( unsigned int fsamp, unsigned int qsamples );	// calcul des parametres derives
void log_resamp_precalc();
void log_resamp_dump();
int alloc_WH();
// top actions
int init( unsigned int fsamp, unsigned int qsamples );
void compute( float * src );	// calcul spectre complet W colonnes x H lignes
// conversion en style GDK pixbuf
// N.B. spectro ne connait pas GDK mais est compatible avec le style
void spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels );
// liberation memoire
void specfree( int deep );	// free all allocated memory
};

// utility functions
double midi2Hz( int midinote );

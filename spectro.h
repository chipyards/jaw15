/*
La classe spectro cree une representation intermediaire du spectrogramme d'un passage musical
sous forme d'un buffer 'spectre2D' de valeurs de 16 bits, destinees a servir d'index dans une LUT pour donner du RGB.
Elle peut aussi creer un spectre1D "local".

Le procede, inspire de Sonic Visualizer, utilise la FFT suivie d'une conversion log de l'axe F ( spectro::compute2D() ).

Ensuite la conversion en RGB ou colorisation peut se faire avec spectro::spectre2rgb(), possiblement directement
dans un GDK pixbuf (actuellement la colorisation traite spectre2D entier)

Le buffer spectre2D represente un ruban de hauteur H et longueur W
	- H est le nombre de "bins" apres passage en echelle log, on le fixe arbitrairement
	  exemple : 840 pour 7 octaves a 120 bins par octave
	- W est le nombre d'echantillons spectraux = nombres de runs FFT [aka icol],
	  (approximativement W = nombre total de samples audio / fftstride)
	- fftstride est le pas temporel des iterations de calcul FFT, c'est la largeur d'un binxel en samples
	- fftsize est la largeur de fenetre FFT
	- fftsize >= fftstride
Changement de coordonnee abcisse (temps) :
	- le centre des binxels d'indice icol est sur le sample d'indice isamp = fftsize/2 + (icol * fftstride)
	  son bord gauche est a isamp0 = fftsize/2 + (icol * fftstride) - (fftstride/2)
	  son bord droit  est a isamp1 = fftsize/2 + (icol * fftstride) + (fftstride/2)
	  l'image s'etend de ( fftsize/2 - (fftstride/2) ) a ( fftsize/2 + ((W-1)*fftstride) + (fftstride/2) )
	  laissant deux petites marges a droite et gauche
	  d'ou les coeffs de transformation U = icol, M = isamp pour l'affichage du spectrogramme RGB 
		km = 1.0 / fftstride			U = ( m - m0 ) * km
		m0 = 0.5 * (fftsize-fftstride)
	  et la transformation ibin(isamp) pour l'extraction d'un spectre1D ponctuel :
		ibin = ( isamp - m0 ) / fftstride, avec division entiere (ou floor), borne sur [0,W-1]
	- le calcul de W 
		le dernier run FFT finit au sample ((W-1)*fftstride) + fftsize - 1
			((W-1)*fftstride) + fftsize - 1	< qsamples
			((W-1)*fftstride) 	 	<= qsamples - fftsize
			  W-1				<= ( qsamples - fftsize ) / fftstride
		d'ou la formule pratique
			W = ( ( qsamples - fftsize ) / fftstride ) + 1 avec division entiere (par defaut)
Changement de coordonnee ordonnee (frequence)
	- transformation V = ibin [aka irow], N = midinote pour l'affichage du spectrogramme RGB
		kn = bpst (bins per semi-tone)		V = ( n - n0 ) * kn
		n0 = midi0 - 0.5/bpst		<-- offset pour que la graduation tombe au milieu du binxel
	  Une transformation similaire est utilisee pour l'axe des abcisses du spectre1D ponctuel :
		km = bpst (bins per semi-tone)		U = ( m - m0 ) * km
		m0 = midi0			<-- pas d'offset ici (ce ne sont pas des pixels)
	  Au niveau des reticules ces tranformations peuvent etre affectees d'un offset de "fine tuning" r0 (resp. q0)

ATTENTION : rappel : les elements de spectre2D sont ranges en memoire 1D par colonne, non par ligne comme dans une image

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
#define FFTSIZEMAX  16384	// pour spectre2D
#define FFTSIZEHUGE 65536	// pour spectre1D

#define BPSTMAX	19
#define OCTAMAX 10
#define HMAX	(BPSTMAX*12*OCTAMAX)

#define QTH	8		// nombre max de threads

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
unsigned short * spectre2D;	// spectre W * H binxels, resample en log, pret a palettiser
unsigned short spectre1D[HMAX];	// spectre de H binxels, resample en log, pret a tracer
unsigned int W;			// nombre de colonnes ( env. nombre_samples / fftstride )
unsigned int H;			// nombre de frequences (bins) sur le spectre resample
unsigned int allocatedWH;	// W*H effectivement alloue
unsigned int umax;		// valeur max mise dans spectre2D[]
unsigned char * pal;		// la palette 16 bits --> RGB, contient PALSIZE byte
unsigned int bpst;		// binxel-per-semi-tone : resolution spectro log DOIT etre IMPAIR
unsigned int octaves;		// hauteur du spectre exprimee en octaves a partir de midi0
int midi0;			// frequence limite inferieure du spectre, exprimee en midinote
double wav_peak;		// pour facteur d'echelle avant conversion du spectre en u16
unsigned int qthread;		// nombre de threads

// pourrait etre private, sauf que les threads doivent y acceder
short * src1;			// audio a transformer
short * src2; 			// second canal si on veut transformer de la stereo sur un spectrogramme
float k;			// facteur d'echelle en vue conversion en u16
float window[FFTSIZEMAX];	// fenetre pre-calculee
logpoint log_resamp[HMAX];	// parametres precalcules pour re-echantillonnage log
double relog_opp;		// echelle spectre re-echantillonne en OPP (Octave Per Point) << 1
double relog_fbase;		// frequence limite inferieure du spectre, exprimee en quantum de FFT
// les donnees separees par thread
float * fftinbuf[QTH];		// buffers pour entree fft reelle
float * fftoutbuf[QTH];		// buffers pour sortie fft complexe
fftwf_plan plan[QTH];		// les plan FFTW
unsigned int umax_part[QTH];	// valeur max mise dans spectre2D[] par chaque thread

// constructeur
spectro() : fftsize(8192), fftstride(1024), window_type(1), spectre2D(NULL), allocatedWH(0), umax(0),
	pal(NULL), bpst(9), octaves(7), midi0(28), wav_peak(32767.0), qthread(1), src1(0), src2(0) {
	for	( int i = 0; i < QTH ; ++i )
		{
		fftinbuf[i] = NULL;
		fftoutbuf[i] = NULL;
		plan[i] = NULL;
		}
	};

// methodes

// FFT window functions
// precalcul de fenetre 0=rect, 1=hann, 2=hamming, 3=blackman, 4=blackmanharris
double window_precalc( int ft_window );
void window_dump();
// FFTW3 functions
int alloc_fft_inout_bufs();
// log spectrum functions
double log_fis( double fid );
void log_resamp_precalc( unsigned int fsamp );
void log_resamp_dump();
int alloc_WH();
// top actions
int init2D( unsigned int fsamp, unsigned int qsamples );
int compute2D( short * srcA, short * srcB = NULL );	// calcul spectre2D complet W x H
int compute1D( unsigned int fsamp, unsigned int isamp, unsigned int local_fftsize );	// spectre1D
// conversion en style GDK pixbuf
// N.B. spectro ne connait pas GDK mais est compatible avec le style
void spectre2rgb( unsigned char * RGBdata, int RGBstride, int channels );
// liberation memoire
void specfree( int deep );	// free all allocated memory
};

// structure pour passer a chaque thread
class kernelblock {
public:
int id;
spectro * lespectro;
};

// utility functions
double midi2Hz( int midinote );

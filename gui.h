/* singleton pseudo global storage ( allocated in main()  )
   JLN's GTK widget naming chart :
   w : window
   f : frame
   h : hbox
   v : vbox
   b : button
   l : label
   e : entry
   s : spin adj
   m : menu
   o : option
 */

#define DEFAULT_SF2 "F:\\STUDIO\\MIDI\\SF2\\GM\\GeneralUser1.471.sf2"

class glostru {
public :
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   darea;
GtkWidget *   zarea;
GtkWidget *   hbut;
GtkWidget *     bpla;
GtkWidget *     brew;
GtkWidget *     esta;
GtkWidget *     bpar;
GtkWidget *     bqui;

gpanel panneau;		// panneau principal, dans darea
gzoombar zbar;		// sa zoombar
param_view para;	// la fenetre auxiliaire de parametres

int idle_id;		// id pour la fonction idle du timeout

int iplay;		// index echantillon en train d'etre joue (<0 <==> idle)
int iplayp;		// index echantillon en pause
int iplay0;		// index debut fenetre playable
int iplay1;		// index fin   fenetre playable (ignore si tres grand)

#ifdef USE_PORTAUDIO
PaStream *stream;	// portaudio stream
// double play_start_time;
#endif

int option_spectrogramme;
int option_monospec;
int option_noaudio;
unsigned int option_threads;
int option_linspec;
int option_verbose;
const char * sf2file;

process pro;		// le process : lecture wav calcul spectro, preparation layout
fluid local_synth;	// synthe local, pour temps-reel

// constructeur
glostru() : para(this), iplay(-1), iplayp(0), iplay0(0), iplay1(2000000000),
	option_spectrogramme(0), option_monospec(0), option_noaudio(0), option_linspec(0), option_verbose(0),
	sf2file(DEFAULT_SF2) {};

// methodes
void wavisualize( const char * fnam );	// chargement et layout d'un fichier audio
void spectrographize();			// creation et layout du spectrogramme
void parametrize();			// recuperation des parametres editables
};


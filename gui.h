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

gpanel panneau;		// panneau principal (wav), dans darea
gzoombar zbar;		// sa zoombar
param_view para;	// la fenetre auxiliaire de parametres

int iplay;		// index echantillon en train d'etre joue (<0 <==> idle)
int iplayp;		// index echantillon en pause
int iplay0;		// index debut fenetre playable
int iplay1;		// index fin   fenetre playable (ignore si tres grand)

#ifdef USE_PORTAUDIO
PaStream *stream;	// portaudio stream
// double play_start_time;
#endif

int option_monospec;
int option_noaudio;

process pro;		// le process : lecture wav calcul spectro, preparation layout

// constructeur
glostru() : iplay(-1), iplayp(0), iplay0(0), iplay1(2000000000), option_monospec(0), option_noaudio(0) {};

};


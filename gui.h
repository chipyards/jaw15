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

int iplay;		// index echantillon en train d'etre joue (-1 <==> idle)
int iplayp;		// index echantillon en pause
int iplay0;		// index depart
int iplay1;		// index fin

#ifdef USE_PORTAUDIO
PaStream *stream;	// WAV player : portaudio stream
double play_start_time;
#endif

process pro;		// le process : lecture wav calcul spectro, preparation layout

int darea_expose_cnt;
int idle_profiler_cnt;
int idle_profiler_time;

};


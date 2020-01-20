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


typedef struct
{
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   farea;
GtkWidget *   darea;
GtkWidget *   sarea;
GtkWidget *   hbut;
GtkWidget *     bpla;
GtkWidget *     brew;
GtkWidget *     esta;
GtkWidget *     bqui;
GtkWidget *   hsli;
int darea_queue_flag;

gpanel panneau;		// panneau principal (wav), dans darea
gzoombar zbar;		// sa zoombar

int iplay;		// index echantillon en train d'etre joue (-1 <==> idle)
int iplayp;		// index echantillon en pause
int iplay0;		// index depart
int iplay1;		// index fin

#ifdef PAUDIO
PaStream *stream;	// WAV player : portaudio stream
double play_start_time;
#endif

process pro;		// le process : lecture wav calcul spectro, preparation layout
int auto_png;		// index pour la generation d'une sequence de png
double auto_png_incm;	// increment M (en samples !)
double auto_png_dm;	// largeur M
double auto_png_mend;	// M max

double level;		// niveau relatif pour saturation colorisation spectre et extraction frequ.

int darea_expose_cnt;	// stats
int idle_profiler_cnt;
int idle_profiler_time;

} glostru;


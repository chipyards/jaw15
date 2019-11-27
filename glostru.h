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
int darea_queue_flag;

gpanel panneau;		// panneau principal (wav), dans darea
gzoombar zbar;		// sa zoombar

char wnam[256];		// fichiers WAV
wavpars wavp;

int iplay;		// index echantillon en train d'etre joue (-1 <==> idle)
int iplayp;		// index echantillon en pause
int iplay0;		// index depart
int iplay1;		// index fin

#ifdef PAUDIO
PaStream *stream;	// WAV player : portaudio stream
double play_start_time;
#endif
spectro spec;		// un spectrographe

int darea_expose_cnt;
int idle_profiler_cnt;
int idle_profiler_time;

} glostru;


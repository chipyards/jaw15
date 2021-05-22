
class glostru;

// classe pour fenetre auxiliaire non-modale
class param_view {
public :
// elements du GUI GTK
GtkWidget * wmain;	// fenetre principale
GtkWidget * nmain;	// notebook principal
  GtkWidget * vlay;	// layout multitrack
  GtkWidget * vspe;	// params spectro
    GtkWidget * hspe;	// params spectro
      GtkWidget * vspp;	// params spectro
        GtkWidget * cfwin;	// combo pour FFT window
        GtkWidget * cfsiz;	// combo pour FFT size
        GtkWidget * estri;	// entry for FFT stride
        GtkWidget * bbpst;	// spin button pour BPST
	GtkWidget * bc2p;	// check clic-to-play
      GtkWidget * sarea;	// spectre1D
    GtkAdjustment * adjk;	// ajustement pour gain module

  GtkWidget * vfil;	// files
  GtkWidget * vpor;	// I/O ports


gpanel panneau;		// panneau pour spectre1D local, dans sarea
glostru * glo;

// constructeur
param_view( glostru * laglo ) : wmain(NULL), glo(laglo) {};

// methodes
void build();
void show();
void hide();

// constantes
const unsigned int small_fftsize[11] = { 512, 3*256, 1024, 3*512, 2048, 3*1024, 4096, 3*2048, 8192, 3*4096, 16384 };
const unsigned int huge_fftsize[25]  = { 4096, 18*256 , 20*256 , 24*256 , 27*256 , 30*256 ,
					 8192, 18*512 , 20*512 , 24*512 , 27*512 , 30*512 ,
					16384, 18*1024, 20*1024, 24*1024, 27*1024, 30*1024,
					32768, 18*2048, 20*2048, 24*2048, 27*2048, 30*2048, 32*2048 };
};

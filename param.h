
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
};

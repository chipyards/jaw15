
class glostru;

// classe pour fenetre auxiliaire non-modale
class param_view {
public :
// elements du GUI GTK
GtkWidget * wmain;	// fenetre principale
GtkWidget * nmain;	// notebook principal
  GtkWidget * vlay;	// layout multitrack
  GtkWidget * vspe;	// params spectro
    GtkWidget * sarea;	// spectrum
    GtkWidget * cfwin;	// combo pour FFT window
    GtkWidget * cfsiz;	// combo pour FFT size
  GtkWidget * vfil;	// files
  GtkWidget * vpor;	// I/O ports


gpanel panneau;		// panneau pour spectre local, dans sarea
glostru * glo;

// constructeur
param_view( glostru * laglo ) : wmain(NULL), glo(laglo) {};

// methodes
void build();
void show();
void hide();

};

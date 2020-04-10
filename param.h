// classe pour fenetre auxiliaire non-modale
class param_view {
public :
// elements du GUI GTK
GtkWidget * wmain;	// fenetre principale
GtkWidget * nmain;	// notebook principal
  GtkWidget * vlay;	// layout multitrack
  GtkWidget * vspe;	// params spectro
    GtkWidget * sarea;	// spectrum
  GtkWidget * vfil;	// files
  GtkWidget * vpor;	// I/O ports


gpanel panneau;		// panneau pour spectre local, dans sare

// constructeur
param_view() : wmain(NULL) {};

// methodes
void build();
void show();
void hide();

};

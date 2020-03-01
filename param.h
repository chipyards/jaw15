// classe pour fenetre auxiliaire non-modale
class param_view {
public :
// elements du GUI GTK
GtkWidget * wmain;	// fenetre principale
GtkWidget * nmain;	// notebook principal
  GtkWidget * vlay;	// layout multitrack
  GtkWidget * vspe;	// spectrum
  GtkWidget * vfil;	// files
  GtkWidget * vpor;	// I/O ports

// constructeur
param_view() : wmain(NULL) {};

// methodes
void build();
void show();
void hide();

};

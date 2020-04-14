// classe pour fenetre auxiliaire non-modale
class param_view {
public :
// elements du GUI GTK
GtkWidget * wmain;	// fenetre principale
GtkWidget * nmain;	// notebook principal
  GtkWidget * vlay;	// layout multitrack
  GtkWidget * vpro;	// process
  GtkWidget * vfil;	// files
  GtkWidget * vpor;	// I/O ports

// constructeur
param_view() : wmain(NULL) {};

// methodes
void build();
void show();
void hide();

};

// classe pour un parametre analog ajustable
class param_analog {
public:
const char * tag;
double amin;
double amax;
int decimales;			// nombre de chiffres apres la virgule
GtkWidget *hbox;		// boite pour label + slider
GtkWidget *label;		// le label
GtkWidget *hscale;		// le slider
GtkAdjustment *adjustment;	// l'ajustement utilise par le slider
void (*callback)(GtkAdjustment*,void*);	// call back facultative

// constructeur
param_analog(): amin(0.0), amax(1.0), decimales(0), callback(NULL) {};

// methodes
GtkWidget * build();		// d'apres les params. rend la hbox
double get_value();
void set_value( double v );
};

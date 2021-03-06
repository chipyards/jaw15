// une image RGB telle qu'un spectrogramme (classe derivee de layer_base)
// (no destructor needed , GdkPixbuf * spectropix is allocated and freed by app)

class layer_rgb : public layer_base {
public :
GdkPixbuf * spectropix;	// pixbuf contenant le spectrogramme apres palettisation

// constructeur
layer_rgb() : layer_base(), spectropix(NULL) {};

// methodes propres a cette classe derivee

// les methodes qui sont virtuelles dans la classe de base
double get_Umin();
double get_Umax();
double get_Vmin();
double get_Vmax();
void draw( cairo_t * cai );	// dessin
};

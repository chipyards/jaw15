// une courbe a pas uniforme unsigned short (classe derivee de layer_base)

class layer_u16 : public layer_base {
public :

unsigned short * V;
unsigned short Vmin, Vmax;
int qu;		// nombre de points effectif
int curi;	// index point courant
int style;
// constructeur
layer_u16() : layer_base(),
	  Vmin(0), Vmax(65535), qu(0), curi(0), style(0) {};

// methodes propres a cette classe derivee

int goto_U( double U0 );		// chercher le premier point U >= U0
void goto_first();
int get_pi( double & rU, double & rV );	// get XY then post increment
void scan();				// mettre a jour les Min et Max
// void allocV( size_t size );		// ebauche de service d'allocation DEPRECATED

// les methodes qui sont virtuelles dans la classe de base
double get_Umin();
double get_Umax();
double get_Vmin();
double get_Vmax();
void draw( cairo_t * cai );	// dessin
};

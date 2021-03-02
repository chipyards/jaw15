// layer_f_param : une courbe parametrique en float (classe derivee de layer_base)

class layer_f_param : public layer_base {
public :

float * U;	// abcisse
float * V;	// ordonnee
float Umin, Umax, Vmin, Vmax;
int qu;		// nombre de points effectif
int curi;	// index point courant

// constructeur
layer_f_param() : layer_base(),
	  Umin(0.0), Umax(1.0), Vmin(0.0), Vmax(1.0), qu(0), curi(0) {};

// methodes propres a cette classe derivee

int goto_U( double U0 );		// chercher le premier point U >= U0
int get_pi( double & rU, double & rV );	// get XY then post increment
void scan();				// mettre a jour les Min et Max

// les methodes qui sont virtuelles dans la classe de base
double get_Umin() { return (double)Umin; };
double get_Umax() { return (double)Umax; };
double get_Vmin() { return (double)Vmin; };
double get_Vmax() { return (double)Vmax; };
void draw( cairo_t * cai );	// dessin
};

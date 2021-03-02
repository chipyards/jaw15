// layer_f_fifo : une courbe a pas uniforme en float (classe derivee de layer_base)
// supporte un fifo sous forme de buffer circulaire, pour utilisation en temps reel
// l'espace U (qui represente le temps) croit indefiniment (debordement non gere)
// l'intervalle glissant [Ub, Ue[ contient des data significatives
// contrairement a la plupart des layers, dans celui-ci l'appli n'est pas proprietaire du buffer
// elle ne doit pas allouer la memoire ni ecrire directement dans le buffer

class layer_f_fifo : public layer_base {
public :
unsigned int log_qfifo;	// log2 de la taille du fifo
long long Ub;		// U de la plus ancienne donnee disponible
long long Ue;		// U de la prochaine donnee attendue (Ue-1 = U de la derniere donnee dispo)
float * V;
float Vmin, Vmax;
int style;
// constructeur (avec 1 parametre optionnel)
layer_f_fifo( unsigned int lelog=10 ) : layer_base(),
	log_qfifo(lelog), Ub(0), Ue(0), Vmin(0.0), Vmax(1.0), style(0) {
	V = (float *)malloc( (1<<log_qfifo) * sizeof(float) );
	if (V == NULL) { fprintf(stderr, "failed layer_f_fifo malloc"); exit(1); }
	};

// methodes propres a cette classe derivee
unsigned int maskfifo() {
	return (1<<log_qfifo) - 1;
	};
void push( double Ve );		// ajouter un point
void clear() {			// effacer le contenu
	Ub = Ue = 0; Vmin = 0.0; Vmax = 1.0;
	};

void scan();				// mettre a jour les Min et Max

// les methodes qui sont virtuelles dans la classe de base
double get_Umin();
double get_Umax();
double get_Vmin();
double get_Vmax();
void draw( cairo_t * cai );	// dessin
};

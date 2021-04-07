// layer_f_lod : une courbe a pas uniforme en float 32 bits (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)

class lod_f {	// un niveau de detail
public :
float * max;
float * min;	// des couples min_max
int qc;		// nombre de couples min_max
int kdec;	// decimation par rapport a l'audio original
// constructeur
lod_f() : max(NULL), min(NULL), qc(0), kdec(1) {};
// methodes
void allocMM( size_t size );	// ebauche de service d'allocation
};

class layer_f_lod : public layer_base {
public :

float * V;
float Vmin, Vmax;
int qu;			// nombre de points effectif
int curi;		// index point courant
double linewidth;	// 0.5 pour haute resolution, 1.0 pour bon contraste
double spp_max;		// nombre max de sample par pixel (samples audio ou couple minmax)
			// s'il est trop petit, on a une variation de contraste au changement de LOD
			// s'il est trop grand on ralentit l'affichage
			// bon compromis  : 0.9 / linewidth
vector <lod_f> lods;	// autant le LODs que necessaire
int ilod;		// indice du LOD courant, -1 pour affichage a pleine resolution
			// -2 pour inconnu (choix ilod a faire)

// constructeur
layer_f_lod() : layer_base(), V(NULL), Vmin(-1.0), Vmax(1.0),
		qu(0), curi(0), linewidth(0.7), spp_max(0.9/linewidth),
		ilod(-2) {};

// methodes propres a cette classe derivee
int make_lods( unsigned int klod1, unsigned int klod2, unsigned int maxwin );	// allouer et calculer les LODs
void scan();			// mettre a jour les Min et Max
				// aussi fait  par make_lods()
void find_ilod();		// choisir le LOD optimal (ilod) en fonction du zoom
int goto_U( double U0 );	// chercher le premier point U >= U0
void goto_first();
int get_pi( double & rU, double & rV );	// get XY then post increment

void draw( cairo_t * cai, double tU0, double tU1 );	// dessin partiel

// les methodes qui sont virtuelles dans la classe de base
void refresh_proxy();
double get_Umin();
double get_Umax();
double get_Vmin();
double get_Vmax();
void draw( cairo_t * cai );		// dessin full strip
};

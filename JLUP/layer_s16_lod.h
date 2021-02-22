// layer_s16_lod : une courbe a pas uniforme en signed 16 bits (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)
// l'allocation mémoire est securité zéro

class lod {	// un niveau de detail
public :
short * max;
short * min;	// des couples min_max
int qc;		// nombre de couples min_max
int kdec;	// decimation par rapport a l'audio original
// constructeur
lod() : max(NULL), min(NULL), qc(0), kdec(1) {};
// methodes
void allocMM( size_t size );	// ebauche de service d'allocation
};

class layer_s16_lod : public layer_base {
public :

short * V;
int qu;			// nombre de points effectif
int curi;		// index point courant
double linewidth;	// 0.5 pour haute resolution, 1.0 pour dessin plus rapide
double spp_max;		// nombre max de sample par pixel (samples audio ou couple minmax)
			// s'il est trop petit, on a une variation de contraste au changement de LOD
			// s'il est trop grand on ralentit l'affichage
			// bon compromis  : 0.9 / linewidth
vector <lod> lods;	// autant le LODs que necessaire
int ilod;		// indice du LOD courant, -1 pour affichage a pleine resolution
			// -2 pour inconnu (choix ilod a faire)

// constructeur
layer_s16_lod() : layer_base(), V(NULL), qu(0), curi(0), linewidth(0.7), spp_max(0.9/linewidth),
		  ilod(-2) {};

// methodes propres a cette classe derivee
int make_lods( unsigned int klod1, unsigned int klod2, unsigned int minwin );	// allouer et calculer les LODs
void find_ilod();			// choisir le LOD optimal (ilod) en fonction du zoom
int goto_U( double U0 );		// chercher le premier point U >= U0
void goto_first();
int get_pi( double & rU, double & rV );	// get XY then post increment
// void scan();				// mettre a jour les Min et Max
// void allocV( size_t size );		// ebauche de service d'allocation DEPRECATED

void draw( cairo_t * cai, double tU0, double tU1 );	// dessin partiel

// les methodes qui sont virtuelles dans la classe de base
void refresh_proxy();
double get_Umin();
double get_Umax();
double get_Vmin();
double get_Vmax();
void draw( cairo_t * cai );				// dessin full strip
};

// les classes pour le projet JLUPLOT
// en general on met les noms de classes en anglais, les membres en fr


/*
====================== Spec de JLUPLOT 2 ======================================

0) hierarchie
   jluplot trace un panel contenant une ou plusieurs strips, contenant chacune une ou plusieurs layers

	(cairo surface) > panel > strip > layer

   la graduation X est commune a tous les strips, la graduation Y a tous les layers d'un strip
   (si on veut superposer 2 courbes avec echelle Y differente, il faut superposer 2 strips (si c'est supporte un jour))

   cette hiérarchie donne lieu a une double indirection :
	dans un strip : vector < layer_base * > courbes;
	dans un panel : vector < strip * > bandes;
   classes de base :
	layer_base est abstraite - les layers operationnels sont definis dans l'unite layer.h
	strip est operationnelle

1) contexte graphique
   les fonctions graphiques recoivent un pointeur qui sert a leur passer le contexte graphique, ici un cairo *
   ce contexte peut appartenir à une cairo surface, parmi GTK drawing area, GDK pixmap, PDF surface
   le code jluplot est independant de la nature de cette surface (qui est determinee par gluplot)

   cette version a un placeholder pour pointer sur une zoombar horizontale facultative (sans etre aware de sa nature)
   pour pouvoir la notifier d'un zoom horizontal effectue

2) 4 espaces de coordonnees 2D :
	- l'espace UV ou espace source : les coordonnees des elements d'un layer tels qu'ils sont disponibles
	  par exemple pour un signal echantillonne, U c'est l'index des echantillons
	- l'espace MN ou espace utilisateur : ce qui sert de reference pour aligner les layers dans un strip,
	  et les strips horizontalement dans un panel (meme si leurs espaces UV sont incompatibles).
	  par exemple pour un signal temporel, M est le temps en s.
	- l'espace XY ou espace ecran : les coordonnées en unites d'affichage, pixels (ou points en PDF)
	  chaque strip a son origine au coin inferieur gauche (X+ vers la droite, Y+ vers le haut)
	- l'espace QR ou espace graduations : normalement identique a MN, sauf graduation "alternative"
   definition des transformations primaires :
	- les coeffs de transformation MN <--> UV sont stockes dans chaque layer
			m0 et km	t.q.	u = ( m - m0 ) * km
			n0 et kn	t.q.	v = ( n - n0 ) * kn
	  ils sont mis a jour en meme temps que les donnees
	- les coeffs de transformation MN <--> XY sont stockes respectivement dans :
		panel : x0 et kx	t.q. 	m = ( x - x0 ) * kx
		strip : y0 et ky	t.q.	n = ( y - y0 ) * ky
	  ils sont mis a jour a chaque zoom ou pan
	- les coeffs de transformation QR <--> MN sont stockes respectivement dans :
		panel : q0 et kq	t.q.	m = ( q - q0 ) * kq
		strip : r0 et kr	t.q.	n = ( r - r0 ) * kr
	  ils sont mis a jour par des options de config
   definition des proxies ou transformations secondaires :
	- les coeffs de transformation XY <--> UV sont stockes dans chaque layer :
			u0 et ku	t.q.	x = ( u - u0 ) * ku )
			v0 et kv	t.q.	y = ( v - v0 ) * kv )
	  ils sont mis a jour a chaque changement de m0, km, n0, kn, x0, kx, y0, ky
	  afin de garantir la coherence, les 8 coeffs ci-dessus ne sont accessibles que par get-set

   N.B. ?? window resize et zoom relatif ne marchent que s'il y a eu au moins 1 zoom absolu !
   ATTENTION PIEGE CAIRO : l'espace XY de Cairo est l'espace XY de JLUPLOT au signe de Y pres !!!
   ==> dans le draw() des layers, le signe de Y est change systematiquement avant d'etre passe a Cairo

3) Full Zooms
	- jluplot s'appuie entierement sur les methodes get_Umin(), get_Umax(), get_Vmin(), get_Vmax()
	  que tout layer doit implementer (obligatoire).
	- en general les valeurs par defaut de min et max sont codees en dur dans chaque layer derive,
	  et appliquees par le constructeur du layer
	- chaque layer derive peut implementer une methode scan() pour mettre a jour ses min et max,
	  a charge pour l'application de l'appeler quand les data sont mises a jour

4) PDF plot
	- la methode pdfplot( fnam, caption ) modifie le contexte :
		- pour les elements fixes en pixels : pdf_DPI = 72 ==> 1 pixel -> 1 pt ( 0.353 mm )
		- format A4 landscape (@ pdf_DPI = 72), indep. de la taille de fenetre
		- fond transparent (optcadre = 1)
	  elle restitue le contexte au retour.
*/
class panel;
class strip;

// une couleur pour Cairo
class jcolor {
public :
double dR;
double dG;
double dB;
// constructeurs
jcolor() : dR(0.0), dG(0.0), dB(0.0) {};
jcolor( double gray ) {
  dR = dG = dB = gray;
  }
void set( double R, double G, double B ) {
  dR = R; dG = G; dB = B;
  }
// cree une couleur parmi 8 (noir inclus, pas de blanc)
void arc_en_ciel( int i );
};

// une courbe (ou autre graphique) superposable : classe de base abstraite
class layer_base {
public :
strip * parent;		// au moins pour connaitre dx et dy
// positionnement du layer dans l'espace utilisateur MN <--> UV
private :		// private = unreachable from derived class, si c'est trop dur mettre protected
double m0;	// offset horizontal dans l'user espace (associe a u=0)
double km;	// coeff horizontal user -> source
double n0;	// offset vertical dans l'user space (associe a v=0)
double kn;	// coeff vertical user -> source
public :
// proxy pour acceleration du trace (redondant) XY <--> UV
double u0;	// offset horizontal dans l'espace source (associe a x=0)
double ku;	// coeff horizontal source -> graphe bas niveau X (i.e. pixels)
double v0;	// offset vertical dans l'espace source
double kv;	// coeff vertical source -> graphe bas niveau Y (i.e. pixels)
int visible;	// flag de visibilite

jcolor fgcolor;		// couleur du trace
int ylabel;		// position label
string label;

// constructeur
layer_base() : parent(NULL),	m0(0.0), km(1.0), n0(0.0), kn(1.0),
				u0(0.0), ku(1.0), v0(0.0), kv(1.0),
				visible(1), fgcolor( 0.0 ), ylabel(20), label("") {};
// destructeur
virtual ~layer_base() { printf("layer_base destructor called\n"); fflush(stdout); };

// methodes de conversion essentielles
double UdeM( double m ) { return( ( m - m0 ) * km );  };
double VdeN( double n ) { return( ( n - n0 ) * kn );  };
double MdeU( double u ) { return( ( u / km ) + m0 );  };
double NdeV( double v ) { return( ( v / kn ) + n0 );  };
// methodes de conversion utilisant le proxy
double XdeU( double u ) { return( ( u - u0 ) * ku );  };
double YdeV( double v ) { return( ( v - v0 ) * kv );  };
double UdeX( double x ) { return( ( x / ku ) + u0 );  };
double VdeY( double y ) { return( ( y / kv ) + v0 );  };

virtual void refresh_proxy();	// virtual pour etre appelee en fonction du type reel de l'objet
				// mais les classes derivees ne sont pas obligees de la redefinir

void set_m0( double M0 ) { m0 = M0; refresh_proxy(); };
void set_km( double kM ) { km = kM; refresh_proxy(); };
void set_n0( double N0 ) { n0 = N0; refresh_proxy(); };
void set_kn( double kN ) { kn = kN; refresh_proxy(); };

virtual double get_Umin() = 0;	// les classes derivees sont obligees de redefinir
virtual double get_Umax() = 0;
virtual double get_Vmin() = 0;
virtual double get_Vmax() = 0;

virtual void draw( cairo_t * cai ) = 0;	// dessin
// dump
virtual void dump() {
  printf("      layer %s, %s\n", label.c_str(), (visible?"visible":"invisible") );
  printf("\tm0=%-12.5g km=%-12.5g n0=%-12.5g kn=%-12.5g\n", m0, km, n0, kn );
  printf("\tu0=%-12.5g ku=%-12.5g v0=%-12.5g kv=%-12.5g\n", u0, ku, v0, kv );
  };
};

// un strip, pour heberger une ou plusieurs courbes superposees
class strip {
public :
panel * parent;		// au moins pour connaitre dx
// positionnement vertical
private :
double y0;		// position y (pixels) de l'origine user n=0
double ky;		// coeff vertical y -> user
public :
double kmfn;		// coeff de marge visuelle pour le fullN (t.q. 5% style jluplot 0)
double r0;		// offset vertical graduations, valeur associee a n=0
double kr;		// coeff vertical graduations --> user

double tdr;		// intervalle ticks Y dans l'espace graduations R
unsigned int qtky;	// nombre de ticks
unsigned int subtk;	// nombre de subticks par tick
jcolor bgcolor;		// couleur du fond ou du cadre (selon optcadre)
jcolor lncolor;		// couleur du reticule
int force_redraw;	// demande mise a jour du trace vectoriel
vector < layer_base * > courbes;
unsigned int fdy;	// full_dy = hauteur totale du strip (pixels)
unsigned int ndy;	// net_dy  = hauteur occupee par la courbe (pixels) = fdy - hauteur graduations
int optX;		// option pour l'axe X : 0 <==> ne pas mettre les graduations
int optretX;		// option pour reticule X : 0 ==> rien, 1 ==> lignes sous le layer, 2 ==> lignes sur le layer  
int optretY;		// option pour reticule Y : idem
int optcadre;		// option cadre : 0 ==> fill, 1 ==> fond transparent, cadre au trait
int visible;
string Ylabel;
// constructeur
strip() : parent(NULL), y0(0.0), ky(1.0), kmfn(0.05), r0(0.0), kr(1.0),
	  tdr(10.0), qtky(11), subtk(1), bgcolor( 1.0 ), lncolor( 0.5 ),
	  force_redraw(1), fdy(100), ndy(100), optX(0), optretX(1), optretY(1), optcadre(0), visible(1) {};
// destructeur virtuel
virtual ~strip() {}; 
// methodes
double NdeY( double y ) { return( ( y - y0 ) * ky );  };
double YdeN( double n ) { return( ( n / ky ) + y0 );  };
double NdeR( double r ) { return( ( r - r0 ) * kr );  };
double RdeN( double n ) { return( ( n / kr ) + r0 );  };

void refresh_proxies();
void set_y0( double Y0 );
void set_ky( double kY );
double get_y0() { return y0; };
double get_ky() { return ky; };

void add_layer( layer_base * lacourbe ) {
lacourbe->parent = this;
courbes.push_back( lacourbe );
}

// dump
void dump() {
unsigned int i;
printf("   strip fdy=%d, %s, %d layers\n", fdy, (visible?"visible":"invisible"), courbes.size() );
printf("\ty0=%-12.5g ky=%-12.5g r0=%-12.5g kr=%-12.5g\n", y0, ky, r0, kr );
for ( i = 0; i < courbes.size(); i++ )
    courbes.at(i)->dump();
}

void zoomN( double nmin, double nmax );	// zoom absolu
void zoomY( double ymin, double ymax );	// zoom relatif
void panY( double dy );			// pan relatif
void zoomYbyK( double k );		// zoom par un facteur
void panYbyK( double k );		// pan par un facteur
void fullN();
void presize( int rendy );	// met a jour la hauteur nette en pixels
void resize( int rendy );	// met a jour la hauteur puis le zoom
// dessin
virtual void draw( cairo_t * cai );
// event
int clicY( double y, double * py );
void reticule_X( cairo_t * cai );	// tracer le reticule x : la grille de barres verticales
void gradu_X( cairo_t * cai );		// les textes de l'axe X
void reticule_Y( cairo_t * cai );	// tracer le reticule y : la grille de barres horizontales
void gradu_Y( cairo_t * cai );		// les textes de l'axe Y
};


// le panel, pour heberger plusieurs strips (axe X commun ou identique)
class panel {
public :
double fullmmin;		// M min du full zoom (pour la zoombar)
double fullmmax;		// M max du full zoom   "        "

// positionnement horizontal
private :
double x0;		// position x (pixels) de l'origine user m=0
double kx;		// coeff horizontal x -> user
public :
double q0;		// offset horizontal graduations, valeur associee a m=0
double kq;		// coeff horizontal graduations --> user

double tdq;		// intervalle ticks X dans l'espace graduations QR
unsigned int qtkx;	// nombre de ticks
int full_valid;		// indique que les coeffs de transformations sont dans un etat coherent
int force_redraw;	// demande mise a jour du trace vectoriel

vector < strip * > bandes;

unsigned int fdx;	// full_dx = largeur totale (pixels) mx inclus
unsigned int fdy;	// full_dy = hauteur totale (pixels)
unsigned int ndx;	// net_dx  = largeur nette des graphes (pixels) mx deduit

// details visuels
unsigned int mx;	// marge x pour les textes a gauche (pixels)
			// sert a translater le repere pour trace des courbes
unsigned int my;	// marge pour les textes de l'axe X (pixels)
unsigned int pdf_DPI;	// conversion des pixels t.q. pdf_DPI = 72 <==> 1 pix = 1 point

// zoombar X optionnelle
void * zoombar;		// pointeur sur l'objet, a passer a la callback
void(* zbarcall)(void*, double, double); 	// callback de zoom normalise

// constructeur
panel() : x0(0.0), kx(1.0), q0(0.0), kq(1.0),
	  tdq(10.0), qtkx(11), full_valid(0), force_redraw(1),
	  fdx(200), fdy(200), mx(60), my(20), pdf_DPI(72),
	  zbarcall(NULL)
	  { ndx = fdx - mx; };
// methodes
double MdeX( double x ) { return( ( x - x0 ) * kx );  };
double XdeM( double m ) { return( ( m / kx ) + x0 );  };
double MdeQ( double q ) { return( ( q - q0 ) * kq );  };
double QdeM( double m ) { return( ( m / kq ) + q0 );  };

void refresh_proxies();
void set_x0( double X0 );
void set_kx( double kX );
double get_x0() { return x0; };
double get_kx() { return kx; };

void add_strip( strip * labande ) {
labande->parent = this;
bandes.push_back( labande );
}

// dessin
void zoomM( double mmin, double mmax );	// zoom absolu
void zoomX( double xmin, double xmax );	// zoom relatif
void panX( double dx );			// pan relatif
void zoomXbyK( double k );	// zoom par un facteur
void panXbyK( double k );	// pan par un facteur
void fullM();
void fullMN();
void presize( int redx, int redy );	// met a jour les dimensions en pixels
void resize( int redx, int redy );	// met a jour les dimensions en pixels puis les zooms
void draw( cairo_t * cai );
// event
int clicXY( double x, double y, double * px, double * py );
int clicMN( double x, double y, double * pM, double * pN );
int clicQR( double x, double y, double * pQ, double * pR );
// PDF
int pdfplot( const char * fnam, const char * caption );	// rend 0 si Ok
// dump
void dump() {
  unsigned int i;
  printf("panel fdx=%d fdy=%d, %d strips\n", fdx, fdy, bandes.size() );
  printf("\tx0=%-12.5g kx=%-12.5g q0=%-12.5g kq=%-12.5g\n", x0, kx, q0, kq );
  for	( i = 0; i < bandes.size(); i++ )
	bandes.at(i)->dump();
  };
};

// constantes pour valeur retour methode clicXY
#define CLIC_OUT_X		-1
#define CLIC_OUT_Y		-2
#define CLIC_MARGE_GAUCHE	0x40000000
#define CLIC_MARGE_INF		0x20000000
#define CLIC_MARGE		(CLIC_MARGE_GAUCHE|CLIC_MARGE_INF)
#define CLIC_ZOOMBAR		0x10000000	// pour le cas ou context menu est commun avec zoombar

// fonctions non-membres diverses

// trouver une valeur de tick appropriee
double autotick( double range, unsigned int nmax );

// conversion double-->texte, avec un format de precision
// en rapport avec la valeur du tick.
// si la valeur du tick est nulle ou absente, utilisation
// du format anterieur memorise
// retour = nombre de chars utiles
// lbuf doit pouvoir contenir 32 bytes
int scientout( char * lbuf, double val, double tick=0.0 );

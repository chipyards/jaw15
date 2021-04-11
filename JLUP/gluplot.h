/*
====================== Spec de GLUPLOT 2 ======================================

GLUPLOT est une couche au dessus de JLUPLOT, dont le but est :
	- heberger tout ce qui est specifique de GTK-GDK (jluplot ne depend que de Cairo)
	  en particulier les event-callbacks
En plus cette version apporte :
	- piloter JLUPLOT pour lui faire faire un trace vectoriel intermediaire sur un drawpad GDK,
	  en vue d'accelerer les rafraichissement (lorsqu'il n'y a pas zoom ni pan)
	  (option offscreen_flag)
	- supporter un overlay "fantome" temporaire t.q. rectangle de selection 
	- supporter le trace incremental sur l'ecran (drawing area) pour le curseur "audio play",
	  ce qui implique de desactiver temporairement le double-buffer
	- supporter un widget auxiliaire "zoombar"

Implementation :
	- la classe gpanel est derivee de la classe panel de JLUPLOT.
	- sa methode draw() dessine sur le drawpad (si necessaire) en iterant les appels a strip::draw()
	  comme ferait panel::draw() (qui n'est pas utilisee par gpanel::draw() )
	- l'allocation memoire pour le drawpad est automatique via gpanel::drawpad_resize(),
	  appele automatiquement par gpanel::draw()
	- sa methode gpanel::paint() est au niveau au-dessus,
		elle peut appeller gpanel::draw() ou panel::draw() selon l'option offscreen_flag
		elle trace les overlays t.q. rectangle de selection et curseur audio 
	- les niveaux strip et layer ne sont pas impactes par le passage par un drawpad
	- il y a 2 modes de copie du drawpad sur l'ecran : B1 et B2, le choix est fait par le membre
	  drawab ( drawab == NULL ==> B2 ) - normalement pas d'impact sur le resultat visuel

Callback :
	- l'application peut enregistrer 2 callbacks :
		- clic_call_back : pour interpreter les clics sur le panel
		  N.B. sont deja interceptes pour zoom et pan : wheel, right drag, left drag + spacebar
		- key_call_back : pour les actions clavier
		  N.B. 'f' est deja interceptee pour full zoom XY

Concepts :
	- "ecran" ou "drawing area" : zone de frame buffer, visualisee en direct ou via un frame buffer cache
	  peut supporter du dessin incremental de petits motifs seulement (curseur), en single-buffer
	- drawpad : zone de memoire aka tampon graphique, supporte dessin incremental
	- draw : dessin vectoriel (au sens large, peut inclure de l'image)
	- paint : mise a jour de la drawing area, en mixant copie de drawpad et dessin vectoriel
	- automatique : methode capable de decider automatiquement si elle a quelque chose a faire
	  on peut l'appeler "a toutes fins utiles"

Methode gpanel::paint :
	Cette methode automatique met a jour la "drawing area" dite "ecran" (une zone du frame buffer en fait)
	Elle est destinee a etre appelee periodiquement (typiquement 30 fois par seconde)
	Elle est econome, s'il n'y a rien a faire elle ne fait rien
	Elle superpose 3 couches, dans cet ordre :
		1) les strips, soit par draw direct soit par copie du drawpad (offscreen buffer)
		2) un motif fantome t.q. rectangle de zoom ou de selection ou curseur manuel
		3) un curseur audio et la zone qu'il occupait auparavent
	Ces actions dependent de flags
		1) les strips seulement s'il y a force_repaint ou drag.mode
		2) le fantome s'il y a drag.mode
		3) la zone quittee par le curseur si xdirty>=0.0, le curseur si xcursor>=0.0
		   de plus s'il n'y a ni force_repaint ni drag.mode, la couche 3 sera tracee en single-buffer 
	Pour les strips il y a 2 modes possibles :
		offscreen_flag == 0 : mode direct, le draw est fait directement sur la drawing area
		offscreen_flag == 1 : le draw est fait sur un drawpad qui est copie sur la drawing area,
		le but est d'economiser le couteux dessin vectoriel dans les cas ou le repaint est demande
		seulement parceque :
			- la fenetre est exposee
			- le fantome a bouge
			- le curseur audio a bouge 
		pour forcer le rafraichissement vectoriel du drawpad, utiliser le flag force_redraw,
		soit au niveau du panel soit au niveau du strip
	Notes :
		- les flags force_redraw n'ont pas d'effet en mode direct (offscreen_flag == 0)
		- l'animation du curseur audio n'est praticable qu'avec le drawpad (offscreen_flag == 0)
paint "queue" :
	Pour eviter de faire plusieurs paints consecutifs dans le meme tour de boucle, l'appel de
	la methode paint est centralise dans l'idle function, celle-ci va appeler paint() seulement si :
		- le curseur a bouge, ou
		- un event a mis force_repaint a 1
	N.B. pour mettre a jour seulement le curseur, appeler paint() avec force_repaint = 0
PDF plot :
	gpanel::pdf_modal_layout offre un dialogue incluant GTK file chooser,
	si on n'aime pas, appeler panel::pdfplot() de jluplot  

*/

// trouver l'index d'un element dans un vecteur (a condition que les elements supportent l'operateur == )
// en pratique c'est pour utiliser avec un vecteur de pointeurs
template <typename vectype> int vectindex( vector<vectype> * v, vectype elem )
{
for	( unsigned int i = 0; i < v->size(); ++i )
	if	( v->at(i) == elem )
		return i;
return -1;
}


// ghost_drag : un rectangle fantome dragable pour selection ou zoom relatif
#define MIN_DRAG 5	// limite entre clic et drag en pixels
enum ghost_dragmode { nil, select_zone, pan, zoom };
class ghost_drag {
public :
ghost_dragmode mode;
double x0;
double y0;
double x1;
double y1;
// constructeur
ghost_drag() : mode(nil) {};
// methodes
void zone( cairo_t * cair );
void box( cairo_t * cair );
void line( cairo_t * cair );
void draw( cairo_t * cair );	// automatique
};

// gstrip derive du strip de jluplot
class gstrip : public strip {
public :
// widgets associes
GtkWidget * smenu_y;    // scale menu : menu contextuel pour les echelles Y
// constructeur
gstrip() : smenu_y(NULL) {};

void add_layer( layer_base * lacourbe );
};

// gpanel derive du panel de jluplot
class gpanel : public panel {
public :
GtkWidget * widget;
GdkRegion * laregion;	// pour gestion double-buffer a la demande
GdkPixmap * drawpad;	// offscreen drawable N.B. pas la peine de stocker ses dimensions on peut lui demander
cairo_t * offcai;	// le cairo persistant pour le drawpad
// special B1 (si drawab est NULL on est en B2)
GdkWindow * drawab;	// GDK drawable de la drawing area contenant le panel
GdkGC * gc;		// GDK drawing context
// widgets associes
GtkWidget * smenu_x;    // scale menu : menu contextuel pour les echelles X
GtkWidget * gmenu;	// global menu : menu contextuel principal
ghost_drag drag;
// flags et indicateurs
int offscreen_flag;	// autorise utilisation du buffer offscreen aka drawpad
int force_repaint;	// zero pour dessin cumulatif en single-buffer
double xcursor;		// position demandee pour le curseur tempporel, abcisse en pixels
double xdirty;		// region polluee par le curseur temporel, abcisse en pixels
int paint_cnt;		// profilage
// variables
void (*clic_call_back)(double,double,void*);	// pointeur callback pour clic sur courbe
void (*key_call_back)(int,void*);		// pointeur callback pour touche clavier
void * call_back_data;			// pointeur a passer aux callbacks

// constructeur (N.B. l'initialisation est obligatoirement completee par la methode layout() )
gpanel() : laregion(NULL), drawpad(NULL), offcai(NULL), drawab(NULL), gc(NULL),
	   offscreen_flag(1), force_repaint(1), xcursor(-1.0), xdirty(-1.0),
	   paint_cnt(0), clic_call_back(NULL), key_call_back(NULL) {};

// methodes
GtkWidget * layout( int w, int h );

void add_strip( gstrip * labande ) {
	panel::add_strip( labande );
	labande->smenu_y = mksmenu("Y AXIS");
	}

void configure();
void expose();
void toggle_vis( unsigned int ib, int ic );
void copy_gmenu2visi();		// copie les checkboxes du menu contextuel vers les flags 'visible' 
void paint();			// copie automatique du drawpad sur la drawing area
void draw();			// dessin vectoriel automatique sur le drawpad
void drawpad_resize();		// automatique, alloue ou re-alloue le pad, cree le cairo
void drawpad_compose( cairo_t * cai );	// copier le drawpad entier sur l'ecran
void cursor_zone_clean( cairo_t ** cai );// copier une petite zone du drawpad sur l'ecran
// methodes pour repondre aux events
void clic( GdkEventButton * event );
void motion( GdkEventMotion * event );
void wheel( GdkEventScroll * event );
void key( GdkEventKey * event );
// png service methods
void png_save_drawpad( const char * fnam );
// pdf service widgets
GtkWidget * wchoo;
GtkWidget * fchoo;
GtkWidget * edesc;
// pdf service methods
void pdf_modal_layout( GtkWidget * mainwindow );
void pdf_ok_call();
// contex menu service
GtkWidget * mksmenu( const char * title );
GtkWidget * mkgmenu();
static void smenu_set_title( GtkWidget * lemenu, const char *titre );
int selected_strip;	// strip duquel on a appele le menu (flags de marges inclus)
// bindkey service
int selected_key;	// touche couramment pressee
// callback service
void clic_callback_register( void (*fon)(double,double,void*), void * data );
void key_callback_register( void (*fon)(int,void*), void * data );
// dev utility
void dump() {
	panel::dump();
	printf("offscreen_flag = %d\n", offscreen_flag );
	};
};

// la zoombar, en version horizontale
class gzoombar {
public :
GtkWidget * widget;
gpanel * panneau;
int ww;	// largeur drawing area
double ndx;	// largeur nette
double xm;	// marge
double dxmin;	// barre minimale
double xlong;	// limite barre "longue"
double x0;	// extremite gauche
double x1;	// extremite droite
double xc;	// clic (debut drag)
double x0f;	// extremite gauche fantome
double x1f;	// extremite droite fantome
int opcode;	// 0 ou 'L', 'M', 'R'
// constructeur
gzoombar() : panneau(NULL), ww(640), xm(12), dxmin(3), xlong(36), opcode(0) {
	x0 = 100.0; x1 = 200.0;
	ndx = (double)ww - ( 2.0 * xm );
	};
// methodes
GtkWidget * layout( int w );
void configure();
void expose();
void clic( GdkEventButton * event );
void motion( GdkEventMotion * event );
void wheel( GdkEventScroll * event );
int op_select( double x );	// choisir l'operation en fonction du lieu du clic
void update( double x );	// calculer la nouvelle position de la barre (fantome)
void zoom( double k0, double k1 );	// mettre a jour la barre en fonction d'un cadrage normalise
};

// fonction C exportable
void gzoombar_zoom( void * z, double k0, double k1 );


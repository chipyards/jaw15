/*
====================== Spec de GLUPLOT 2 ======================================

GLUPLOT est une couche au dessus de JLUPLOT, dont le but est :
	- heberger tout ce qui est specifique de GTK-GDK (jluplot ne depend que de Cairo)
	  en particulier les event-callbacks
En plus cette version apporte :
	- piloter JLUPLOT pour lui faire faire un trace vectoriel intermediaire sur un drawpad GDK,
	  en vue d'accelerer les rafraichissement (lorsqu'il n'y a pas zoom ni pan)
	- supporter le trace incremental sur l'ecran (drawing area) pour le curseur "audio play",
	  ce qui implique de desactiver temporairement le double-buffer
	- supporter un widget auxiliaire "zoombar"

Implementation :
	- la classe gpanel est derivee de la classe panel de JLUPLOT.
	  sa methode draw dessine sur le drawpad (si necessaire) en iterant les appels a strip::draw()
	  comme ferait panel::draw() (qui n'est pas utilisee)
	- les niveaux strip et layer ne sont pas concernes par le passage par un drawpad
	- il y a 2 modes de copie du drawpad sur l'ecran : B1 et B2, le choix est fait par le membre
	  drawab ( drawab == NULL ==> B2 ) - normalement pas d'impact sur le resultat visuel

Concepts :
	- "ecran" ou "drawing area" : zone de frame buffer, visualisee en direct ou via un frame buffer cache
	  peut supporter du dessin incremental de petits motifs seulement (curseur), en single-buffer
	- drawpad : zone de memoire aka tampon graphique, supporte dessin incremental
	- draw : dessin vectoriel (au sens large, peut inclure de l'image)
	- paint : mise a jour de la drawing area, en mixant copie de drawpad et dessin vectoriel
	- automatique : methode capable de decider automatiquement si elle a quelque chose a faire
	  on peut l'appeler "a toutes fins utiles"


----------------------- Actions de GTK GUI :
IDLE FUNCTION
si le curseur a bouge : (que ce soit a cause de playing ou autre motif)
	met a jour gpanel::xcursor
	met gpanel::queue_flag a 1
si gpanel::queue_flag :
	met gpanel::queue_flag a 0
	appelle gpanel::paint()


*/

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
GtkWidget * menu1_x;
GtkWidget * menu1_y;
ghost_drag drag;
// flags et indicateurs
int offscreen_flag;	// autorise utilisation du buffer offscreen aka drawpad
int force_repaint;	// zero pour dessin cumulatif en single-buffer
int queue_flag;		// flag de redraw optimise
double xcursor;		// position demandee pour le curseur tempporel, abcisse en pixels
double xdirty;		// region polluee par le curseur temporel, abcisse en pixels
int paint_cnt;		// profilage
// variables
void (*clic_call_back)(double,double,void*);	// pointeur callback pour clic sur courbe
void (*key_call_back)(int,void*);		// pointeur callback pour touche clavier
void * call_back_data;			// pointeur a passer aux callbacks
// constructeur
gpanel() : laregion(NULL), drawpad(NULL), offcai(NULL), drawab(NULL), gc(NULL),
	   offscreen_flag(1), force_repaint(1), queue_flag(0), xcursor(-1.0), xdirty(-1.0),
	   paint_cnt(0), clic_call_back(NULL), key_call_back(NULL) {};
// methodes
GtkWidget * layout( int w, int h );
void configure();
void expose();
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
// pdf service widgets
GtkWidget * wchoo;
GtkWidget * fchoo;
GtkWidget * edesc;
// pdf service methods
void pdf_modal_layout( GtkWidget * mainwindow );
void pdf_ok_call();
// context menu service
GtkWidget * mkmenu1( const char * title );
int selected_strip;	// strip duquel on a appele le menu (avec les flags de marges)
// bindkey service
int selected_key;	// touche couramment pressee
// callback service
void clic_callback_register( void (*fon)(double,double,void*), void * data );
void key_callback_register( void (*fon)(int,void*), void * data );
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


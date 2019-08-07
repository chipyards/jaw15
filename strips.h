
// la classe derivee strip_dp est celle qui supporte le drawpad (off screen drawable)

class strip_dp : public strip {
public :
int offscreen_flag;	// autorise utilisation du buffer offscreen aka drawpad
int dp_dirty_flag;	// demande mise a jour du drawpad
GdkPixmap * drawpad;	// offscreen drawable N.B. pas la peine de stocker ses dimensions on peut lui demander
cairo_t * offcai;	// le cairo persistant pour le drawpad
double old_r0;		// ancienne valeur de r0
double old_kr;		// ancienne valeur de r1
double old_q0;		// ancienne valeur de parent->q0
double old_kq;		// ancienne valeur de parent->q1
// special B1
GdkWindow * drawab;	// GDK drawable de la drawing area contenant le panel
GdkGC * gc;		// GDK drawing context
// constructeur
strip_dp() : strip(), offscreen_flag(1), dp_dirty_flag(1), drawpad(NULL), offcai(NULL),
	     old_r0(0.0), old_kr(0.0), old_q0(0.0), old_kq(0.0),
	     drawab(NULL), gc(NULL) {};
// methodes
void draw( cairo_t * cai );
void drawpad_resize();	// alloue ou re-alloue le pad, cree le cairo
};

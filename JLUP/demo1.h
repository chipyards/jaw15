class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   darea1;
GtkWidget *   zarea1;
GtkWidget *   darea2;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     bpau;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

#define QBUF 		(1<<12)		// taille de buffer
#define BUFMASK 	(QBUF-1)
float Xbuf[QBUF];	// donnees pour l'experience
float Ybuf[QBUF];
double k;

int running;		// scroll continu

// constructeur
glostru() : k(2.0), running(0) {};

// methodes
void gen_data();
void process();


};


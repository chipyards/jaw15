class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   nmain;
GtkWidget *   vpans;   
GtkWidget *     vpan1;
GtkWidget *       darea1;
GtkWidget *       zarea1;
GtkWidget *     darea2;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     braz;

int option_tabs;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

#define QBUF 		(1<<11)		// taille de buffer
float Xbuf[QBUF];	// donnees pour l'experience
float Ybuf[QBUF];
double k;

int running;		// scroll continu
double Uspan;		// etendue fenetre scroll continu

// constructeur
glostru() : option_tabs(0), k(2.0), running(1), Uspan(1000.0) {};

// methodes
void gen_data_point( int razflag=0 );
void layout1();
void layout2();

};


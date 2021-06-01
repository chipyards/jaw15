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

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

int idle_id;		// id pour la fonction idle du timeout

unsigned int qbuf;	// taille de buffer
unsigned int pispan;	// taille de PI dans la reponse impulsionnelle
unsigned int qfir;	// taille de la reponse impulsionnelle 

double * Cbuf;	// donnees pour l'experience
double * Sbuf;
double * Tbuf;
fftw_plan plan;
double k;

// constructeur
glostru() : qbuf(1<<20), pispan(1<<9), qfir(1<<13), Cbuf(0), Sbuf(0), Tbuf(0), plan(0), k(2.0) {};

// methodes
int generate();
void layout1();
void layout2();

};


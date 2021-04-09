class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   darea1;
GtkWidget *   zarea1;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     bpau;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar

unsigned int qbuf;	// taille de buffer
float * Fbuf;	// donnees pour l'experience
short * Sbuf;
unsigned short * Ubuf;

unsigned int ecostroke;


// constructeur
glostru() : qbuf(1024), ecostroke(800) {};

// methodes
void gen_data();
void process();

};


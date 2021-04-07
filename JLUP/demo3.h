class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *       darea1;
GtkWidget *       zarea1;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     braz;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar

#define QBUF 		(48000*60*20)		// taille de buffer
short Sbuf[QBUF];	// donnees pour l'experience
float Fbuf[QBUF];

int k;			// coeff de decimation LODs

// constructeur
// glostru() {};

// methodes
void process();


};


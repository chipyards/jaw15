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

int idle_id;		// id pour la fonction idle du timeout

#define QBUF 		(44100*60*20)	// taille de buffer e.g. 20mn
short Sbuf[QBUF];	// donnees pour l'experience
float Fbuf[QBUF];

int k;			// coeff de decimation LODs

// constructeur
// glostru() {};

// methodes
void process();


};


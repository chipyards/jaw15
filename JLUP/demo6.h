class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   darea1;
GtkWidget *   zarea1;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar

int idle_id;		// id pour la fonction idle du timeout

double * bodeplots[4];	// data buffers for 4 layers
int qsamples;
double fc;		// frequ centrale des filtres
double Q;
double f0;		// f start
double f1;		// f stop
double ppd;		// points per decade

// constructeur
glostru() : qsamples(0), fc(300.0), Q(10.0), f0(10.0), f1(10000.0), ppd(200.0) {};

// methodes
int gen_data();
void process();

};

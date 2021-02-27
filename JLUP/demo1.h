class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   farea;
GtkWidget *   darea;
GtkWidget *   sarea;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     bpau;

int darea_queue_flag;

gpanel panneau;		// panneau principal (wav), dans darea
gzoombar zbar;		// sa zoombar

// une methode
void process();

int running;		// scroll continu

};


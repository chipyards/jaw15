// classe pour fenetre GTK modale
class patch_chooser {
public :
// elements du GUI GTK
GtkWidget * wmain;
GtkWidget *  wscro;
GtkWidget *   tarb;

GtkTreeStore * tmod1;
GtkTreeStore * tmod2;

// constructeur
patch_chooser() : tmod1(NULL), tmod2(NULL) {};

// variables
int tree_first_time;
int patch_number;

// machine 0=GM, 1=Lucina - rend un numero de patch( >=0 ) ou -1 si rien choisi
int choose( int machine, GtkWindow *parent );
};


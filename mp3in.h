

class mp3in {
public:
mpg123_handle *mhand;
int verbose;	// valeurs utiles 2, 3, 4
const char * errfunc;	// (une idee de JLN)
// parametres du mp3
int monosamplesize;	// 2 for 16-bit audio
int qchan;		// 1 ou 2
int fsamp;		// sample rate
off_t estqsamp;		// taille estimee
off_t realqsamp;	// taille apres lecture
size_t outblock;	// taille de buffer recommandee

// constructeur
mp3in() : mhand(NULL), verbose(3), errfunc(""), realqsamp(0)  {};

// methodes
int read_head( const char * fnam );	// renseigne les parametres

int read_data( void * pcmbuf, size_t qpcmbuf ); // remplit 1 buffer

};
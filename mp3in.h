// lecture d'un fichier mp3, en 2 etapes
//	- lecture des parametres
//	- iteration sur la lecture des audio samples
// N.B. les methodes read_data rendent des samples entrelaces dans le cas d'un clip stereo
//	read_data_b prend la taille en bytes et rend le nombre de bytes lus
//	read_data_p prend la taille en pcm frames et rend la quantite lue idem
// les deux mettent a jour realpfr.
// N.B. "pcm frame" c'est 1 sample en mono, une paire en stereo
// ne pas confondre : 1 mp3 frame = 1152 pcm frames
//
#include "audiofile.h"

class mp3in : public audiofile {
public:
/* attributs de la classe de base
unsigned int monosamplesize;	// 2 for 16-bit audio
unsigned int qchan;		// 1 ou 2
unsigned int fsamp;		// sample rate
unsigned int estpfr;	// taille estimee  (PCM frames)
unsigned int realpfr;	// taille apres lecture
*/
mpg123_handle *mhand;
int verbose;	// valeurs utiles 2, 3, 4
const char * errfunc;	// (une idee de JLN)
// parametres du mp3
size_t outblock;	// taille de buffer recommandee (bytes)

// constructeur
mp3in() : audiofile(), mhand(NULL), verbose(3), errfunc("") {};

// methodes
int read_head( const char * fnam );	// renseigne les parametres

int read_data_b( void * pcmbuf, size_t qpcmbuf ); // remplit 1 buffer
int read_data_p( void * pcmbuf, size_t qpfr ); // remplit 1 buffer
};
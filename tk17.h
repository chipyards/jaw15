// le but de cette classe est de modifier le json obtenu
// par decodage des .pes du pose editor

enum tangent_t { SMOOTH=0, LIN, FLAT };

#define QCHAR (1<<20)

class tk17 {
public:
// file names
const char * src_fnam;
const char * dst_fnam;
// buffer for full input file
#define QCHAR (1<<20)
char json_src[QCHAR];
int src_size;

// key concepts
int perso;
int itrack;

// animated value
float * X;
int qframes;	// nombre de frames de la sequence
float min_X;
float max_X;

// constructeur
tk17() : src_fnam(NULL), dst_fnam(NULL), src_size(0), perso(1), itrack(126),
	X(NULL), qframes(0), min_X(0.04), max_X(0.5) {};

// methodes
void gen1key( FILE *fil, int ifram, double x, double y, double z, tangent_t tangent );
void gen1trk_head( FILE *fil );
void gen1trk( FILE *fil, double seuil = 0.0 );
int json_load( const char * fnam );
int json_patch();
};

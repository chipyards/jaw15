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

// stable parameters
int perso;
double neck_angle;	// sigma des rotations random neck (degres)
int neck_period;	// periode moyenne des rotations random neck (frames)
int blink_period;	// periode moyenne des eye blink

// continuous animation (jaw open)
float * X;
int qframes;	// nombre de frames de la sequence
float min_X;
float max_X;

// constructeur
tk17() : src_fnam(NULL), dst_fnam(NULL), src_size(0),
	perso(1), neck_angle(2.0), neck_period(19), blink_period(40),
	X(NULL), qframes(0), min_X(0.04), max_X(0.5) {};

// methodes generiques
void gen1key( FILE *fil, int ifram, double x, double y, double z, tangent_t tangent );
void gen1trk_head( FILE *fil, int itrack );
int json_load( const char * fnam );
int json_search( int itrack, int * begin, int * end );
// methodes specifiques par track
void gen_jaw_trk_126( FILE *fil, double seuil = 0.0 );
void gen_neck_trk_15( FILE *fil );
// main
int json_patch();
};


// utilitaires

// generation d'un double pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
double random_normal_double( double avg, double sigma );

// generation d'un entier pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
int random_normal_int( int avg, int sigma );

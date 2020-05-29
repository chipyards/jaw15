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
double hips_angle;	// sigma des rotations random de bassin (degres)
int hips_period;	// periode moyenne des rotations de bassin (frames)
double neck_angle;	// sigma des rotations random neck (degres)
int neck_period;	// periode moyenne des rotations neck (frames)
double shoulders_angle;	// sigma des haussements d'epaule (degres)
int shoulders_period;	// periode moyenne des haussements d'epaule
int blink_period;	// periode moyenne des eye blink
double breath_breadth;	// amplitude respiration, "vertebre 3"

// continuous animation (jaw open)
float * X;
int qframes;	// nombre de frames de la sequence
float min_X;
float max_X;
// discrete animation : eye blink
char blink_flag[1152];	// 1 flag par frame : 0 ou 1
// discrete animation : breath
char breath_flag[1152]; // 1 flag par frame : -1 (debut inspire), 0 ou +1 (debut expire)
// discrete animation : shoulders up
float shoulders[1152]; // 1 val par frame : 0.0 = no event

// constructeur
tk17() : src_fnam(NULL), dst_fnam(NULL), src_size(0), perso(1),
	hips_angle(2.0), hips_period(23), neck_angle(1.0), neck_period(19),
	shoulders_angle(2.0), shoulders_period(26),
	blink_period(40), breath_breadth(0.004),
	X(NULL), qframes(0), min_X(0.04), max_X(0.5) {
	for	( int i = 0; i < 1152; ++i )
		{ blink_flag[i] = breath_flag[i] = 0; shoulders[i] = 0.0; }
	};

// methodes generiques
void gen1key( FILE *fil, int ifram, double x, double y, double z, tangent_t tangent );
void gen1trk_head( FILE *fil, int itrack );
void gen1trk_tail( FILE *fil );
void gen_3D_trk( FILE *fil, int itrack, double sigma_X, double sigma_Y, double sigma_Z, double Y0, int period );
int json_load( const char * fnam );
int json_search( int itrack, int * begin, int * end );
int json_search_1vect( int pos, double * x, double * y, double * z );

// methodes specifiques par track
void gen_hips_trk_1( FILE *fil, double y0 );
void gen_neck_trk_15( FILE *fil );
void gen_shoulders_events();
void gen_shoulders_trk_17_18( FILE *fil, int itrack );
void gen_jaw_trk_126( FILE *fil, double seuil = 0.0 );
void gen_blink_events();
void gen_blink_trk_136_137( FILE *fil, int itrack );
void gen_breath_events( double seuil = 0.0 );
void gen_breath_trk_152( FILE *fil );

// main
int json_patch();
};


// utilitaires

// generation d'un table de conversion de timestamps
void generate_timetable( int intro );

// generation d'un double pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
double random_normal_double( double avg, double sigma );

// generation d'un entier pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
int random_normal_int( int avg, int sigma );

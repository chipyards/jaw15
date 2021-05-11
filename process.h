class process {
public:
char wnam[256];		// fichier audio
const char * sf2file;	// fichier soundfont pour fuidsynth
wavio wavp;		// objet pour lecture wav
mp3in m3;		// objet pour lecture mp3
midirender mid;		// objet pour lecture midi et render immediat
audiofile * af;		// pointeur sur wavp ou m3 ou mid
autobuf <short> Lbuf;	// audio brut sl16
autobuf <short> Rbuf;
GdkPixbuf * Lpix;	// spectre sous forme de pixbuf
GdkPixbuf * Rpix;	// spectre sous forme de pixbuf
spectro Lspek;		// un spectrographe
spectro Rspek;		// un spectrographe
unsigned char mutpal[PALSIZE];	// la palette 16 bits --> RGB

// constucteur
process() : sf2file(""), Lpix(NULL), Rpix(NULL) { wnam[0] = 0; };

// methodes
// la partie du process qui traite en memoire les wavs et le spectre

int audiofile_process();	// wav ou mp3 ou mid

int spectrum_compute2D( int force_mono, int opt_lin );

// la partie du process en relation avec jluplot

// layout pour les waveforms
void prep_layout_W( gpanel * panneau );
int connect_layout_W( gpanel * panneau );

// layout pour les spectres
void auto_layout_S( gpanel * panneau, int opt_lin );
void auto_layout2( gpanel * panneau, int time_curs );

// adapte la palette a la limite iend et l'applique a tous les spectres
void palettize( unsigned int iend );

// nettoyage
void clean_spectros();		// liberer la memoire des spectros et pixbufs
};

// utilitaires
void fill_palette_simple( unsigned char * pal, unsigned int iend );
void spectre2rgb( spectro * spek, GdkPixbuf * lepix );


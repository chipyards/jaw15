class process {
public:
char wnam[256];		// fichiers WAV
wavpars wavp;		// structure pour wav_head.c
mp3in m3;		// objet pour lecture mp3
short * Lbuf;		// audio brut sl16
size_t Lallocated;	// espace alloue, en samples
short * Rbuf;		// audio brut sl16
size_t Rallocated;	// espace alloue
int qspek;		// nombre de spectres ( 1 ou 2 )
GdkPixbuf * Lpix;	// spectre sous forme de pixbuf
GdkPixbuf * Rpix;	// spectre sous forme de pixbuf
spectro Lspek;		// un spectrographe
spectro Rspek;		// un spectrographe
unsigned char mutpal[PALSIZE];	// la palette 16 bits --> RGB

// constucteur
process() : Lallocated(0), Rallocated(0), qspek(0), Lpix(NULL), Rpix(NULL) {};

// methodes
// la partie du process qui traite en memoire les wavs et le spectre
int wave_process();
int mp3_process();
int spectrum_compute( int force_mono );

// la partie du process en relation avec jluplot

// layout pour le domaine temporel
void prep_layout_W( gpanel * panneau );
void prep_layout_S( gpanel * panneau );
int connect_layout_W( gpanel * panneau );
int connect_layout_S( gpanel * panneau );
// layout pour le spectre ponctuel
void prep_layout2( gpanel * panneau );
int connect_layout2( gpanel * panneau, int pos );

// adapte la palette a la limite iend et l'applique a tous les spectres
void palettize( unsigned int iend );
};

// utilitaires
void fill_palette_simple( unsigned char * pal, unsigned int iend );
void spectre2rgb( spectro * spek, GdkPixbuf * lepix );


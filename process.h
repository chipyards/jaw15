class process {
public:
char wnam[256];		// fichiers WAV
wavpars wavp;		// structure pour wav_head.c
short * Lbuf;		// audio brut
short * Rbuf;
int qspek;		// nombre de spectres ( 1 ou 2 )
GdkPixbuf * Lpix;	// spectre sous forme de pixbuf
GdkPixbuf * Rpix;	// spectre sous forme de pixbuf
spectro Lspek;		// un spectrographe
spectro Rspek;		// un spectrographe
unsigned char mutpal[PALSIZE];	// la palette 16 bits --> RGB

// methodes
// la partie du process qui traite en memeoire les wavs et le spectre
int wave_process_full();
// la partie du process en relation avec jluplot
// layout pour le domaine temporel
void prep_layout( gpanel * panneau );
int connect_layout( gpanel * panneau );
// layout pour le domaine frequentiel
void prep_layout2( gpanel * panneau );
int connect_layout2( gpanel * panneau, int pos );


// adapte la palette a la limite iend et l'applique a tous les spectres
void palettize( unsigned int iend );
};

// utilitaires
void fill_palette_simple( unsigned char * pal, unsigned int iend );
void colorize( spectro * spek, GdkPixbuf * lepix );


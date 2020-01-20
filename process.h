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

// methodes
// remplissage de la palette et colorisation d'un pixbuf sur le spectre precalcule
void colorize( spectro * spek, GdkPixbuf * lepix, unsigned int iend );
// la partie du process qui traite en memeoire les wavs et le spectre
int wave_process_full();
// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
int connect_layout( gpanel * panneau );

};


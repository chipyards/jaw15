class process {
public:
char wnam[256];		// fichiers WAV
wavpars wavp;		// structure pour wav_head.c
short * Lbuf;		// audio brut
short * Rbuf;

// les attributs relatifs a l'application renoiser
float * Pbuf;		// buffer pour la puissance
unsigned int qpow;	// taille de son contenu
unsigned int pww;	// taille de la fenetre de calcule de puissance en samples
double noise_floor;	// seuil

// methodes
// la partie du process qui traite en memoire les wavs et le spectre
int wave_process_1( tk17 * tk );
int wave_process_2( tk17 * tk );
// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
int connect_layout( gpanel * panneau );
};

// fonctions

// calculer la puissance en fonction du temps, par fenetres de ww samples
// la memoire doit etre allouee pour destbuf
// rend le nombre effectif de samples mis dans destbuf
unsigned int compute_power( short * srcbuf, unsigned int qsamp, unsigned int ww, float * destbuf, double * avg_pow, double * min_pow );

// lecture WAV entier en RAM, assure malloc 1 ou 2 buffers
// :-( provisoire, devrait etre membre de wavpars
int read_full_wav16( wavpars * wavpp, const char * wnam, short ** pLbuf, short ** pRbuf );

// ecriture WAV entier en RAM, wavpp doit contenir les parametres requis par WAVwriteHeader()
// :-( provisoire, devrait etre membre de wavpars
int write_full_wav16( wavpars * wavpp, const char * wnam, short * Lbuf, short * Rbuf );

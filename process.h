class process {
public:
char wnam[256];		// fichiers WAV
wavpars wavp;		// structure pour wav_head.c
short * Lbuf;		// audio brut
short * Rbuf;
gpanel * lepanneau;	// pour reference ulterieure

// les attributs relatifs a l'application
float * Pbuf;		// buffer pour la puissance
unsigned int qpow;	// taille de son contenu
unsigned int pww;	// taille de la fenetre de calcule de puissance en samples
double maxpow;		// stats
double minpow;

// parametres pour le process tk17
tk17 * letk;			// pour reference ulterieure
float lipXbuf[1152];
param_analog intro;		// duree intro en frames
param_analog lipmin;		// range pour track 126
param_analog lipmax;
param_analog sil16;		// seuil de silence (en unites audio sl16)
param_analog hips_angle;	// sigma des rotations random bassin (degres)
param_analog neck_angle;	// sigma des rotations random neck (degres)
param_analog neck_period;	// periode moyenne des rotations random neck (frames)
param_analog shoulders_angle;	// sigma des haussements(degres)
param_analog blink_period;	// periode moyenne des eye blink
param_analog breath_breadth;	// amplitude inspiration (vertebre 3)

// methodes
// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
int connect_layout( gpanel * panneau );
// la partie du process en relation avec la fenetre de param
void prep_layout_pa( param_view * para );
// les actions
int wave_process_1( tk17 * tk );
int wave_process_2();
int wave_process_3();

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

// plus d'explications dans wav_head.c
typedef struct {

unsigned int monosamplesize;	// 2 for 16-bit audio
unsigned int qchan;		// 1 ou 2
unsigned int fsamp;		// sample rate
unsigned int estpfr;	// taille estimee  (PCM frames)
unsigned int realpfr;	// taille apres lecture

int hand;		// file handle	
unsigned int type;	// 1 = signed short int, 3 = float
//unsigned int chan;
//unsigned int freq;	// frequence d'echantillonnage
unsigned int bpsec;	// bytes par seconde
unsigned int block;	// bytes par frame
//unsigned int resol;	// bits par sample
//unsigned int wavsize;	// longueur en echantillons (par canal)
			// = nombre de frames
} wavpars;

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef  __cplusplus
extern "C" {
#endif

void WAVreadHeader( wavpars *s );

void WAVwriteHeader( wavpars *d );

void gasp( const char *fmt, ... );  /* traitement erreur fatale */

#ifdef  __cplusplus
}
#endif

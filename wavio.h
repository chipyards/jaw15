// plus d'explications dans wavio.cpp
#include "audiofile.h"

class wavio : public audiofile {
public:
/* attributs de la classe de base
unsigned int monosamplesize;	// 2 for 16-bit audio
unsigned int qchan;		// 1 ou 2
unsigned int fsamp;		// sample rate
unsigned int estpfr;	// taille estimee  (PCM frames)
unsigned int realpfr;	// taille apres lecture
*/
int hand;		// file handle	
unsigned int type;	// 1 = signed short int, 3 = float
unsigned int bpsec;	// bytes par seconde
unsigned int block;	// bytes par frame

// constructeur
wavio() : audiofile() {};

// methodes
void WAVreadHeader();
void WAVwriteHeader();
};

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef  __cplusplus
extern "C" {
#endif

void gasp( const char *fmt, ... );  /* traitement erreur fatale */

#ifdef  __cplusplus
}
#endif

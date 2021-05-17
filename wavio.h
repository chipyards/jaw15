// plus d'explications dans audiofile.h et wavio.cpp 
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
int writing;		// flag d'ecriture pour afclose()

// constructeur
wavio() : audiofile(), hand(-1), type(1), writing(0) {};

// methodes bas niveau (legacy)
int WAVreadHeader();
void WAVwriteHeader();

// methodes d'interface heritee d'audiofile
int read_head( const char * fnam, int verbose );
int read_data_p( void * pcmbuf, unsigned int qpfr );
void afclose();		// OBLIGATOIRE en cas d'ecriture
// methodes ajoutees
int write_head( const char * fnam );
int write_data_p( void * pcmbuf, unsigned int qpfr );

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

#ifndef AUDIOFILE_H
#define AUDIOFILE_H

class audiofile {
public:
// attributs communs aux types de fichier supportes
unsigned int monosamplesize;	// 2 for 16-bit audio
unsigned int qchan;		// 1 ou 2
unsigned int fsamp;		// sample rate
unsigned int estpfr;	// taille estimee  (PCM frames)
unsigned int realpfr;	// taille apres lecture

// constructeur
audiofile() : realpfr(0) {};

};
#endif

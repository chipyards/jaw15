#ifndef AUDIOFILE_H
#define AUDIOFILE_H

// lecture d'un fichier wav ou mp3, en 2 etapes
//	- lecture des parametres
//	- iteration sur la lecture des audio samples
// N.B. la methodes read_data_p rend des samples entrelaces dans le cas d'un clip stereo
//	read_data_p prend la taille en pcm frames et rend la quantite lue idem
//	et met a jour realpfr.
// N.B. "pcm frame" c'est 1 sample en mono, une paire en stereo
// ne pas confondre : 1 mp3 frame = 1152 pcm frames
//
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

// methodes
virtual int read_head( const char * fnam ) = 0;
virtual int read_data_p( void * pcmbuf, unsigned int qpfr ) = 0;
virtual void afclose() = 0;
};
#endif

// midi-to-RAM renderer, classe derivee de audiofile
// supported sources :
//	- midi file (soon)
//	- program generated sequence
// supported synths :
//	- fluid

#include "../audiofile.h"
#include "fluid.h"
#include "song.h"


class midirender : public audiofile {
public:
fluid flusyn;
song * lesong;

// constructeur
midirender() : audiofile() {};

// methodes

// transmettre au synth les evenements significatifs de l'instant realpfr
// rend -1 si le show est fini
int midiprocess() {
	if	( realpfr == 0 )		// au moins un debut
		{
		flusyn.bank_select( 0, 0 );
		flusyn.program_change( 0, 0 );	// zero-based, subtract 1 from https://en.wikipedia.org/wiki/General_MIDI
		flusyn.noteon( 0, 60, 127 );	// C4 sur piano
		}
	if	( realpfr >= ( fsamp * 10 ) )	// et une fin
		return -1;
	return 0;
	}


// methodes d'interface heritee d'audiofile
int read_head( const char * fnam ) {
	int retval;
	lesong = new song();
	retval = lesong->load( fnam );
	if	( retval ) return retval;
	printf("just read %s\n", fnam );	fflush(stdout);
	lesong->dump( stdout );			fflush(stdout);

	retval = flusyn.init( fsamp );	// le fsamp d'audiofile
	if	( retval ) return retval;
	retval = flusyn.load_sf2();	// selon flusyn.sf2file
	if	( retval ) return retval;
	monosamplesize = 2;		// pour le moment seult s16...
	qchan = 2;			// toujours stereo
	estpfr = 0;
	flusyn.set_gain( 2.0 );

	return 0;
	};

int read_data_p( void * pcmbuf, unsigned int qpfr ) {
	if	( midiprocess() >= 0 )
		{
		// demander qpfr frames au synthe, en stereo entrelacee 16 bits dans 1 seul buffer
		fluid_synth_write_s16( flusyn.synth, qpfr, (short *)pcmbuf, 0, 2, (short *)pcmbuf, 1, 2 );
		realpfr += qpfr;
		return qpfr;
		}
	else	return 0;
	};

void afclose() {
	delete_fluid_synth( flusyn.synth );	// on ne sait pas si cela delete la sf2...
	flusyn.synth = NULL;
	delete_fluid_settings( flusyn.settings );
	flusyn.settings = NULL;
	};
};
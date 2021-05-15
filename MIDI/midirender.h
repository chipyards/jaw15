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

int next_evi;	// next playable event index (in lesong->events_merged)
		// -1 if not playing or beyond end-of-song

// constructeur
midirender() : audiofile(), next_evi(-1) {};

// methodes

void send_event( midi_event * ev );

// methodes d'interface heritee d'audiofile
int read_head( const char * fnam );

int read_data_p( void * pcmbuf, unsigned int qpfr );

void afclose() {
	delete_fluid_synth( flusyn.synth );	// on ne sait pas si cela delete la sf2...
	flusyn.synth = NULL;
	delete_fluid_settings( flusyn.settings );
	flusyn.settings = NULL;
	};
};
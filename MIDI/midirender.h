// midi-to-RAM renderer, classe derivee de audiofile
// supported sources :
//	- midi file
// supported synths :
//	- fluid
// supported audio formats
//	- s16 (avec monosamplesize = 2, defaut )
//	- f32 (avec monosamplesize = 4 )

#include "../audiofile.h"
#include "fluid.h"
#include "song.h"
#include "filter.h"


class midirender : public audiofile {
public:
fluid flusyn;
song * lesong;

int next_evi;	// next playable event index (in lesong->events_merged)
		// -1 if not playing or beyond end-of-song

// constructeur
midirender() : audiofile(), lesong(NULL), next_evi(-1) {};

// methodes

void send_event( midi_event * ev );	// wrapper fluid_synth

double pre_render();			// rend amplitude max

// methodes d'interface heritee d'audiofile
int read_head( const char * fnam, int verbose );	// lecture midi file en RAM (objet song)

int read_data_p( void * pcmbuf, unsigned int qpfr );	// render par blocs, s16 ou f32

void afclose() {
	delete_fluid_synth( flusyn.synth );	// on ne sait pas si cela delete la sf2...
	flusyn.synth = NULL;
	delete_fluid_settings( flusyn.settings );
	flusyn.settings = NULL;
	};
};
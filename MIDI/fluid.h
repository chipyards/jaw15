#ifndef FLUID_H
#define FLUID_H

#include <fluidsynth.h>

class fluid {
public:
fluid_settings_t* settings;
fluid_synth_t* synth;
const char * sf2file;		// on supporte seult 1 SF2 file pour le moment
int sfont_id;

// constructeur
fluid() : settings(NULL), synth(NULL), sf2file(""), sfont_id(-1) {};

// methodes
int init( int fsamp );	// return 0 si ok
int load_sf2();		// return 0 si ok

// parameter methods
void set_gain( double gain ) {
	fluid_synth_set_gain( synth, gain );
	};
	
// Midi event methods
void bank_select( int chan, int bank ) {
	fluid_synth_bank_select( synth,	chan, bank );
	};	// c'est probablement deja 0 par defaut
void program_change( int chan, int patch ) {
	fluid_synth_program_change( synth, chan, patch );
	};
void noteon( int chan, int note, int vel ) {
	fluid_synth_noteon( synth, chan, note, vel );
	};
void noteoff( int chan, int note ) {
	fluid_synth_noteoff( synth, chan, note );
	};
void allsoundoff( int chan ) {
	fluid_synth_all_sounds_off( synth, chan );
	};
};

#endif

/** pour info
MIDI
				ch bank
fluid_synth_bank_select( synth,	0, 0 );		// c'est probablement deja 0 par defaut
fluid_synth_bank_select( synth,	9, 0 );
				   ch prog
fluid_synth_program_change( synth, 0, patch );

fluid_synth_noteon( synth, 0, 60, 127 );	// C4 sur piano
fluid_synth_noteon( synth, 9, 37, 127 );	// 9 = drum kit, 37 = rim shot aka side stick
fluid_synth_noteoff( fluid

AUDIO

// float 32
// demander blocksize frames au synthe, en stereo 32 bits dans 2 buffers
// N.B. pour chaque buffer on donne un offset et un stride
fluid_synth_write_float( synth, blocksize, (float *)Lbuf, 0, 1, (float *)Rbuf, 0, 1 );
// demander blocksize frames au synthe, en stereo entrelacee 32 bits dans 1 seul buffer
// N.B. on donne 2 fois le meme buffer, ce qui change c'est l'offset 
fluid_synth_write_float( synth, blocksize, (float *)buf, 0, 2, (float *)buf, 1, 2 );

// s16
// demander blocksize frames au synthe, en stereo 16 bits dans 1 seul buffer
fluid_synth_write_s16( synth, blocksize, (short *)Lbuf, 0, 1, (short *)Rbuf, 0, 1 );
// demander blocksize frames au synthe, en stereo entrelacee 16 bits dans 1 seul buffer
fluid_synth_write_s16( synth, blocksize, (short *)buf, 0, 2, (short *)buf, 1, 2 );

*/
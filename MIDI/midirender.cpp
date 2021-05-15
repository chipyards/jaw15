
#include <math.h>

using namespace std;
#include <vector>

#include "midirender.h"




// ouverture midifile et copie entiere en RAM dans l'obet lesong
// retour 0 si Ok
int midirender::read_head( const char * fnam )
{
int retval;
lesong = new song();
retval = lesong->load( fnam );
if	( retval ) return retval;
printf("just read %s\n", fnam );	fflush(stdout);
// lesong->dump( stdout );		fflush(stdout);
lesong->merge();
lesong->apply_tempo();
lesong->apply_tempo_u();
lesong->check();			fflush(stdout);
lesong->dump2( stdout );		fflush(stdout);

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

// lire un bloc de qpfr frames (max)
// rend le nombre de frames effectivement produites
// 0 si rendu fini
int midirender::read_data_p( void * pcmbuf, unsigned int qpfr )
{
unsigned int cnt, totalcnt = 0;

/*
if	( next_evi < 0 )
	return 0;
unsigned int next_pfr, current_mf_time;
midi_event * curev;
while	( totalcnt < qpfr )
	{
	// recuperer le timestamp du prochain event
	curev = lesong->events_merged[next_evi];
	next_pfr = ( curev->ms_timestamp * (fsamp/100) ) / 10;
	cnt = next_pfr - realpfr;
	if	( cnt > qpfr )
		cnt = qpfr;
	// ecouler du temps
	if	( cnt > 0 )
		{	// demander cnt frames au synthe, en stereo entrelacee 16 bits dans 1 seul buffer
		fluid_synth_write_s16( flusyn.synth, cnt, (short *)pcmbuf, 0, 2, (short *)pcmbuf, 1, 2 );
		totalcnt += cnt; realpfr += cnt;
		}
	// envoyer tous les events pour cet instant
	// (on en a deja un dans curev )
	current_mf_time = curev->mf_timestamp;
	while	( current_mf_time == curev->mf_timestamp )
		{
		if	( curev->playable() )
			send_event( curev );	// play_it
		if	( next_evi >= (int)lesong->events_merged.size() )
			return totalcnt;
		curev = lesong->events_merged[next_evi++];
		}
	PAS FINI
	}
*/

/* test minimaliste : une note */
if	( realpfr == 0 )		// test minimaliste
	{
	flusyn.bank_select( 0, 0 );
	flusyn.program_change( 0, 0 );	// zero-based, subtract 1 from https://en.wikipedia.org/wiki/General_MIDI
	flusyn.noteon( 0, 60, 127 );	// C4 sur piano
	}
if	( realpfr < ( fsamp * 10 ) )	// 10 secondes
	{
	cnt = (3 * qpfr) / 4;	// test remplissage partiel du buffer
	// demander cnt frames au synthe, en stereo entrelacee 16 bits dans 1 seul buffer
	fluid_synth_write_s16( flusyn.synth, cnt, (short *)pcmbuf, 0, 2, (short *)pcmbuf, 1, 2 );
	totalcnt += cnt; realpfr += cnt;
	}
else	totalcnt = 0;
//*/

return totalcnt;
};

void midirender::send_event( midi_event * ev )
{}

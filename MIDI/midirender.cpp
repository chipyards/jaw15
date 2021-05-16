
#include <math.h>

using namespace std;
#include <vector>

#include "midirender.h"

// ouverture midifile et copie entiere en RAM dans l'objet lesong
// effectue chargement SF2 selon flusyn.sf2file
// retour 0 si Ok
int midirender::read_head( const char * fnam, int verbose )
{
int retval;
if	( lesong )
	{ delete lesong; lesong = NULL; }
lesong = new song();
retval = lesong->load( fnam );
if	( retval ) return retval;
printf("just read %s\n", fnam );	fflush(stdout);
// lesong->dump( stdout );		fflush(stdout);
lesong->merge();
lesong->apply_tempo();
lesong->apply_tempo_u();	// experimental
lesong->check();			fflush(stdout);
// lesong->dump2( stdout );		fflush(stdout);
estpfr = ( lesong->get_duration_ms() * (fsamp/100) ) / 10 + 1;
realpfr = 0;

retval = flusyn.init( fsamp, verbose );	// le fsamp d'audiofile
if	( retval ) return retval;
retval = flusyn.load_sf2();	// selon flusyn.sf2file
if	( retval ) return retval;

monosamplesize = 2;		// pour le moment seult s16...
qchan = 2;			// toujours stereo
next_evi = 0;			// start play
return 0;
};

// lire un bloc de qpfr frames (max)
// rend le nombre de frames effectivement produites
// 0 si rendu fini
int midirender::read_data_p( void * pcmbuf, unsigned int qpfr )
{
unsigned int cnt, totalcnt;
// y a-t-il quelque chose a jouer ?
if	( ( next_evi < 0 ) || ( next_evi >= (int)lesong->events_merged.size() ) )
	return 0;
unsigned int next_pfr, current_mf_time;
midi_event * curev;
float * shbuf32 = (float *)pcmbuf;
short * shbuf16 = (short *)pcmbuf;
// de l'appel precedent a read_data_p() on herite de next_evi qui pointe sur un event pas encore joue
totalcnt = 0;
curev = lesong->events_merged[next_evi];
// on est ici avec next_evi et curev qui pointent sur un event pas encore joue
while	( totalcnt < qpfr )
	{
	// recuperer le timestamp du prochain event
	next_pfr = ( curev->ms_timestamp * (fsamp/100) ) / 10;
	cnt = next_pfr - realpfr;
	if	( cnt > ( qpfr - totalcnt ) )
		cnt = ( qpfr - totalcnt );	// prochain event est hors de la fenetre
	// ecouler du temps
	if	( cnt > 0 )
		{
		// demander cnt frames au synthe, en stereo entrelacee 16 ou 32 bits dans 1 seul buffer
		if	( monosamplesize == 4 )
			{ fluid_synth_write_float( flusyn.synth, cnt, shbuf32, 0, 2, shbuf32, 1, 2 ); shbuf32 += ( cnt * 2 ); }
		else	{ fluid_synth_write_s16(   flusyn.synth, cnt, shbuf16, 0, 2, shbuf16, 1, 2 ); shbuf16 += ( cnt * 2 ); }
		totalcnt += cnt; realpfr += cnt; 
		// printf("wrote %d to t=%d pfr = %g s\n", cnt, realpfr, double(realpfr) / 44100.0 );  fflush(stdout); 
		}
	if	( totalcnt == qpfr )		// N.B. totalcnt > qpfr est impossible, ok ?
		{
		// printf("returning %d\n", totalcnt );  fflush(stdout); 
		return totalcnt;		// prochain event est hors de la fenetre (sortie normale)
		}
	// envoyer tous les events pour cet instant
	// (on en a deja un dans curev )
	current_mf_time = curev->mf_timestamp;
	while	( current_mf_time == curev->mf_timestamp )
		{
		if	( curev->playable() )
			send_event( curev );	// play_it
		next_evi++;			// N.B. on pase au moins 1 fois ici
		if	( next_evi >= (int)lesong->events_merged.size() )
			return totalcnt;	// sortie finale
		// on recupere deja le prochain pour les besoins du while
		curev = lesong->events_merged[next_evi];
		}
	// on est ici avec next_evi et curev qui pointent sur un event pas encore joue
	}
return totalcnt;

/* test minimaliste : une note *
totalcnt = 0;
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
return totalcnt;
//*/

};

void midirender::send_event( midi_event * ev )
{
// ev->dump( stdout );
// FILE * fil = stdout;
switch( ev->midistatus )
	{
	case 0x80 :  // fprintf( fil, "Off ch=%d n=%d v=%d\n", ev->channel, ev->midinote, ev->vel );
		fluid_synth_noteoff( flusyn.synth, ev->channel, ev->midinote );
		break;
	case 0x90 :  // fprintf( fil, "On ch=%d n=%d v=%d\n", ev->channel, ev->midinote, ev->vel );
		fluid_synth_noteon( flusyn.synth, ev->channel, ev->midinote, ev->vel );
		break;
	case 0xA0 :  // fprintf( fil, "PoPr ch=%d n=%d v=%d\n", ev->channel, ev->midinote, ev->vel );	// poly pressure = note after touch
		fluid_synth_key_pressure( flusyn.synth, ev->channel, ev->midinote, ev->vel );
		break;
	case 0xB0 :  // fprintf( fil, "Par ch=%d c=%d v=%d\n", ev->channel, ev->midinote, ev->vel  );	// controller
		fluid_synth_cc( flusyn.synth, ev->channel, ev->midinote, ev->vel );
		break;
	case 0xC0 :  // fprintf( fil, "PrCh ch=%d p=%d\n", ev->channel, ev->midinote );		// program change
		fluid_synth_program_change( flusyn.synth, ev->channel, ev->midinote );
		break;
	case 0xD0 :  // fprintf( fil, "ChPr ch=%d v=%d\n", ev->channel, ev->midinote );	// channel pressure = channel after touch
		fluid_synth_channel_pressure( flusyn.synth, ev->channel, ev->midinote );
		break;
	case 0xE0 :  // fprintf( fil, "Pb ch=%d v=%d\n", ev->channel, ev->vel );	// pitch bend (vel sur 14 bits => pas de midinote)
		fluid_synth_pitch_bend( flusyn.synth, ev->channel, ev->vel );
		break;
	}
fflush( stdout );
}

// pre-render en float pour evaluer l'amplitude max ( apres read_head() )
double midirender::pre_render()
{
unsigned int qpfr = 2048; 
float tmpbuf[qpfr*qchan];
int saved_monosamplesize = monosamplesize;
monosamplesize = 4;
double amp, maxamp = 0.0;
int retval;

do	{
	retval = read_data_p( (void *)tmpbuf, qpfr );
	for	( int i = 0; i < ( retval * 2 ); ++i )
		{
		amp = fabs( double(tmpbuf[i]) );
		if	( amp > maxamp )
			maxamp = amp;
		}
	} while ( retval > 0 );

printf("pre-rendered PCM frames %u vs %u estimated\n", realpfr, estpfr ); fflush(stdout);

realpfr = 0;	// ne pas oublier d'avouer qu'on a rien rendu !
monosamplesize = saved_monosamplesize;	// remettre comme c'etait
next_evi = 0;				// sans rien oublier

return maxamp;
}

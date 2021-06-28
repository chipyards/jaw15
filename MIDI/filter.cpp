#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

using namespace std;
#include <vector>

#include "midi_event.h"
#include "song.h"
#include "filter.h"

// classe derivee de song pour ajouter des filtres specifiques

// filtre pour post-prod sur un midi fait avec Lilypond
int song_filt::filter_1()
{
unsigned int ie;
midi_event * ev;
int bar, beat;
if	( timesigs.size() < 1 )
	return 1;
printf("----- FILTER 1 : volumes\n");
int chan;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	// B0 = control change, 07 = volume
	if	( ( ev->midistatus == 0xB0 ) && ( ev->midinote == 0x07 ) )
		{
		// alors ev->vel est le volume du channel sur 7 bits
		bar_n_beat( ev, &bar, &beat );	// calcul bar et beat
		printf(" volume @ %6d ticks (%2d:%d) ch %d %2d ",
			ev->mf_timestamp, bar, beat, ev->channel, ev->vel );
		// ev->vel = 75;    // on pourrait le changer... par defaut Lilypond met 100
 		printf(" -> %2d\n", ev->vel );
		}
	}

printf("----- FILTER 1 : accents\n");	// fixer les velocites sur le canal 2 = right piano (en base 1)
chan = 2;
--chan;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	// 90 avec vel>0 pour note ON
	if	( ( ev->midistatus == 0x90 ) && ( ev->channel == chan ) && ( ev->vel > 0 ) )
		{
		bar_n_beat( ev, &bar, &beat );	// calcul bar et beat
		beat = beat % 3;		// beat est zero-based
		if	( beat == 1 )
			ev->vel = 49;
		else if	( beat == 2 )
			ev->vel = 69;
		}
	}
return 0;
}

// ce filtre transpose toutes les notes sauf ch 9 (drums)
int song_filt::filter_transp( int dst )
{
unsigned int ie;
midi_event * ev;
printf("----- FILTER TRANS : %d semitones\n", dst );
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	if	( ( ev->midistatus == 0x90 ) && ( ev->channel != 9 ) )
		ev->midinote += dst;
	}
return 0;
}

// ce filtre change la division et met a jour tous les mf_timestamp
int song_filt::filter_division( int newdiv )
{
unsigned int ie;
midi_event * ev;
printf("----- FILTER DIVISION : %d --> %d\n", division, newdiv );
double k = double(newdiv) / double(division);

for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	ev->mf_timestamp = int( round( (double)ev->mf_timestamp * k ) );
	}
division = newdiv;
pulsation = 1.0 / (1000.0 * (double)division );
return 0;
}

// ce filtre change la division et met a jour le tempo (fresh recording only)
int song_filt::filter_division_fresh( int newdiv )
{
if	(!is_fresh_recorded())
	return -2;
if	( tracks.size() < 1 )
	return 0;
midi_event * ev;
track * tr0 = &tracks[0];
int ie;
// on doit scruter la track0 car elle peut contenir un autres meta-events que tempo
for	( ie = 0; ie < (int)tr0->events_tri.size(); ++ie )
	{
	ev = &tr0->events[ tr0->events_tri[ ie ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		ev->vel = ( 1000 * newdiv );
		ev->mf_timestamp = 0;
		}
	}
division = newdiv;
pulsation = 1.0 / (1000.0 * (double)division );
return 0;
}

// ce filtre cree ou efface la track 0, fixe tempo et time-sig
int song_filt::filter_init_tempo( int tempo, int num, int den )
{
track * tr;
midi_event * ev;
printf("----- FILTER tempo : %d us/beat, %d/%d\n", tempo, num, den );
if	( tracks.size() < 1 )
	add_new_track();
tr = &tracks[0];
tr->events.clear(); tr->events_tri.clear();
// tempo event
tr->events.push_back( midi_event() );
ev = &tr->events.back();
ev->mf_timestamp = 0;
ev->ms_timestamp = 0;
ev->midistatus = 0xFF;
ev->channel = 0;
ev->midinote = 0x51;
ev->vel = tempo;
ev->iext = -1;
tr->events_tri.push_back( tr->events.size() - 1 );
// time sig event
tr->events.push_back( midi_event() );
ev = &tr->events.back();
ev->mf_timestamp = 0;
ev->ms_timestamp = 0;
ev->midistatus = 0xFF;
ev->channel = 0;
ev->midinote = 0x58;
switch	( den )
	{
	case  2 : den = 1; break;
	case  4 : den = 2; break;
	case  8 : den = 3; break;
	case 16 : den = 4; break;
	case 32 : den = 5; break;
	default : den = 2;
	}
ev->vel = ( num << 24 ) | ( den << 16 ) | ( 24 << 8 ) | 8;
ev->iext = -1;
tr->events_tri.push_back( tr->events.size() - 1 );
return 0;
}


// En se basant sur les notes de reference (k_notes) d'une track juste enregistree sans metronome
// (fresh recording), creer un tempo event a chaque k_note, pour accompagner les variations de cadence
// Les k_notes sont supposéees apparaitre tous les bpkn beats (1 beat = 1 midi quarter note)
// Ces notes sont ajustees au fur et a mesure pour que leur temps reel ne change pas et leur music time
// (MBT) tombe sur les beats de reference.
// Puis les autres events de la track sont ajustes pour que leur temps reel ne change pas.
// (cela concerne surtout les note-off et controles divers presents par accident)
// Les autres tracks sont ignorees.
int song_filt::filter_t_follow( int itrack, int bpkn )
{
if	( itrack >= (int)tracks.size() )
	return -1;
if	(!is_fresh_recorded())
	return -2;
printf("----- FILTER t-follow\n");
unsigned int ie; int dt_ms, tempo;
midi_event * ev0 = NULL;	// k_note precedente
midi_event * ev1;		// k-note courante, fin de la portee du dernier tempo event
midi_event * evt = NULL;	// tempo event

// premiere passe : seulement les k_notes, car les notes intermediaires dependront d'un tempo-event
// qui ne sera determine qu'a la fin du beat
for	( ie = 0; ie < tracks[itrack].events_tri.size(); ++ie )
	{
	ev1 = &tracks[itrack].events[tracks[itrack].events_tri[ie]];
//	if	( ( ev1->midinote == k_note ) && ( ev1->midistatus == 0x90 ) && ( ev1->vel > 0 ) )
	if	( ( ev1->midistatus == 0x90 ) && ( ev1->vel > 0 ) )
		{
		if	( ev0 )				// traiter intervalle ev1 - ev0
			{
			dt_ms = ev1->ms_timestamp - ev0->ms_timestamp;	// intervalle en ms
			tempo = ( dt_ms * 1000 ) / bpkn;		// tempo en us/beat
			// creation tempo event dans track 0
			// a l'instant de la precedente k-note ev0
			tracks[0].events.push_back( midi_event() );
			tracks[0].events_tri.push_back( tracks[0].events.size() - 1 );
			evt = &tracks[0].events.back();
			evt->mf_timestamp = ev0->mf_timestamp;		// debut de beat
			evt->ms_timestamp = ev0->ms_timestamp;
			evt->midistatus = 0xFF;
			evt->channel = 0;
			evt->midinote = 0x51;
			evt->vel = tempo;
			evt->iext = -1;
			printf("creation tempo @ %d (%d ms) = %d us\n",
				evt->mf_timestamp, evt->ms_timestamp, evt->vel );
			// maintenant il faut mettre a jour ev1, qui va devenir ev0 a son tour
			ev1->mf_timestamp = ev0->mf_timestamp + ( bpkn * division );	// creation mf_t
			// on prend soin de recalculer ev1->ms_timestamp pour eviter erreur cumulative
			printf("recalcul ms_timestamp %d", ev1->ms_timestamp );
			ev1->ms_timestamp = mft2mst0( ev1->mf_timestamp, evt );
			printf(" --> %d\n", ev1->ms_timestamp );
			}
		else	{				// premiere k_note
			// on va la caler a mf_timestamp = 0, car on considere qu'elle est au debut de la partition
			// (si on lui mettait un mf_timestamp > 0, l'application du seul tempo event disponible qui est celui du
			// fresh recording lui impliquerait un ms_timestamp sans signification reelle.
			// on lui laisse son ms_timestamp car a partir d'elle les ms_timestamp seront toujours interpretes
			// ou recalcules de maniere relative (ils sont tous faux au final, mais pas sauves dans la midifile)
			ev1->mf_timestamp = 0;
			// il va en resulter que le premier tempo event nouveau va overrider le fresh tempo event
			// il vaudrait mieux effacer ce dernier
			}
		ev0 = ev1;
		}
	else	{					// event autre que k-note
		if	( ev0 == NULL )
			ev1->mf_timestamp = 0;		// event anterieur a premiere k_note, on le met a l'instant zero
		else	ev1->mf_timestamp = 0x80000000;	// invalid value ==> traitement ulterieur	
		}
	}

// seconde passe : traiter les events autre que les k_notes, et verifier le timing des k_notes
for	( ie = 0; ie < tracks[itrack].events_tri.size(); ++ie )
	{
	ev1 = &tracks[itrack].events[tracks[itrack].events_tri[ie]];
	if	( ev1->mf_timestamp & 0x80000000 )
		{	// ajuster event autre
		ev1->mf_timestamp = mst2mft( ev1->ms_timestamp );
		}
	else	{	// verifier k-note .. normalement cela devrait rater .. mais en fait mst2mft fait confiance aux
		//	   ms_timestamps des tempo events, il ne les recalcule pas .. donc cela marche quand meme !
		if	( ev1->mf_timestamp != mst2mft( ev1->ms_timestamp ) )
			printf("k-note : warning recalcul mf_t %d vs %d\n", mst2mft( ev1->ms_timestamp ), ev1->mf_timestamp );
		}
	}
// troisieme passe : recalculer tous les ms_timestamps, car ils sont tous decales
apply_tempo();
return 0;
}

// lire les timestamps de reference dans un fichier CSV de Sonic Visualizer (annotation layer)
// le vecteur timestamps doit etre fourni (vide)
// Les timestamps de reference sont supposéees apparaitre tous les bpkn beats (1 beat = 1 midi quarter note)
int song_filt::read_CSV_instants( const char * fnam, vector <double> * timestamps, int bpkn, int FIR )
{
printf("----- FILTER read_CSV_instants (%s)\n", fnam );
FILE * csv_fil = fopen( fnam, "r" );
if	( csv_fil == NULL )
	return -1;
char lbuf[256]; double t;
timestamps->clear();
while	( fgets( lbuf, sizeof( lbuf ), csv_fil ) )
	{
	t = strtod( lbuf, NULL );
	timestamps->push_back( t );
	}
fclose( csv_fil );
// statistiques
int qstamps = timestamps->size();
printf("lu %u timestamps\n", qstamps );
if	( qstamps < 2 )
	return -2;
int i; double dt, dtmin = 100000.0, dtmax = 0.0, dtsum;
for	( i = 1; i < qstamps; ++i )
	{
	dt = timestamps->at(i) - timestamps->at(i-1);
	if	( dt < dtmin ) dtmin = dt;
	if	( dt > dtmax ) dtmax = dt;
	}
dtsum = timestamps->at(qstamps-1) - timestamps->at(0);
dtsum /= (qstamps-1);
printf("[%g,%g] avg %g --> %g BPM\n", dtmin, dtmax, dtsum, double(60 * bpkn) / dtsum );
if	( ( FIR & 1 ) == 0 )
	return 0;
// filtrage FIR optionnel (ordre impair), ce sont les intervalles qui sont filtres
int qint = qstamps-1;
double interval_in[qint];
double interval_out[qint];
double firsum; int j, k, demi;
demi = FIR/2; 	// arrondi par defaut
// calcul des intervalles, ce sont eux qui sont filtres
for	( i = 0; i < qint; ++i )
	interval_in[i] = timestamps->at(i+1) - timestamps->at(i);
// filtrage
for	( i = 0; i < qint; ++i )
	{
	firsum = 0.0;
	for	( j = 0; j < FIR; j++ )
		{
		k = i + j - demi;
		if	( k < 0 ) k = 0;		// hack du bord gauche
		if	( k >= qint ) k = qint - 1;	// hack du bord droit
		firsum += interval_in[k];
		}
	interval_out[i] = firsum / FIR;
	}
// reconstruction des timestamps
double oldlast = timestamps->at(qint);
for	( i = 0; i < qint; ++i )
	timestamps->at(i+1) = timestamps->at(i) + interval_out[i];
// verification de la duree totale (en theorie le filtre respecte la composante DC, mais
// une petite erreur peut etre causee par les hacks des bords) 
double correction = oldlast - timestamps->at(qint);
correction /= qint;
for	( i = 0; i < qint; ++i )
	interval_out[i] += correction;
// reconstruction des timestamps
for	( i = 0; i < qint; ++i )
	timestamps->at(i+1) = timestamps->at(i) + interval_out[i];
// nouvelles statistiques
dtmin = 100000.0; dtmax = 0.0;
for	( i = 1; i < qstamps; ++i )
	{
	dt = timestamps->at(i) - timestamps->at(i-1);
	if	( dt < dtmin ) dtmin = dt;
	if	( dt > dtmax ) dtmax = dt;
	}
dtsum = timestamps->at(qstamps-1) - timestamps->at(0);
dtsum /= (qstamps-1);
printf("apres filtrage FIR ordre %d :\n", FIR );
printf("[%g,%g] avg %g --> %g BPM\n", dtmin, dtmax, dtsum, double(60 * bpkn) / dtsum );
// nouveau CSV pour retour vers SV
char newcsv[512];
snprintf( newcsv, sizeof(newcsv), "%s.fir%d.csv", fnam, FIR );
csv_fil = fopen( newcsv, "w" );
if	( csv_fil )
	{
	for	( i = 0; i < qstamps; ++i )
		fprintf( csv_fil, "%.9f,%d\n", timestamps->at(i), i+1 ); 
	}
fclose( csv_fil );
return 0;
}

// creer un tempo event a chaque timestamp (exprime en secondes), pour accompagner les variations de cadence
// Les timestamps de reference sont supposéees apparaitre tous les bpkn beats (1 beat = 1 midi quarter note)
// le filtre ecrit dans la track 0 de la song, qui est supposee etre fresh, avec une time signature compatible avec bpkn
// 	exemple 1 : time sig 4/4,  bpkn = 4 : un timestamp par mesure
//	exemple 2 : time sig 12/8, bpkn = 6 : un timestamp par mesure
// leadin_flag :
//	0 : le premier timestamp est mis au tick MIDI 0
//	1 : le premier timestamp est mis a son instant reel, par insertion d'un nombre arbitraire de mesures
//	    precedees d'un tempo event ad-hoc 
int song_filt::filter_instants_follow( vector <double> * timestamps, int bpkn, int leadin_flag )
{
if	(!is_fresh_recorded())
	return -2;
printf("----- FILTER instants-follow\n");
// action sur track 0
int dt_ms, tempo;
int ms_t1;	// beat courant en ms, fin de la portee du dernier tempo event
int ms_t0;	// beat precedent en ms
int mf_t0;	// beat precedent en midi ticks
midi_event * evt = NULL;	// tempo event
unsigned int qstamps = timestamps->size();

if	( leadin_flag )
	{
	int leadin_bars;	// nombre de mesures de lead-in
	// calculer la duree moyenne d'une mesure, pour avoir quelque chose de plausible
	double dt = ( timestamps->at(qstamps-1) - timestamps->at(0) ) / (qstamps-1);
	leadin_bars = int(round( timestamps->at(0) / dt ));
	// duree d'une mesure de lead-in
	dt = timestamps->at(0) / leadin_bars;
	// tempo correspondant en us/beat
	tempo = int(round( ( dt * 1000000.0 ) / bpkn ));
	// creation tempo event dans track 0 a l'instant 0
	tracks[0].events.push_back( midi_event() );
	tracks[0].events_tri.push_back( tracks[0].events.size() - 1 );
	evt = &tracks[0].events.back();
	evt->mf_timestamp = 0;		// debut de beat
	evt->ms_timestamp = 0;
	evt->midistatus = 0xFF;
	evt->channel = 0;
	evt->midinote = 0x51;
	evt->vel = tempo;
	evt->iext = -1;
	printf("creation tempo @ %d (%d ms) = %d us/beat\n", evt->mf_timestamp, evt->ms_timestamp, evt->vel );
	printf("inserting %d leading bars\n", leadin_bars );
	mf_t0 = leadin_bars * bpkn * division;
	// logiquement on pourrait faire ms_t0 = (int)floor( 1000.0 * timestamps->at(0) );
	// mais en raison de l'arrondi sur tempo, alors on le recalcule
	ms_t0 = mft2mst0( mf_t0, evt );
	if	( ms_t0 != (int)floor( 1000.0 * timestamps->at(0) ) )
		printf("recalcul ms_timestamp %d --> %d\n", (int)floor( 1000.0 * timestamps->at(0) ), ms_t0 );
	}
else	{
	mf_t0 = 0;
	// N.B. ci-dessous on devrait mettre zero et soustraire timestamps->at(0) a tous les timestamps,
	// mais comme c'est utilise en relatif seulement, voila on triche
	ms_t0 = (int)floor( 1000.0 * timestamps->at(0) );
	}
for	( unsigned int i = 1; i < qstamps; ++i )
	{				// traiter intervalle ms_t1 - ms_t0
	ms_t1 = (int)floor( 1000.0 * timestamps->at(i) );
	dt_ms = ms_t1 - ms_t0;			// intervalle en ms
	tempo = ( dt_ms * 1000 ) / bpkn;	// tempo en us/beat
	// creation tempo event dans track 0
	// a l'instant du precedent timestamp du CSV
	tracks[0].events.push_back( midi_event() );
	tracks[0].events_tri.push_back( tracks[0].events.size() - 1 );
	evt = &tracks[0].events.back();
	evt->mf_timestamp = mf_t0;		// debut de beat
	evt->ms_timestamp = ms_t0;
	evt->midistatus = 0xFF;
	evt->channel = 0;
	evt->midinote = 0x51;
	evt->vel = tempo;
	evt->iext = -1;
	printf("creation tempo @ %d (%d ms) = %d us/beat\n", evt->mf_timestamp, evt->ms_timestamp, evt->vel );
	// maintenant il faut mettre a jour le beat precedent pour le prochain tour
	mf_t0 += ( bpkn * division );
	// logiquement on pourrait faire ms_t0 = ms_t1, mais il pourrait y avoir erreur cumulative
	// en raison de l'arrondi sur tempo, alors on le recalcule
	ms_t0 = mft2mst0( mf_t0, evt );
	if	( ms_t0 != ms_t1 )
		printf("recalcul ms_timestamp %d --> %d\n", ms_t1, ms_t0 );
	}
merge();
printf("duree avant apply_tempo() = %u\n", get_duration_ms() );	// faux dans le cas leadin_flag vue la triche
apply_tempo();
printf("duree apres apply_tempo() = %u\n", get_duration_ms() );
return 0;
}

// ce filtre produit (en stdout) un texte CSV compatible avec Sonic Visualizer
// une note clef k_note choisie par convention est utilisee comme repere temporel,
// la premiere note clef est mise a l'instant shift ms
int song_filt::filter_SV_CSV_gen( int itrack, int k_note, int shift )
{
if	( itrack >= (int)tracks.size() )
	return -1;
printf("----- FILTER SV-CSG gen t0=%d ms\n", shift );
// unsigned int ie, cnt, subbeats=4;
unsigned int ie, cnt, subbeats=1;
midi_event * ev;
cnt = 0;
for	( ie = 0; ie < tracks[itrack].events_tri.size(); ++ie )
	{
	ev = &tracks[itrack].events[tracks[itrack].events_tri[ie]];
//	if	( ( ev->midinote == k_note ) && ( ev->midistatus == 0x90 ) && ( ev->vel > 0 ) )
	if	( ( ev->midinote >= k_note ) && ( ev->midistatus == 0x90 ) && ( ev->vel > 0 ) )
		{
		if	( cnt == 0 )
			shift -= ev->ms_timestamp;
		// printf("%g,%d\n", (double)( shift + ev->ms_timestamp ) * 0.001, cnt );
		// printf("%g,%d.%d\n", (double)( shift + ev->ms_timestamp ) * 0.001, 1 + (cnt/subbeats), 1 + (cnt%subbeats) );
		if	( (cnt%subbeats) == 0 )
			printf("%g,%d\n", (double)( shift + ev->ms_timestamp ) * 0.001, 1 + (cnt/subbeats) );
		++cnt;
		}
	}
return 0;
}

// copier la track 0 (tempo) du song sng2 (divisions doivent etre egales)
int song_filt::filter_copy_tempo( song * sng2 )
{
if	( ( tracks.size() < 2 ) || ( sng2->tracks.size() < 1 ) )
	return -1;
if	( division != sng2->division )
	return -2;
track * tr, *tr2;
midi_event * ev2;
printf("----- FILTER copy tempo\n" );
tr = &tracks[0];
tr2 = &sng2->tracks[0];
// vider tout
tr->events.clear();		// les events, eventuellement dans le desordre
tr->events_tri.clear();		// acces aux events tries
tr->event_exts.clear();		// bytes des sysex et meta-events
// boucle de copie
unsigned int i;
for	( i = 0; i < tr2->events.size(); ++i )
	{
	ev2 = &tr2->events[tr2->events_tri[i]];
	// ne copier que les time sigs et tempos
	if	( ( ev2->midistatus = 0xFF ) &&
		  ( ( ev2->midinote == 0x51 ) || ( ev2->midinote == 0x58 ) )
		)
		{
		ev2->iext = -1;
		tr->events.push_back( *ev2 );
		tr->events_tri.push_back( tr->events.size() - 1 );
		}
	}
printf("copie terminee, %d events (tempo ou timesig)\n", tr->events.size() );
return 0;
}


// convertir une piste de Lucina drums en GM
int song_filt::filter_lucina_drums_to_GM( int itrack )
{
if	( itrack >= (int)tracks.size() )
	return -1;
unsigned int ie, cnt, note;
midi_event * ev;
cnt = 0;
for	( ie = 0; ie < tracks[itrack].events_tri.size(); ++ie )
	{
	ev = &tracks[itrack].events[tracks[itrack].events_tri[ie]];
	if	( ( ev->midistatus == 0x80 ) || ( ev->midistatus == 0x90 ) )
		{
		ev->channel = 9;
		note = ev->midinote;
		if	( note < 60 )		// bass drum
			ev->midinote = 36;
		else if	( note < 72 )		// snare drum
			ev->midinote = 40;
		else if	( note < 79 )		// hi hat
			ev->midinote = 44;
		else				// cymbal
			ev->midinote = 51;
		++cnt;
		}
	}
printf("----- FILTER lucina_drums_to_GM : %d events converted\n", cnt );
return 0;
}



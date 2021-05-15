#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

using namespace std;
#include <vector>

#include "song.h"

extern "C" {
void gasp( const char *fmt, ... );  /* fatal error handling */
}

/** --------------------------------- midifile parsing ----------------------------------- **/

// bas niveau : lecture fichier .mid
int mf_in::mopen( const char * fnam )
{
fil = fopen( fnam, "rb" );
if	( fil == NULL )
	return -1;
totalread = 0;
return 0;
}

// lire un byte
int mf_in::mgetc()
{
int c = fgetc( fil );	// attention mefiance il faut ouvrir le fichier en mode bin
if	( c == EOF )
	gasp("premature end of file, %d to be read, total %d", toberead, totalread );
--toberead; ++totalread;
return c;
}

void mf_in::chkfourcc( const char * fcc )
{
int i;
for	( i = 0; i < 4; ++i )
	{
	if	( mgetc() != *(fcc++) )
		{
		gasp("wrong fcc @ %d (%d)", totalread, totalread );
		}
	}
}

// readvarinum - read a varying-length number
int mf_in::readvarinum()
{
int value;
int c;

c = mgetc(); // fprintf(stderr,"v-%02x", c );
value = c;
if	( c & 0x80 )
	{
	value &= 0x7f;
	do	{
		c = mgetc(); // fprintf(stderr,"v-%02x", c );
		value = (value << 7) + (c & 0x7f);
		} while (c & 0x80);
	}
// fprintf(stderr,"-->%x\n", (unsigned int)value );
return (value);
}

// lire un nombre de n bytes en big-endian
int mf_in::read_big_endian( int n )
{
int val;
val  = ( mgetc() & 0xFF );
val <<= 8;
val |= ( mgetc() & 0xFF );
if	( n == 2 ) return val;
val <<= 8;
val |= ( mgetc() & 0xFF );
if	( n == 3 ) return val;
val <<= 8;
val |= ( mgetc() & 0xFF );
return val;
}

// attention la track doit etre vierge
void track::load( mf_in * mst, song * chanson )
{
mst->toberead = 4;
mst->chkfourcc( "MTrk");
mst->toberead = mst->read_big_endian( 4 );
// printf("MTrk %d\n", mst->toberead );
parent = chanson;

int t = 0;
int prev_status = 0;	// status memorise pour le prochain event au cas ou il serait en running status
int c0, c1, c2;		// 3 premiers bytes de l'event
midi_event *ev;		// evenement courant

while	( mst->toberead > 0 )	// event loop
	{
	t += mst->readvarinum();	// delta time, every event has one
	c0 = mst->mgetc();
	// regler la question du running status
	if	( c0 & 0x80 )
		{					// normal status
		prev_status = c0;
		c1 = mst->mgetc();
		}
	else	{					// running status
		if	( prev_status == 0 )
			gasp("unexpected running status");
		c1 = c0;
		c0 = prev_status;
		}
	// a partir d' ici c0 a le MSB set et c1 valide
	if	( c0 < 0xF0 )
		{	// traiter event normal (aka channel message)
		if	( ( c0 >= 0xC0 ) && ( c0 <= 0xDF ) )
			c2 = 0;			// 2-byte events ( Cx prog change et Dx channel pressure )
		else	c2 = mst->mgetc();	// 3-byte events
		if	( c1 & 0x80 )
			gasp( "MSB set on c1 data : %02x", c1 );
		if	( c2 & 0x80 )
			gasp( "MSB set on c1 data : %02x", c2 );
		events.push_back( wevent() );
		ev = &events.back();
		ev->mf_timestamp = t;
		ev->midistatus = c0 & 0xF0;
		ev->channel = c0 & 0x0F;
		ev->midinote = c1;
		if	( ev->midistatus == 0xE0 )
			{		// cas particulier pitch bend 14 bits little endian (!)
			ev->vel = c1 | ( c2 << 7 );
			}
		else	ev->vel = c2;
		ev->iext = -1;
		}
	else if	( c0 == 0xFF )
		{	// meta event
		int meta_size = mst->readvarinum();
		// printf("meta type %d, taille %d\n", c1, meta_size );
		events.push_back( wevent() );
		ev = &events.back();
		ev->mf_timestamp = t;
		ev->midistatus = c0;
		ev->channel = 0;
		ev->midinote = c1;	// ici c'est le type de meta
		ev->iext = -1;
		switch	( c1 )
			{			// traiter les evenements courts (<= 4 bytes)
			case 0 :	// sequence number
				if	( meta_size != 2 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->read_big_endian( meta_size );
				break;
			case 0x20 :	// midi channel prefix
				if	( meta_size != 1 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->mgetc();
				break;
			case 0x21 :	// midi port
				if	( meta_size != 1 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->mgetc();
				break;
			case 0x2f :	// End of Track
				if	( meta_size != 0 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				break;
			case 0x51 :	// Set tempo
				if	( meta_size != 3 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->read_big_endian( meta_size );
				break;
			case 0x58 : {	// time signature
				if	( meta_size != 4 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->read_big_endian( meta_size );
				char * bytes = ((char *)(&ev->vel));
				parent->timesigs.push_back( timesig( t, parent->division, bytes[3], 1<<bytes[2], bytes[0] ) );
				} break;
			case 0x59 :	// key signature
				if	( meta_size != 2 ) gasp("wrong meta event size %d for type %d", meta_size, c1 );
				ev->vel = mst->read_big_endian( meta_size );
				break;
			default :	// stocker en ext les evenements de longueur non limitee ou inconnus (meme de longueur nulle !)
				event_exts.push_back( midi_event_ext() );
				midi_event_ext * evx = &event_exts.back();
				for	( int i = 0; i < meta_size; ++i )
					evx->bytes.push_back( (unsigned char)mst->mgetc() );
				ev->iext = event_exts.size() - 1;
			}
		}
	else	{	// sysex
		if	( ( c0 == 0xF0 ) || ( c0 == 0xF7 ) )
			{	// sysex normal
			printf("sysex %02X\n", c0 );
			int sysex_size;		// ici on doit lire la taille, mais on en a deja un byte dans c1
			sysex_size = c1;	// on est oblige de dupliquer un bout de code de readvarinum()
			if	( c1 & 0x80 )
				{
				sysex_size &= 0x7f;
				do	{
					c1 = mst->mgetc(); // fprintf(stderr,"v-%02x", c );
					sysex_size = (sysex_size << 7) + (c1 & 0x7f);
					} while (c1 & 0x80);
				}
			printf("size %d\n", sysex_size );
			for	( int i = 0; i < sysex_size; ++i )
				mst->mgetc();
			}
		else	gasp( "illegal status %02x", c0 );
		}
	}	// event loop finie
unsigned int i;
for	( i = 0; i < events.size(); ++i )
	events_tri.push_back(i);
}


int song::load( const char * fnam )
{
mf_in mst;
int retval;

retval = mst.mopen( fnam );
if	( retval )
	return retval;

mst.toberead = 4;
mst.chkfourcc( "MThd");

mst.toberead	= mst.read_big_endian(4);
format		= mst.read_big_endian(2);
int ntrks	= mst.read_big_endian(2);
division	= mst.read_big_endian(2);
set_cadence( 1.0 );

// flush any extra stuff, in case the length of header is not 6
while	( mst.toberead > 0 )
	mst.mgetc();

// read the tracks
track * tr;
for	( int i = 0; i < ntrks; ++i )
	{
	tracks.push_back( track() );
	tr = &tracks.back();
	tr->load( &mst, this );
	}
// timesig par defaut
if	( timesigs.size() == 0 )
	timesigs.push_back( timesig( 0, division ) );
return 0;
}

// ajouter track vierge sur song existant (pour enregistrer)
// rend l'index de la track
int song::add_new_track()
{
events_merged.clear();		// pointeur invalides
tracks.push_back( track() );
tracks.back().parent = this;
return( tracks.size() - 1 );
}


/** ============================== data processing methods ========================== **/

// fusionner les tracks en preservant l'ordre chronologique (selon les mf_timestamp)
// pas de filtre, on prend tout
// mettre a jour events_merged
int song::merge()
{
events_merged.clear();
// initialisation de notre algo
vector <int> next_ie;
unsigned int it;
for	( it = 0; it < tracks.size(); ++it )
	if	( tracks[it].events_tri.size() )
		next_ie.push_back( 0 );
	else	next_ie.push_back( -1 );
// scrutation
int it_sel;	// index de la track selectionnee pour fournir le prochain event
unsigned int mintime;	// timestamp du plus proche prochain event
midi_event * e, * e_sel;
while	(1)
	{
	// chercher dans quelle track se touve le prochain event
	// en cas d'ex-aequo, la track d'indice plus faible est prioritaire
	it_sel = -1; mintime = 0x7FFFFFFF;
	for	( it = 0; it < tracks.size(); ++it )
		{
		if	( next_ie[it] >= 0 )
			{
			e = &tracks[it].events[tracks[it].events_tri[next_ie[it]]];
			if	( e->mf_timestamp < 0 )
				return -1;
			if	( e->mf_timestamp < mintime )
				{ it_sel = it; e_sel = e; mintime = e->mf_timestamp; }
			}
		}
	// detecter la fin du song ( next_ie[] rempli de -1 )
	if	( it_sel < 0 )
		break;
	// stocker l'event selectionne
	events_merged.push_back( e_sel );
	// avancer dans la track selectionnee
	next_ie[it_sel] += 1;
	if	( next_ie[it_sel] >= (int)tracks[it_sel].events_tri.size() )
		next_ie[it_sel] = -1;	// fin de la track
	}
return 0;
}

void song::set_cadence( double k )
{
pulsation = k / (1000.0 * (double)division );
}

// calculer ms_timestamp (millisecond timestamp) en fonction de mf_timestamp pour chaque event
// ATTENTION : algo base sur events_merged
// en multipliant par k * 0.001 * tempo / division
// 	division est constant pour le song (lu dans le header de midi file) = PPQN Pulse Per Quarter Note
//	tempo peut etre est modifie a tout moment par un meta-event. c'est en us/quarter_note
//	le facteur 0.001 est pour obtenir des ms alors tempo est en us/QN
//	k est le facteur de vitesse d'execution
//	pulsation est k * 0.001 / division pre-calcule
// ici calcul tout en double
void song::apply_tempo()
{
unsigned int ie;
midi_event * ev;
if	( is_fresh_recorded() )		// dans ce cas pulsation est ignore, k est suppose 1.0
	{
	printf("apply fresh tempo\n");
	for	( ie = 0; ie < events_merged.size(); ++ie )
		{
		ev = events_merged[ie];
		ev->ms_timestamp = ev->mf_timestamp;
		}
	return;
	}
// k /= ( 1000.0 * (double)division );
double tempo = 500000.0;
double mul = pulsation * tempo;
int    ref_mf_t = 0;	// date en mf ticks du dernier tempo event
double ref_ms_t = 0.0;	// date en ms du dernier tempo event
double ms_t;
printf("apply guided tempo\n");
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	ms_t = double( ev->mf_timestamp - ref_mf_t );
	ms_t *= mul;
	ms_t += ref_ms_t;
	ev->ms_timestamp = (int)ms_t;
	// tempo meta-vent ?
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		tempo = (double)ev->vel;
		mul = pulsation * tempo;
		ref_mf_t = ev->mf_timestamp;
		// ref_ms_t = ms_t; // c'etait plus exact mais moins reproductible
		ref_ms_t = (double)ev->ms_timestamp;
		}
	}
}

// calculer us_timestamp (microsecond timestamp) en fonction de mf_timestamp pour chaque event
// ATTENTION : algo base sur events_merged
// en multipliant par k * tempo / division
// 	division est constant pour le song (lu dans le header de midi file) = PPQN Pulse Per Quarter Note
//	tempo peut etre est modifie a tout moment par un meta-event. c'est en us/quarter_note
//	k est le facteur de vitesse d'execution
//	pulsation est k / division, pre-calcule
// ici calcul tout en double
void song::apply_tempo_u()
{
unsigned int ie;
midi_event * ev;
if	( is_fresh_recorded() )		// dans ce cas pulsation est ignore, k est suppose 1.0
	{
	printf("apply fresh tempo\n");
	for	( ie = 0; ie < events_merged.size(); ++ie )
		{
		ev = events_merged[ie];
		ev->us_timestamp = 1000 * ev->mf_timestamp;
		}
	return;
	}
double tempo = 500000.0;
double mul = 1000.0 * pulsation * tempo;	// le 1000 est provizoar
int    ref_mf_t = 0;	// date en mf ticks du dernier tempo event
double ref_us_t = 0.0;	// date en us du dernier tempo event
double us_t;
printf("apply guided tempo\n");
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	us_t = double( ev->mf_timestamp - ref_mf_t );
	us_t *= mul;
	us_t += ref_us_t;
	ev->us_timestamp = (int)us_t;
	// tempo meta-vent ?
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		tempo = (double)ev->vel;
		mul = 1000.0 * pulsation * tempo;
		ref_mf_t = ev->mf_timestamp;
		// ref_ms_t = ms_t; // c'etait plus exact mais moins reproductible
		ref_us_t = (double)ev->us_timestamp;
		}
	}
}

// fresh_recorded track est un enregistrement dont on souhaite sauver les timestamps en ms
// ce n'est pas prevu par MIDI file, mais si tempo == ( 1000 * division ) alors t_ms = t_mf
// la condition pour etre fresh est :
//	- au moins 1 tempo event a l'instant zero t.q. tempo == ( 1000 * division )
//	- aucun tempo event avec une autre valeur de tempo
int song::is_fresh_recorded()
{
if	( tracks.size() < 1 )
	return 0;
midi_event * ev;
track * tr0 = &tracks[0];
int ie, cnt=0; int tempo;
// on doit scruter la track0 car elle peut contenir d'autres meta-events que tempo
for	( ie = 0; ie < (int)tr0->events_tri.size(); ++ie )
	{
	ev = &tr0->events[ tr0->events_tri[ ie ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		tempo = ev->vel;
		if	( tempo != ( 1000 * division ) )
			return 0;
		if	( ( ev->mf_timestamp != 0 ) && ( cnt = 0 ) )
			return 0;
		++cnt;
		}
	}
return cnt;
}

// traduction ponctuelle de ms (milliseconds) en mf_timestamp (midi ticks)
// basee sur la track 0 qui doit contenir au moins un tempo event a l'instant 0

// traduction ponctuelle de mf_timestamp (midi ticks) en ms (milliseconds)
// basee sur la track 0 qui doit contenir au moins un tempo event a l'instant 0
int song::mft2mst( unsigned int mf_timestamp )
{
if	( tracks.size() < 1 )
	return -2;
// premiere etape : savoir dans quel segment de la fonction PWL on est.
// le segment sera designe par l'index du tempo event dans la track 0
int iseg, iseg_old;
midi_event * ev;
track * tr0 = &tracks[0];
iseg = 0; iseg_old = -1;
// securite : on n'aura iseg_old >= 0 que si il existe au moins un tempo event
// anterieur a mf_timestamp
while	( iseg < (int)tr0->events_tri.size() )
	{
	ev = &tr0->events[ tr0->events_tri[ iseg ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		if	( mf_timestamp >= ev->mf_timestamp )
			iseg_old = iseg;
		else	break;
		}
	++iseg;
	}
if	( iseg_old < 0 )
	return -1;
// deuxieme etape : effectuer le calcul (ici maniere lourde)
ev = &tr0->events[ tr0->events_tri[ iseg_old ] ];
return( mft2mst0( mf_timestamp, ev ) );
// double ms_t = pulsation * (double)ev->vel * double( mf_timestamp - ev->mf_timestamp );
// return( ev->ms_timestamp + (int)ms_t );
}

int song::mft2ust( unsigned int mf_timestamp )
{
if	( tracks.size() < 1 )
	return -2;
// premiere etape : savoir dans quel segment de la fonction PWL on est.
// le segment sera designe par l'index du tempo event dans la track 0
int iseg, iseg_old;
midi_event * ev;
track * tr0 = &tracks[0];
iseg = 0; iseg_old = -1;
// securite : on n'aura iseg_old >= 0 que si il existe au moins un tempo event
// anterieur a mf_timestamp
while	( iseg < (int)tr0->events_tri.size() )
	{
	ev = &tr0->events[ tr0->events_tri[ iseg ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		if	( mf_timestamp >= ev->mf_timestamp )
			iseg_old = iseg;
		else	break;
		}
	++iseg;
	}
if	( iseg_old < 0 )
	return -1;
// deuxieme etape : effectuer le calcul (ici maniere lourde)
ev = &tr0->events[ tr0->events_tri[ iseg_old ] ];
return( mft2ust0( mf_timestamp, ev ) );
// double ms_t = pulsation * (double)ev->vel * double( mf_timestamp - ev->mf_timestamp );
// return( ev->ms_timestamp + (int)ms_t );
}

int song::mst2mft( int ms_timestamp )
{
if	( tracks.size() < 1 )
	return -2;
// premiere etape : savoir dans quel segment de la fonction PWL on est.
// le segment sera designe par l'index du tempo event dans la track 0
int iseg, iseg_old;
midi_event * ev;
track * tr0 = &tracks[0];
iseg = 0; iseg_old = -1;
// securite : on n'aura iseg_old >= 0 que si il existe au moins un tempo event
// anterieur a ms_timestamp
while	( iseg < (int)tr0->events_tri.size() )
	{
	ev = &tr0->events[ tr0->events_tri[ iseg ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		if	( ms_timestamp >= ev->ms_timestamp )
			iseg_old = iseg;
		else	break;
		}
	++iseg;
	}
if	( iseg_old < 0 )
	return -1;
// deuxieme etape : effectuer le calcul (ici maniere lourde)
ev = &tr0->events[ tr0->events_tri[ iseg_old ] ];
return( mst2mft0( ms_timestamp, ev ) );
// double mf_t = double( ms_timestamp - ev->ms_timestamp ) / (pulsation * (double)ev->vel);
// return( ev->mf_timestamp + (int)ceil(mf_t) );
}

int song::ust2mft( int us_timestamp )
{
if	( tracks.size() < 1 )
	return -2;
// premiere etape : savoir dans quel segment de la fonction PWL on est.
// le segment sera designe par l'index du tempo event dans la track 0
int iseg, iseg_old;
midi_event * ev;
track * tr0 = &tracks[0];
iseg = 0; iseg_old = -1;
// securite : on n'aura iseg_old >= 0 que si il existe au moins un tempo event
// anterieur a us_timestamp
while	( iseg < (int)tr0->events_tri.size() )
	{
	ev = &tr0->events[ tr0->events_tri[ iseg ] ];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		if	( us_timestamp >= ev->us_timestamp )
			iseg_old = iseg;
		else	break;
		}
	++iseg;
	}
if	( iseg_old < 0 )
	return -1;
// deuxieme etape : effectuer le calcul (ici maniere lourde)
ev = &tr0->events[ tr0->events_tri[ iseg_old ] ];
return( ust2mft0( us_timestamp, ev ) );
// double mf_t = double( ms_timestamp - ev->ms_timestamp ) / (pulsation * (double)ev->vel);
// return( ev->mf_timestamp + (int)ceil(mf_t) );
}

// post processing apres enregistrement
void track::post_rec()
{
int ie;
midi_event * ev;
events_tri.clear();
printf("post_rec _tri %d events\n", events.size() );
for	( ie = 0; ie < (int)events.size(); ++ie )
	events_tri.push_back(ie);
if	( parent->is_fresh_recorded() )
	{
	printf("post_rec fresh timing %d events\n", events_tri.size() );
	for	( ie = 0; ie < (int)events_tri.size(); ++ie )
		{
		ev = &events[events_tri[ie]];
		ev->mf_timestamp = ev->ms_timestamp;
		}
	}
else	{
	printf("post_rec guided timing %d events\n", events_tri.size() );
	for	( ie = 0; ie < (int)events_tri.size(); ++ie )
		{
		ev = &events[events_tri[ie]];
		ev->mf_timestamp = parent->mst2mft( ev->ms_timestamp );
		}
	}
printf("end post_rec\n");
}

void song::check()
{
unsigned int it, s0, s, ie;
midi_event * ev;

printf( "----- CHECK : format %d, %d tracks, division=%d\n", format, tracks.size(), division );

// comptage des events
int tot = 0;
for	( it = 0; it < tracks.size(); ++it )
	{
	s0 = tracks[it].events.size();
	s  = tracks[it].events_tri.size();
	if	( s0 == s )
		printf("track %d : %d events\n", it, s  );
	else	printf("ERREUR track %d : %d events # %d events tries\n", it, s0, s  );
	tot += s;
	}
if	( (int)events_merged.size() == tot )
	printf("total merged ok %d events\n", tot );
else	printf("ERREUR merged %d vs %d events\n", events_merged.size(), tot );

// verif chronologie mf per track
for	( it = 0; it < tracks.size(); ++it )
	{
	unsigned int mf_t = 0;
	for	( ie = 0; ie < tracks[it].events_tri.size(); ++ie )
		{
		ev = &tracks[it].events[ie];
		if	( ev->mf_timestamp < mf_t )
			printf("err chronologie mf event %d.%d : %d < %d\n", it, ie, ev->mf_timestamp, mf_t );
		mf_t = ev->mf_timestamp;
		}
	printf("chronogie mf timestamp verifiee, trk %d, fin @ %d ticks\n", it, mf_t );
	}

// verif chronologie mf globale
unsigned int mf_t = 0;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	if	( events_merged[ie]->mf_timestamp < mf_t )
		printf("err chronologie mf event %d : %d < %d\n", ie, events_merged[ie]->mf_timestamp, mf_t );
	mf_t = events_merged[ie]->mf_timestamp;
	}
printf("chronogie mf timestamp verifiee (apres merge) fin @ %d ticks\n", mf_t );

// verif chronologie ms globale
int ms_t = -1;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	if	( events_merged[ie]->ms_timestamp < ms_t )
		printf("err chronologie ms event %d : %d < %d\n", ie, events_merged[ie]->ms_timestamp, ms_t );
	ms_t = events_merged[ie]->ms_timestamp;
	}
printf("chronogie ms timestamp verifiee, fin @ %d ms\n", ms_t);
// verif chronologie us globale
int us_t = -1;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	if	( events_merged[ie]->us_timestamp < us_t )
		printf("err chronologie us event %d : %d < %d\n", ie, events_merged[ie]->us_timestamp, us_t );
	us_t = events_merged[ie]->us_timestamp;
	}
printf("chronogie us timestamp verifiee, fin @ %d us\n", us_t);
// listing patch changes
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	if	( ev->midistatus == 0xC0 )
		printf("  program change @ %8d ticks (%8d ms) : ch %d, prog %d\n", ev->mf_timestamp, ev->ms_timestamp,
			ev->channel, ev->midinote );
	}
// listing tempo events
tot = 0;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
		{
		printf("  tempo event    @ %8d ticks (%8d ms) : tempo = %d\n",
			ev->mf_timestamp, ev->ms_timestamp, ev->vel );
		printf("  tempo event    @ %8d ticks (%9d us) : tempo = %d\n",
			ev->mf_timestamp, ev->us_timestamp, ev->vel );
		++tot;
		}
	}
// verification tempo events tous dans track 0
int tot2 = 0;
if	( tracks.size() )
	{
	for	( ie = 0; ie < tracks[0].events_tri.size(); ++ie )
		{
		ev = &tracks[0].events[ie];
		if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x51 ) )
			++tot2;
		}
	}
if	( tot == tot2 )
	printf("vu %d tempo events tous dans track 0\n", tot );
else	printf("ATTENTION %d tempo events dont %d dans track 0\n", tot, tot2 );
// listing time sig events
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	if	( ( ev->midistatus == 0xFF ) && ( ev->midinote == 0x58 ) )
		{
		char * bytes = ((char *)(&ev->vel));
		printf("  time sig event @ %8d ticks (%8d ms) : %d/%d, metr=%d, %d 32nd/Midi_Q\n",
			ev->mf_timestamp, ev->ms_timestamp, bytes[3], 1<<bytes[2], bytes[1], bytes[0] );
		}
	}
// listing time sig compiled events
for	( ie = 0; ie < timesigs.size(); ++ie )
	{
	printf("  time sig       @ %8d ticks : %d beats/bar, %d ticks each\n",
			timesigs[ie].mf_timestamp, timesigs[ie].bpbar, timesigs[ie].mf_tpb );
	}
// re-calcul timing pour validation de la fonction mft2ust()
int dev, devmin, devmax;
// printf("recalcul apply_tempo begin\n");
tot = 0; devmin = 0x7FFFFFFF; devmax = -devmin;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	us_t = mft2ust( ev->mf_timestamp );
	dev = us_t - ev->us_timestamp;
	if	( dev != 0 )
		{
		// printf("err recalcul us_t : event %d : %d vs %d\n", ie, us_t, ev->us_timestamp );
		++tot;
		}
	if	( dev > devmax ) devmax = dev;
	if	( dev < devmin ) devmin = dev;
	}
printf("recalcul apply_tempo done %d <= dev <= %d, %d errs\n", devmin, devmax, tot );
// re-calcul timing pour validation de la fonction mft2ust()
// printf("recalcul deply tempo begin\n");
tot = 0; devmin = 0x7FFFFFFF; devmax = -devmin;
for	( ie = 0; ie < events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	mf_t = ust2mft( ev->us_timestamp );
	dev = mf_t - ev->mf_timestamp;
	if	( dev != 0 )
		{
		// printf("err recalcul mf_t : event %d : %d vs %d\n", ie, mf_t, ev->mf_timestamp );
		++tot;
		}
	if	( dev > devmax ) devmax = dev;
	if	( dev < devmin ) devmin = dev;
	}
printf("recalcul deply_tempo done %d <= dev <= %d, %d errs\n", devmin, devmax, tot );
}

// recherche d'un event pour un instant donné (dichotomie)
// rend un index dans events_merged
// le plus grand index tel que l'event soit strictement anterieur a ms_time
int song::find( int ms_time )
{
unsigned int i, mask;
mask = 0x40000000;
i = 0;
while	( mask )
	{
	i |= mask;
	if	( i >= events_merged.size() )
		i &= (~mask);
	else if	( events_merged[i]->ms_timestamp >= ms_time )
		i &= (~mask);
	mask >>= 1;
	}
return i;
}

// calcul bar et beat pour un event (zero-based)
// pour le moment ne tient compte que de la premiere time-signature
void song::bar_n_beat( midi_event * ev, int * bar, int * beat )
{
if	( timesigs.size() < 1 )
	{ * beat = * bar = 0; return; }
* beat  = ev->mf_timestamp / timesigs[0].mf_tpb;
* bar   = (* beat) / timesigs[0].bpbar;
* beat -= (* bar) * timesigs[0].bpbar;
}
// avec beat fractionnaire
void song::bar_n_beat( midi_event * ev, int * bar, double * beat )
{
if	( timesigs.size() < 1 )
	{ * beat = 0.0; * bar = 0; return; }
int ibeat  = ev->mf_timestamp / timesigs[0].mf_tpb;
* bar  = ibeat / timesigs[0].bpbar;
int mf_t = ev->mf_timestamp - ( timesigs[0].mf_tpb * (* bar) * timesigs[0].bpbar );
* beat = (double)mf_t / (double)timesigs[0].mf_tpb;
}

/** ============================== dump methods, dans le style MF2T ========================== **/

void track::dump( FILE * fil )
{
unsigned int ie;
int iet, iext;
fprintf( fil, "MTrk\n");
for	( ie = 0; ie < events_tri.size(); ++ie )
	{
	iet = events_tri[ie];
	iext = events[iet].dump( fil );
	if	( iext >= 0 )
		event_exts[iext].dump( fil, events[iet].midinote );
	}
fprintf( fil, "TrkEnd\n");
}

void song::dump( FILE * fil )
{
// header
if	( division & 0x8000 )	// SMPTE
	fprintf( fil, "MFile %d %d %d %d\n", format, tracks.size(), -((-(division>>8))&0xff), division&0xff );
else    fprintf( fil, "MFile %d %d %d\n", format, tracks.size(), division );
// tracks
unsigned int itrack;
for	( itrack = 0; itrack < tracks.size(); ++itrack )
	{
	tracks[itrack].dump( fil );
	}
}

/** ============================== dump methods, JLN debug ============================== **/

// ce dump affiche les events dans l'ordre chronologique, tracks merged, avec timestamps reels en ms
// ==> pas d'info sur la track... :-(
void song::dump2( FILE * fil )
{
// header
if	( division & 0x8000 )	// SMPTE
	fprintf( fil, "MFile %d %d %d %d\n", format, tracks.size(), -((-(division>>8))&0xff), division&0xff );
else    fprintf( fil, "MFile %d %d %d\n", format, tracks.size(), division );
// merged tracks
merge();
apply_tempo();
int ie, mn, s, ms, bar; double beat; midi_event * ev;
for	( ie = 0; ie < (int)events_merged.size(); ++ie )
	{
	ev = events_merged[ie];
	// le temps reel
	ms = ev->ms_timestamp;
	s = ms / 1000; ms -= ( s * 1000 );
	mn = s / 60;    s -= ( mn * 60 );
	// fprintf( fil, "%7dms (%02d:%02d.%03d) ", ev->ms_timestamp, mn, s, ms );
	fprintf( fil, "%6dms {%8d} (%02d:%02d.%03d) ", ev->ms_timestamp, ev->us_timestamp, mn, s, ms );
	// bars & beats
	bar_n_beat( ev, &bar, &beat );
	fprintf( fil, "%4d|%02.2f ", bar+1, beat+1.0 );
	// on termine la ligne par un dump style MF2T (mais on ne traite pas extensions de meta-events)
if      ( ev->dump( fil ) >= 0 )
	fprintf( fil, "\n" );   // s'il y a extension on doit termine la ligne nous meme
	}
}

/** ============================== midifile write methods =============================== **/

// write a varying-length number
void mf_out::writevarinum( unsigned int v )
{
unsigned char bytes[4]; int i;
// couper en tranches de 7 bits en commençant par les poids faibles
i = 0;
do	{
	bytes[i++] = v & 0x7F;
	v >>= 7;
	} while ( ( v ) && ( i < 4 ) );
// emettre en big endian
while	( --i >= 0 )
	{
	if	( i )
		mputc( bytes[i] | 0x80 );	// MSB pour tous sauf le dernier
	else	mputc( bytes[i] );
	}
}

// ecrire le temps relatif et mettre a jour le temps absolu
void mf_out::write_time( unsigned int v )
{
if	( v < current_time )
	gasp("write time error %d, previous was %d", v, current_time );
writevarinum( v - current_time );
current_time = v;
}

// ecrire un nombre de n bytes en big-endian
void mf_out::write_big_endian( int v, int n )
{
while	( n )
	mputc( v >> ( --n * 8 ) );
}

// mettre a jour la taille du chunk a l'emplacement prevu, apres le fourcc
// 4 bytes, big-endian
void mf_out::write_chunk_size()
{
unsigned int v = data.size() - cur_chunk_pos - 4;
data[cur_chunk_pos++] = (unsigned char)(v >> 24);
data[cur_chunk_pos++] = (unsigned char)(v >> 16);
data[cur_chunk_pos++] = (unsigned char)(v >> 8);
data[cur_chunk_pos]   = (unsigned char)(v);
}

// ecrire un fourcc
void mf_out::writefourcc( const char * fcc )
{
mputc( fcc[0] ); mputc( fcc[1] );
mputc( fcc[2] ); mputc( fcc[3] );
cur_chunk_pos = data.size();		// memoriser position debut chunk
}

// sauver sur disk
void mf_out::commit( const char * fnam )
{
FILE * fil;
fil = fopen( fnam, "wb" );
if	( fil == NULL )
	gasp("file %s not opened", fnam );
for	( unsigned int i = 0; i < data.size(); ++i )
	fputc( data[i], fil );
fclose( fil );
}

// ecrire un event (N.B. le MSB est forcé à la bonne valeur, sans détection d'erreur concommitante)
// pas de running status chez nous
int wevent::write( mf_out * mf )
{
mf->write_time( mf_timestamp );
if	( midistatus < 0xC0 )		// message de base ( 3 bytes )
	{
	mf->mputc( 0x80 | midistatus | channel );
	mf->mputc( midinote & 0x7F );
	mf->mputc( vel & 0x7F );
	}
else if	( midistatus < 0xE0 )		// message court ( 2 bytes )
	{
	mf->mputc( 0x80 | midistatus | channel );
	mf->mputc( midinote & 0x7F );
	}
else if	( midistatus < 0xF0 )		// pitch bend ( 3 bytes, 14 bits little endian )
	{
	mf->mputc( 0x80 | midistatus | channel );
	mf->mputc( vel & 0x7F );
	mf->mputc( ( vel >> 7 ) & 0x7F );
	}
else if ( midistatus == 0xFF )		// meta_event
	{
	mf->mputc( 0x80 | midistatus );
	mf->mputc( midinote & 0x7F );	// le type
	if	( iext < 0 )		// meta-event court
		{
		int meta_size;
		switch	( midinote )
			{
			case 0 :	// sequence number
			case 0x59 :	// key signature
				meta_size = 2;
				mf->mputc( meta_size );
				mf->write_big_endian( vel, meta_size );
				break;
			case 0x20 :	// midi channel prefix
			case 0x21 :	// midi port
				meta_size = 1;
				mf->mputc( meta_size );
				mf->mputc( vel );
				break;
			case 0x2f :	// End of Track
				meta_size = 0;
				mf->mputc( meta_size );
				break;
			case 0x51 :	// Set tempo
				meta_size = 3;
				mf->mputc( meta_size );
				mf->write_big_endian( vel, meta_size );
				break;
			case 0x58 :	// time signature
				meta_size = 4;
				mf->mputc( meta_size );
				mf->write_big_endian( vel, meta_size );
				break;
			}
		}
	// meta-events longs sont traites par appel à wevent_ext::write
	}
else	{				// sysex not supported
	}
return iext;
}

void wevent_ext::write(mf_out * mf )
{
unsigned int meta_size = bytes.size();
mf->writevarinum( meta_size );
for	( unsigned int i = 0; i < meta_size; ++i )
	mf->mputc( bytes[i] );
}

int song::save( const char * fnam )
{
mf_out mf;

mf.writefourcc("MThd");

mf.write_big_endian( 6, 4 );

mf.write_big_endian( format, 2 );
mf.write_big_endian( tracks.size(), 2 );
mf.write_big_endian( division, 2 );

// tracks
unsigned int itrack, ie;
track * tr;
int iext, iet;
for	( itrack = 0; itrack < tracks.size(); ++itrack )
	{
	mf.writefourcc("MTrk");
	mf.write_big_endian( 0, 4 );	// reserver place pour la taille
	mf.current_time = 0;
	tr = &tracks[itrack];
	for	( ie = 0; ie < tr->events_tri.size(); ++ie )
		{
		iet = tr->events_tri[ie];
		iext = ((wevent *)&tr->events[iet])->write( &mf );
		if	( iext >= 0 )
			((wevent_ext *)&tr->event_exts[iext])->write( &mf );
		}
	mf.write_chunk_size();
	}
mf.commit( fnam );
return 0;
}

#ifndef SONG_H
#define SONG_H

#include "midi_event.h"

/*
INTRO

- la classe song represente en memoire un morceau de musique, tel qu'on peut lire ou ecrire dans une midi-file
  l'essentiel de l'info est dans une collection de tracks.

- la classe track contient essentiellement des events

VECTEURS D'EVENTS

- chaque track represente ses events au moyen de 2 vecteurs:
	events[] : les events possiblement pas dans l'ordre
	events_tri[] : les indices des events, garantis dans l'ordre
  N.B. pour le moment la methode de tri n'existe pas - les events sont toujours crees dans l'ordre
  mais deja la methode song::check() verifie l'ordre dans events_tri[] puis dans events_merged[]

- la classe song contient un vecteur d'events REDONDANT events_merged[] construit par la methode song::merge()
  (interet controverse, mais en attendant cela marche.)
  La methode merge() s'appuie sur les tableaux events_tri des tracks.

MIDI FILES

- la classe mf_in represente un fichier midi en cours de lecture, mais ce n'est pas elle qui effectue le job.
  Elle fournit juste des données et de methodes utilisees par song::load() qui itere track::load()
  Attention : la methode song::load() remplit les vecteurs events_tri[] mais pas events_merged[].

- la classe mf_out represente un fichier midi en cours d'ecriture, mais ce n'est pas elle qui effectue le job.
  Elle fournit juste des données et de methodes utilisees par song::save()

TIMING

- chaque event a 3 timestamps
	int mf_timestamp;	// midifile timestamp en midi ticks
	int ms_timestamp;	// millisecond timestamp apres application du tempo (via 1 ou plusieurs tempo events)
	int us_timestamp;	// microsecond timestamp   "       "        "   "
  ms_timestamp et us_timestamp sont redondants, un seul des deux devrait subsister a terme.
	- la motivation de ms_timestamp est la compatibilite avec portmidi qui prend des millisecondes
	- la motivation de us_timestamp est de moderer l'effet de cumulation des erreurs d'arrondi sur les tempo events
	  cette moderation pourrait etre encore meilleure avec un timestamp en double...
	  une autre motivation serait le fait que la us est l'unite officielle de tempo dans la spec midifile (us/quarter_note).
	- pour local synth (fluid, FM ou brisky) un timestamp en samples serait confortable, sans plus
  Leurs valeurs sont mises a jour par song::apply_tempo() et song::apply_tempo_u() respectivement (apres merge).
  Ce choix d'unite n'a aucune incidence sur l'ordre d'execution des events (merge est effectue avant, sur la base
  des midi timestamps)

DUMP

- song::dump()
	produit une representation reversible, au depart destine a etre compatible avec les progs MF2T/T2MF de Piet van Oostrum
- song::dump2()
	ajoute pour chaque event des infos detaillees de timing y compris hh:mn:ss et bars/beats 
*/

// un fichier midi en cours de lecture
class mf_in {
public :
int toberead;	// number of bytes of current chuck remaining
int totalread;	// total number of bytes already read
FILE * fil;
// constructeur
mf_in() : totalread(0), fil(NULL) {};
// destructeur
~mf_in() { if ( fil ) fclose( fil ); };
// methodes
int mopen( const char * fnam );
int mgetc();
int readvarinum();			// readvarinum - read a varying-length number
int read_big_endian( int n );		// lire un nombre de n bytes en big-endian
void chkfourcc( const char * fcc );	// lire et verifier un fourcc
};

// un fichier midi en cours d'ecriture
class mf_out {
public :
vector <unsigned char> data;		// le fichier entier en RAM
int cur_chunk_pos;			// position du 1er byte du chunk MTrk courant dans mf_data
unsigned int current_time;		// en quoi ???
// constructeur
mf_out() : current_time(0) {};
// methodes
void mputc( int c ) {
	data.push_back( (unsigned char)c );
	}
void writevarinum( unsigned int v );		// write a varying-length number
void write_time( unsigned int v );	// ecrire le temps relatif et mettre a jour le temps absolu
void write_big_endian( int v, int n );	// ecrire un nombre de n bytes en big-endian
void writefourcc( const char * fcc );	// ecrire un fourcc
void write_chunk_size();		// mettre a jour la taille du chunk
void commit( const char * fnam );	// sauver sur disk
};

// classe derivee pour l'ecriture
class wevent : public midi_event {
public :
// methodes
int write( mf_out * mf );	// ecrire un event dans un fichier midi, rend iext comme dump()
};

// classe derivee pour l'ecriture de l'extension d'un event t.q. sysex ou meta-event avec plus de 4 bytes de data
class wevent_ext : public midi_event_ext {
public :
void write( mf_out * mf );		// ecrire un event etendu (en binaire, peu importe le type)
};

// time signature = definition mesure
// utilisee pour afficher (eventuellement) bars et beats
class timesig {
public :
int bpbar;	// numerateur = beats per bar
int mf_tpb;	// midifile ticks per beat
int mf_timestamp;	// midifile timestamp de l'event generateur
// constructeur
timesig( int t, int division, int numerateur=4, int denominateur=4, int thirtysecondnotes_per_midiquarter=8 ) {
  mf_timestamp = t;
  bpbar = numerateur;
/* valeur du beat en ticks : le calcul logique serait :
	valeur de la 32 nd en ticks : v  = division / thirtysecondnotes_per_midiquarter;
	valeur de la whole en ticks : v *= 32;
	valeur du beat en ticks     : v /= denominateur;
   on change l'ordre des operations pour tout faire en int :
*/
  // valeur de la whole note en ticks (normalement c'est division * 4 )
  mf_tpb  = division * 32;
  mf_tpb /= thirtysecondnotes_per_midiquarter;	// normalement c'est 8
  // valeur du beat en ticks
  mf_tpb /= denominateur;
  };
};

class song;

// piste (pouvant contenir de multiples canaux)
class track {
public :
song * parent;
vector <midi_event> events;		// les events, eventuellement dans le desordre
vector <int> events_tri;		// acces aux events tries
vector <midi_event_ext> event_exts;	// bytes des sysex et meta-events
int flags;
// methodes
void load( mf_in * mst, song * chanson );
void post_rec();			// post-recording process sur une track juste enregistree
void dump( FILE * fil );
};

// morceau
class song {
public :
int format;		// 0=single track, 1=normal (concurrent tracks), 2=patterns
int division;
double pulsation;	// t.q. le coeff de conversion mf_t --> ms_t est pulsation * tempo
vector <track> tracks;
vector <midi_event *> events_merged;
vector <timesig> timesigs;		// les time signatures telles que 4/4, 6/8, 5/4...

// constructeur
song() : division(384), pulsation( 1.0 / (1000.0 * (double)division ) ) { /* tracks.reserve(1024); */ };

// methodes
int load( const char * fnam );
int merge();			// mettre a jour events_merged
void set_cadence( double k );	// duree relative d'execution (defaut 1.0)
void apply_tempo();		// base sur events_merged
void apply_tempo_u();		// base sur events_merged
int is_fresh_recorded();	// si oui ms_t = mf_t
unsigned int get_duration_ms() {	// apres apply_tempo !
    if	( events_merged.size() )
	return ( events_merged[events_merged.size()-1]->ms_timestamp );
    return 0;
    };
unsigned int get_duration_us() {	// apres apply_tempo_u !
    if	( events_merged.size() )
	return ( events_merged[events_merged.size()-1]->us_timestamp );
    return 0;
    };

// formules de conversion (utiles seulement en cas de modification du song à chaud)
int mft2mst( unsigned int mf_timestamp );	// conversion timestamp ponctuelle absolue
int mft2mst0( int mf_timestamp, midi_event * evt ) {	// conversion timestamp ponctuelle relative
    double ms_t = pulsation * (double)evt->vel * double( mf_timestamp - evt->mf_timestamp );
    return( evt->ms_timestamp + (int)ms_t );
    };
int mst2mft( int ms_timestamp );
int mst2mft0( int ms_timestamp, midi_event * evt ) {
    double mf_t = double( ms_timestamp - evt->ms_timestamp ) / (pulsation * (double)evt->vel);
    return( evt->mf_timestamp + (int)ceil(mf_t) );
    };

int mft2ust( unsigned int mf_timestamp );
int mft2ust0( int mf_timestamp, midi_event * evt ) {
    double us_t = 1000.0 * pulsation * (double)evt->vel * double( mf_timestamp - evt->mf_timestamp );
    return( evt->us_timestamp + (int)us_t );
    };
int ust2mft( int us_timestamp );
int ust2mft0( int us_timestamp, midi_event * evt ) {
    double mf_t = double( us_timestamp - evt->us_timestamp ) / (1000.0 * pulsation * (double)evt->vel);
    return( evt->mf_timestamp + (int)ceil(mf_t) );
    };

void bar_n_beat( midi_event * ev, int * bar, int * beat );	// calcul bar et beat
void bar_n_beat( midi_event * ev, int * bar, double * beat );	// calcul bar et beat
int find( int ms_time );	// rend un index dans events_merged
int add_new_track();		// ajouter track vierge
int check( int verbose );	// retour 0 si ok
void dump( FILE * fil );	// dump par tracks compatible MF2T
void dump2( FILE * fil );	// dummp merged + tempo applied

int save( const char * fnam );
};
#endif

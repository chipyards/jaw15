/* classe derivee de song pour ajouter des filtres specifiques

Tous les filtres rendent 0 si Ok.

Filtres travaillant sur un seul song --------------------------------------------------------- :

int filter_1();
	Custom code : pour experimenter un filtre hard coded

int filter_transp( int dst );
	Transposition par demi-tons, (travaille sur events_merged, ignore les tracks)

int filter_division( int newdiv );
	Changer la division sans changer les tempos, avec recalcul de tous les mf_timestamps,
	preserve le music time (MBT) aux erreurs d'arrondi pres, pas d'erreur si newdiv est multiple de olddiv.
	(ne pas utiliser pour un fresh recording)

int filter_division_fresh( int newdiv );
	Changer la division en ajustant le tempo pour preserver les ms_timestamps,
	seulement pour fresh recording. (music time (MBT) non significatif)

int filter_init_tempo( int tempo, int num, int den );
	Cree ou efface la track 0, fixe tempo et time-sig

int filter_t_follow( int itrack, int bpkn );
	En se basant sur les notes de reference (k_notes) d'une track juste enregistree sans metronome
	(fresh recording), creer un tempo event a chaque k_note, pour accompagner les variations de cadence
	Les k_notes sont supposéees apparaitre tous les bpkn beats (1 beat = 1 midi quarter note)
	Ces notes sont ajustees au fur et a mesure pour que leur temps reel ne change pas et leur music time
	(MBT) tombe sur les beats de reference.
	Puis les autres events de la track sont ajustes pour que leur temps reel ne change pas.
	(cela concerne surtout les note-off et controles divers presents par accident)
	Les autres tracks sont ignorees.

int filter_CSV_follow( FILE * csv_fil, int bpkn );
	En se basant sur les timestamps de reference lus dans un fichier CSV de Sonic Visualizer (annotation layer),
	creer un tempo event a chaque timestamp, pour accompagner les variations de cadence
	Les timestamps de reference sont supposéees apparaitre tous les bpkn beats (1 beat = 1 midi quarter note)
	le filtre ecrit dans la track 0 de la song, qui est supposee etre fresh

int filter_SV_CSV_gen( int itrack, int k_note, int shift );
	ce filtre produit (en stdout) un texte CSV compatible avec Sonic Visualizer (annotation layer)
	une note clef k_note choisie par convention est utilisee comme repere temporel,
	la premiere note clef est mise a l'instant shift ms

int filter_lucina_drums_to_GM( int itrack );
	convertir une piste de Lucina drums en GM

Filtres travaillant sur 2 songs --------------------------------------------------------- :

int filter_copy_tempo( song * sng2 );
	copier la track 0 (tempo) du song sng2 (divisions doivent etre egales)

*/

#ifndef FILTER_H
#define FILTER_H

class song_filt : public song {
public :
// Filtres travaillant sur un seul song
int filter_1();
int filter_transp( int dst );
int filter_division( int newdiv );
int filter_division_fresh( int newdiv );
int filter_init_tempo( int tempo, int num, int den );
int filter_t_follow( int itrack, int bpkn );

int filter_instants_follow( vector <double> * timestamps, int bpkn );
int filter_SV_CSV_gen( int itrack, int k_note, int shift );
int filter_lucina_drums_to_GM( int itrack );
// Filtres travaillant sur 2 songs
int filter_copy_tempo( song * sng2 );

// methodes auxiliares
static int read_CSV_instants( const char * fnam, vector <double> * timestamps, int bpkn, int FIR=0 );

};

#endif

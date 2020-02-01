/* ce prog genere un son arbitraire
gcc -o wavgen.exe -Wall wav_head.c wavgen.c -lm

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "wav_head.h"

/* --------------------------------------- traitement erreur fatale */
void gasp( const char *fmt, ... )
{
  char lbuf[2048];
  va_list  argptr;
  va_start( argptr, fmt );
  vsprintf( lbuf, fmt, argptr );
  va_end( argptr );
  printf("STOP : %s\n", lbuf ); exit(1);
}
/* ---------------------------------------------------------------- */

// generation signal sinus frequence fixe
void gen_fix_f( wavpars *d, double f, int duree )
{
// parametres de generation
double amplitude = 1.0;

// variables temporaires
short frame[2];		// left et right...
int writcnt, i;
double phi, v;

d->wavsize = d->freq * duree;	// nombre de frames
WAVwriteHeader( d );

amplitude *= 32767.0;
phi = 0.0; 
for	( i = 0; i < d->wavsize; ++i )
	{
	v = amplitude * sin( phi );
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 * M_PI ) / (double)d->freq );
	writcnt = write( d->hand, frame, d->chan * sizeof(short) );
	if	( writcnt != ( d->chan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal triangle frequence fixe
void gen_tri_f( wavpars *d, double f, int duree )
{
// parametres de generation
double amplitude = 1.0;

// variables temporaires
short frame[2];		// left et right...
int writcnt, i;
double phi, v;

d->wavsize = d->freq * duree;	// nombre de frames
WAVwriteHeader( d );

amplitude *= 32767.0;
phi = 0.0; 
for	( i = 0; i < d->wavsize; ++i )
	{
	if	( phi > 2.0 )
		phi -= 2.0;
	v = ( phi > 1 )?( 2.0 - phi ):( phi );
	v -= 0.5;
	v *= amplitude;
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 ) / (double)d->freq );
	writcnt = write( d->hand, frame, d->chan * sizeof(short) );
	if	( writcnt != ( d->chan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal sinus balayage lineaire en frequence
void bal_f_lin( wavpars *d, int duree )
{
// parametres de generation
double f0 = 20.0;
double f1 = 15000;
double finc = (f1-f0)/(double)(d->freq*duree);
double amplitude = 0.7;

// variables temporaires
short frame[2];		// left et right...
int writcnt, i;
double phi, v, f;

d->wavsize = d->freq * duree;	// nombre de frames
WAVwriteHeader( d );

amplitude *= 32767.0;
f = f0;
phi = 0.0; 
for	( i = 0; i < d->wavsize; ++i )
	{
	f += finc;
	v = amplitude * sin( phi );
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 * M_PI ) / (double)d->freq );
	writcnt = write( d->hand, frame, d->chan * sizeof(short) );
	if	( writcnt != ( d->chan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal sinus balayage "log" en frequence
void bal_f_log( wavpars *d, int duree )
{
// parametres de generation
double f0 = 20.0;
double f1 = 15000;
double loginc = log( f1 / f0 ) / (double)(d->freq*duree);
double amplitude = 0.7;

// variables temporaires
short frame[2];		// left et right...
int writcnt, i;
double phi, v, flog, f;

d->wavsize = d->freq * duree;	// nombre de frames
WAVwriteHeader( d );

amplitude *= 32767.0;
flog = log( f0 );
phi = 0.0; 
for	( i = 0; i < d->wavsize; ++i )
	{
	flog += loginc;
	f = exp( flog );
	v = amplitude * sin( phi );
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 * M_PI ) / (double)d->freq );
	writcnt = write( d->hand, frame, d->chan * sizeof(short) );
	if	( writcnt != ( d->chan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

double midi2Hz( int midinote )
{	// knowing that A4 = 440 Hz = note 69
return 440.0 * pow( 2.0, ( ( midinote - 69 ) / 12.0 ) );
}

// generation signal sinus gamme a intervale uniforme specifie en demi-tons,
// debut et fin specifies en midi (A4 = 440Hz = midi 69)
// stereo, mouvement ascendant en L, descendant en R
void bal_gamme( wavpars *d, double duree_note, int interval, int midi0, int midi1 )
{
// parametres de generation
int qnotes = 1 + ( midi1 - midi0 ) / interval;
if	( qnotes <= 0 )
	gasp("trop peu de notes");
double amplitude = 0.99;
unsigned int samp_per_note = (int)( ((double)d->freq) * duree_note );

// variables temporaires
short frame[2];		// left et right...
unsigned int n[2];
double phi[2];		// left et right...
double f[2];

double v;
unsigned int writcnt, i, j;

d->wavsize = samp_per_note * qnotes;	// nombre total de frames
WAVwriteHeader( d );

amplitude *= 32767.0;
n[0] = midi0;
f[0] = midi2Hz( n[0] );
n[1] = midi1;
f[1] = midi2Hz( n[1] );
phi[0] = 0.0;
phi[1] = 0.0;

j = 0;
for	( i = 0; i < d->wavsize; ++i )
	{
	// produire 1 echantillon L et le sauver
	v = amplitude * sin( phi[0] );
	frame[0] = (short int)round(v);
	// incrementer la phase
	phi[0] += ( ( f[0] * 2.0 * M_PI ) / (double)d->freq );
	if	( phi[0] > ( 2.0 * M_PI ) )
		phi[0] -= ( 2.0 * M_PI );
	if	( d->chan > 1 )
		{
		// produire 1 echantillon R et le sauver
		v = amplitude * sin( phi[1] );
		frame[1] = (short int)round(v);
		// incrementer la phase
		phi[1] += ( ( f[1] * 2.0 * M_PI ) / (double)d->freq );
		if	( phi[1] > ( 2.0 * M_PI ) )
			phi[1] -= ( 2.0 * M_PI );
		}
	// ecrire sur le disk
	writcnt = write( d->hand, frame, d->chan * sizeof(short) );
	if	( writcnt != ( d->chan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	// gerer la note
	if	( ++j == samp_per_note )
		{
		j = 0;
		n[0] += interval;
		f[0] = midi2Hz( n[0] );
		n[1] -= interval;	// mouvement oppose
		f[1] = midi2Hz( n[1] );
		}
	if	( j == 1 )
		{ printf("note %03u : %.3f Hz\n", n[0], f[0] ); fflush(stdout); }
	}
}


int main( int argc, char **argv )
{
if	( argc < 4 )
	{
	printf(	"usage :\n"
		"  bal. log : wavgen L <fichier_dest> <duree>\n"
		"  bal. lin : wavgen I <fichier_dest> <duree>\n"
		"  sin. fix : wavgen F <fichier_dest> <frequ>\n"
		"  tri. fix : wavgen T <fichier_dest> <frequ>\n"
		"  gamme tp : wavgen G <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"  quartes  : wavgen Q <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"  quintes  : wavgen D <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"(lettre minuscule pour mono au lieu de stereo)\n"
		);
	return 1;
	}

wavpars d;

d.type = 1;
d.resol = 16;
d.freq = 44100;
d.chan = 2;

d.hand = open( argv[2], O_RDWR | O_BINARY | O_CREAT | O_TRUNC, 0666 );
if	( d.hand == -1 )
	gasp("echec ouverture ecriture %s", argv[1] );

int opt = argv[1][0];
if	( opt >= 'a' )
	{
	opt -= ('a'-'A');
	d.chan = 1;
	}

switch	( opt )
	{
	case 'L' : bal_f_log( &d, (int)strtod( argv[3], NULL ) );
		break;
	case 'I' : bal_f_lin( &d, (int)strtod( argv[3], NULL ) );
		break;
	case 'F' : gen_fix_f( &d, strtod( argv[3], NULL ), 20 );
		break;
	case 'T' : gen_tri_f( &d, strtod( argv[3], NULL ), 20 );
		break;
	case 'G' : if	( argc == 6 )
			bal_gamme( &d, strtod( argv[3], NULL ), 1, (int)strtod( argv[4], NULL ), (int)strtod( argv[5], NULL ) );
		break;
	case 'Q' : if	( argc == 6 )
			bal_gamme( &d, strtod( argv[3], NULL ), 5, (int)strtod( argv[4], NULL ), (int)strtod( argv[5], NULL ) );
		break;
	case 'D' : if	( argc == 6 )
			bal_gamme( &d, strtod( argv[3], NULL ), 7, (int)strtod( argv[4], NULL ), (int)strtod( argv[5], NULL ) );
		break;
	}

close( d.hand );
return 0;
}


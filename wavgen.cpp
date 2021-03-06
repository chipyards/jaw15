/* ce prog genere un son arbitraire
g++ -o wavgen.exe -Wall -O2 wavio.cpp wavgen.cpp -lm

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "wavio.h"

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

// generation signal sinus frequence fixe f, ou avec vibrato de frequence fv
// la modulation de frequence est "logarithmique" (en fait exponentielle)
void gen_fix_f( wavio *d, double f, double fv, int duree )
{
// parametres de generation
double amplitude = 1.0;

// variables temporaires
short frame[2];		// left et right...
unsigned int writcnt, i, j;
double phi0, phi1, phiv;	// phase ch0, ch1, lfo
double v0, v1, vv;		// valeur instantannee ch0, ch1, lfo
double vamp;			// amplitude lfo en semitones
double kf;			// coeff instantanne de variation de freq.

d->realpfr = d->fsamp * duree;	// nombre de frames
d->WAVwriteHeader();

amplitude *= 32767.0;
phi0 = 0.0; phi1 = 0.0; phiv = 0.0;
vamp = 1;

for	( i = 0; i < d->realpfr; ++i )
	{
	v0 = amplitude * sin( phi0 );	// avec vibrato
	v1 = amplitude * sin( phi1 );	// sans vibrato, pour reference
	frame[0] = (short int)round(v0);
	frame[1] = (short int)round(v1);
	phi1 += ( ( f * 2.0 * M_PI ) / (double)d->fsamp );
	if	( fv == 0.0 )
		phi0 += ( ( f * 2.0 * M_PI ) / (double)d->fsamp );
	else	{
		j = i / ( 2 * d->fsamp );	// temps en unites de 2 secondes
		vamp = double(j) / 4;		// increment 1/4 de demi-ton ==> 2 tons en 32s
		vv = vamp * cos( phiv );	// LFO : oscillateur de vibrato
		kf = pow( 2.0, vv / 12.0 );
		phi0 += ( ( f * kf * 2.0 * M_PI ) / (double)d->fsamp );
		phiv += ( ( fv * 2.0 * M_PI ) / (double)d->fsamp );
		}
	writcnt = write( d->hand, frame, d->qchan * sizeof(short) );
	if	( writcnt != ( d->qchan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal triangle frequence fixe
void gen_tri_f( wavio *d, double f, int duree )
{
// parametres de generation
double amplitude = 1.0;

// variables temporaires
short frame[2];		// left et right...
unsigned int writcnt, i;
double phi, v;

d->realpfr = d->fsamp * duree;	// nombre de frames
d->WAVwriteHeader();

amplitude *= 32767.0;
phi = 0.0; 
for	( i = 0; i < d->realpfr; ++i )
	{
	if	( phi > 2.0 )
		phi -= 2.0;
	v = ( phi > 1 )?( 2.0 - phi ):( phi );
	v -= 0.5;
	v *= amplitude;
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 ) / (double)d->fsamp );
	writcnt = write( d->hand, frame, d->qchan * sizeof(short) );
	if	( writcnt != ( d->qchan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal sinus balayage lineaire en frequence
void bal_f_lin( wavio *d, int duree )
{
// parametres de generation
double f0 = 20.0;
double f1 = 15000;
double finc = (f1-f0)/(double)(d->fsamp*duree);
double amplitude = 0.7;

// variables temporaires
short frame[2];		// left et right...
unsigned int writcnt, i;
double phi, v, f;

d->realpfr = d->fsamp * duree;	// nombre de frames
d->WAVwriteHeader();

amplitude *= 32767.0;
f = f0;
phi = 0.0; 
for	( i = 0; i < d->realpfr; ++i )
	{
	f += finc;
	v = amplitude * sin( phi );
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 * M_PI ) / (double)d->fsamp );
	writcnt = write( d->hand, frame, d->qchan * sizeof(short) );
	if	( writcnt != ( d->qchan * sizeof(short) ) )
		gasp("erreur ecriture disque (plein?)");
	}
}

// generation signal sinus balayage "log" en frequence
void bal_f_log( wavio *d, int duree )
{
// parametres de generation
double f0 = 20.0;
double f1 = 15000;
double loginc = log( f1 / f0 ) / (double)(d->fsamp*duree);
double amplitude = 0.7;

// variables temporaires
short frame[2];		// left et right...
unsigned int writcnt, i;
double phi, v, flog, f;

d->realpfr = d->fsamp * duree;	// nombre de frames
d->WAVwriteHeader();

amplitude *= 32767.0;
flog = log( f0 );
phi = 0.0; 
for	( i = 0; i < d->realpfr; ++i )
	{
	flog += loginc;
	f = exp( flog );
	v = amplitude * sin( phi );
	frame[0] = (short int)round(v);
	frame[1] = frame[0];
	phi += ( ( f * 2.0 * M_PI ) / (double)d->fsamp );
	writcnt = write( d->hand, frame, d->qchan * sizeof(short) );
	if	( writcnt != ( d->qchan * sizeof(short) ) )
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
void bal_gamme( wavio *d, double duree_note, int interval, int midi0, int midi1 )
{
// parametres de generation
int qnotes = 1 + ( midi1 - midi0 ) / interval;
if	( qnotes <= 0 )
	gasp("trop peu de notes");
double amplitude = 0.99;
unsigned int samp_per_note = (int)( ((double)d->fsamp) * duree_note );

// variables temporaires
short frame[2];		// left et right...
unsigned int n[2];
double phi[2];		// left et right...
double f[2];

double v;
unsigned int writcnt, i, j;

d->realpfr = samp_per_note * qnotes;	// nombre total de frames
d->WAVwriteHeader();

amplitude *= 32767.0;
n[0] = midi0;
f[0] = midi2Hz( n[0] );
n[1] = midi1;
f[1] = midi2Hz( n[1] );
phi[0] = 0.0;
phi[1] = 0.0;

j = 0;
for	( i = 0; i < d->realpfr; ++i )
	{
	// produire 1 echantillon L et le sauver
	v = amplitude * sin( phi[0] );
	frame[0] = (short int)round(v);
	// incrementer la phase
	phi[0] += ( ( f[0] * 2.0 * M_PI ) / (double)d->fsamp );
	if	( phi[0] > ( 2.0 * M_PI ) )
		phi[0] -= ( 2.0 * M_PI );
	if	( d->qchan > 1 )
		{
		// produire 1 echantillon R et le sauver
		v = amplitude * sin( phi[1] );
		frame[1] = (short int)round(v);
		// incrementer la phase
		phi[1] += ( ( f[1] * 2.0 * M_PI ) / (double)d->fsamp );
		if	( phi[1] > ( 2.0 * M_PI ) )
			phi[1] -= ( 2.0 * M_PI );
		}
	// ecrire sur le disk
	writcnt = write( d->hand, frame, d->qchan * sizeof(short) );
	if	( writcnt != ( d->qchan * sizeof(short) ) )
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
		"  sin. fix : wavgen F <fichier_dest> <frequ> {<freq. vibrato>}\n"
		"  tri. fix : wavgen T <fichier_dest> <frequ>\n"
		"  gamme tp : wavgen G <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"  quartes  : wavgen Q <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"  quintes  : wavgen D <fichier_dest> <duree_note> <midi_note0> <midi_note1>\n"
		"(lettre minuscule pour mono au lieu de stereo)\n"
		);
	return 1;
	}

wavio d;

d.type = 1;
d.monosamplesize = 2;	// 16 bits
d.fsamp = 44100;
d.qchan = 2;

d.hand = open( argv[2], O_RDWR | O_BINARY | O_CREAT | O_TRUNC, 0666 );
if	( d.hand == -1 )
	gasp("echec ouverture ecriture %s", argv[1] );

int opt = argv[1][0];
if	( opt >= 'a' )
	{
	opt -= ('a'-'A');
	d.qchan = 1;
	}

switch	( opt )
	{
	case 'L' : bal_f_log( &d, (int)strtod( argv[3], NULL ) );
		break;
	case 'I' : bal_f_lin( &d, (int)strtod( argv[3], NULL ) );
		break;
	case 'F' : if	( argc == 5 )
			gen_fix_f( &d, strtod( argv[3], NULL ), strtod( argv[4], NULL ), 32 );
		   else	gen_fix_f( &d, strtod( argv[3], NULL ), 0, 20 );
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

// cloture du travail
// close( d.hand ); NOOOON ne pas faire cela
d.afclose();	// chucksize et filesize seront calcules par WAVwriteHeader en fonction de d.realpfr
 
printf("audio file closed, %d frames\n", d.realpfr );
return 0;
}


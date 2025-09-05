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
#include "cli_parse.h"

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

double midi2Hz( int midinote )
{	// knowing that A4 = 440 Hz = note 69
return 440.0 * pow( 2.0, ( ( midinote - 69 ) / 12.0 ) );
}

class CLI_params {
	public:
	double d;	// duree, totale ou note, toujours en s
	double b;	// begin, frequence ou midinote
	double e;	// end, frequence ou midinote
	double a;	// amplitude
	int mss;	// monosamplesize, 2 pour s16le, 4 pour f32
	int c;		// 1 pour mono, 2 pour stereo
	int f;		// fsamp
	char type;	// t.q. F pour sin frequ. fixe
	const char * fnam;

	// constructeur
	CLI_params() : d(1.0), b(440), e(440), a(0.5), mss(2), c(1), f(44100), type('F'), fnam(NULL) {};

	void parse( int argc, char **argv )
	{
	if	( argc < 2 )
		{ usage(); exit(0); }
	cli_parse * lepar = new cli_parse( argc, (const char **)argv, "dbeaft" );
	const char * val;
	if	( ( val = lepar->get( 'd' ) ) )	d = strtod( val, NULL );
	if	( ( val = lepar->get( 'b' ) ) )	b = strtod( val, NULL );
	if	( ( val = lepar->get( 'e' ) ) )	e = strtod( val, NULL );
	if	( ( val = lepar->get( 'a' ) ) )	a = strtod( val, NULL );
	if	( ( val = lepar->get( 'f' ) ) )	f = atoi( val );
	if	( ( val = lepar->get( 't' ) ) )	type = val[0];
	if		( lepar->get( 'F' ) )	mss = 4; else mss = 2;
	if		( lepar->get( 'S' ) )	c = 2;	 else c = 1;
	fnam = lepar->get( '@' );		// naked string = input file
	};

	void usage()
	{
	printf("// Usage //\n"
	" -d duree, totale ou note, toujours en s\n"
	" -b begin, frequence ou midinote\n"
	" -e end, frequence ou midinote\n"
	" -a amplitude\n"
	" -f fsamp\n"
	" -t type {}\n"
	"    L   bal. log\n"
	"    I   bal. lin\n"
	"    F   sin. fix\n"
	"    T   tri. fix\n"
	"    G   demitons (midinotes)\n"
	"    H   tierces  (midinotes)\n"
	"    Q   quartes  (midinotes)\n"
	"    D   quintes  (midinotes)\n"
	"    P   pulses	  (ton, toff)\n"
	" -F float out\n"
	" -S stereo out\n" );
	};

	void dump()
	{
	printf(" d = %g\n", d );
	printf(" b = %g\n", b );
	printf(" e = %g\n", e );
	printf(" a = %g\n", a );
	printf(" f = %d\n", f );
	printf(" type = %c\n", type );
	printf(" mss = %d\n", mss );
	printf(" c = %d\n", c );
	if ( fnam ) printf(" fnam = %s\n", fnam );
	};
};	// class CLI_params


// buffer pour accelerer l'ecriture disk, et encapsuler la conversion f32 -> s16le
// pcmbuf n'a pas vraiment besoin de savoir combien de canaux il y a,
// l'entrelacement est a la charge du client
// mais il a besoin de l'adresse du wavio
#define QBUF 4096
class pcmbuf {
	public:
	wavio * d;
	int monosamplesize;
	int lebuf[QBUF];
	int i;			// index prochaine position
	// init
	void init( wavio * w ) {
		d = w;
		monosamplesize = d->monosamplesize;
		i = 0;
		}
	// injecteur
	void add( double val ) {
		if	( monosamplesize == 2 )
			{
			short s = round(32767.0 *val);
			short * ibuf = (short *)lebuf;
			ibuf[i++] = s;
			if	( i >= (QBUF*2) )
				flush();
			}
		else if	( monosamplesize == 4 )
			{
			float * fbuf = (float *)lebuf;
			fbuf[i++] = (float)val;
			if	( i >= QBUF )
				flush();
			}
		};
	// vidage
	void flush() {
		int qpfr = i / d->qchan;
		int retval = d->write_data_p( lebuf, qpfr );
		if	( retval < 0 )
			gasp("pcmbuf write failed");
		i = 0;
		};
};	// class pcmbuf

// une ou deux deux variables globales pour les generateurs
double amplitude;

// generation signal sinus frequence fixe f, ou avec vibrato de frequence fv
// la modulation de frequence est "logarithmique" (en fait exponentielle)
// l'amplitude augmente par bond de 1/4 demi-ton toutes les 2s.
void gen_fix_f( pcmbuf *p, double f, double fv, double duree )
{
// variables temporaires
unsigned int i, j;
double phi0, phi1, phiv;	// phase ch0, ch1, lfo
double v0, v1, vv;		// valeur instantannee ch0, ch1, lfo
double vamp;			// amplitude lfo en semitones
double kf;			// coeff instantanne de variation de freq.

unsigned int estpfr = floor( double(p->d->fsamp) * duree );	// nombre de frames

phi0 = 0.0; phi1 = 0.0; phiv = 0.0;
vamp = 1;

for	( i = 0; i < estpfr; ++i )
	{
	v0 = amplitude * sin( phi0 );	// avec vibrato
	v1 = amplitude * sin( phi1 );	// sans vibrato, pour reference
	// sortir 1 ou 2 signaux
	p->add( v0 );
	if	( p->d->qchan == 2 )
		p->add( v1 );
	// mettre a jour les phases
	phi1 += ( ( f * 2.0 * M_PI ) / (double)p->d->fsamp );
	if	( fv == 0.0 )
		phi0 += ( ( f * 2.0 * M_PI ) / (double)p->d->fsamp );
	else	{
		j = i / ( 2 * p->d->fsamp );	// temps en unites de 2 secondes
		vamp = double(j) / 4;		// increment 1/4 de demi-ton ==> 2 tons en 32s
		vv = vamp * cos( phiv );	// LFO : oscillateur de vibrato
		kf = pow( 2.0, vv / 12.0 );
		phi0 += ( ( f * kf * 2.0 * M_PI ) / (double)p->d->fsamp );
		phiv += ( ( fv * 2.0 * M_PI ) / (double)p->d->fsamp );
		}
	}
}

// generation signal pulse frequence fixe
// mettre ton a zero pour pulse unitaire
void gen_pulse( pcmbuf *p, double ton, double toff, double duree )
{
// variables temporaires
unsigned int i, nexti, iton, itoff;

iton  = round( double(p->d->fsamp) * ton );
itoff = round( double(p->d->fsamp) * toff );
if 	( iton == 0 )
	iton = 1;

unsigned int estpfr = floor( double(p->d->fsamp) * duree );	// nombre de frames

int v = 0;
nexti = itoff;
for	( i = 0; i < estpfr; ++i )
	{
	if	( i == nexti )
		{
		if	( v )
			{ v = 0; nexti += itoff; }
		else	{ v = 1; nexti += iton; }
		}
	p->add( v * amplitude );
	if	( p->d->qchan == 2 )
		p->add( v );
	}
}


// generation signal triangle frequence fixe
void gen_tri_f( pcmbuf *p, double f, double duree )
{
// variables temporaires
unsigned int i;
double phi, v;

unsigned int estpfr = floor( double(p->d->fsamp) * duree );	// nombre de frames

phi = 0.0; 
for	( i = 0; i < estpfr; ++i )
	{
	if	( phi > 2.0 )			// dent de scie de 0 a 2
		phi -= 2.0;
	v = ( phi > 1 )?( 2.0 - phi ):( phi );	// pliage de 0 a 1
	v -= 0.5;				// centrage
	v *= 2.0 * amplitude;
	// sortir 1 ou 2 signaux
	p->add( v );
	if	( p->d->qchan == 2 )
		p->add( v );
	// mettre a jour les phases
	phi += ( ( f * 2.0 ) / (double)p->d->fsamp );
	}
}

// generation signal sinus balayage lineaire en frequence
void bal_f_lin( pcmbuf *p, double duree, double f0, double f1 )
{
unsigned int estpfr = floor( double(p->d->fsamp) * duree );	// nombre de frames

// parametres de generation
double finc = (f1-f0)/double( estpfr );

// variables temporaires
unsigned int i;
double phi, v, f;

f = f0;
phi = 0.0; 
for	( i = 0; i < estpfr; ++i )
	{
	f += finc;
	v = amplitude * sin( phi );
	// sortir 1 ou 2 signaux
	p->add( v );
	if	( p->d->qchan == 2 )
		p->add( v );
	// mettre a jour les phases
	phi += ( ( f * 2.0 * M_PI ) / (double)p->d->fsamp );
	}
}

// generation signal sinus balayage "log" en frequence
void bal_f_log( pcmbuf *p, float duree, double f0, double f1 )
{
unsigned int estpfr = floor( double(p->d->fsamp) * duree );	// nombre de frames

// parametres de generation
if	( ( f0 == 0.0 ) || ( f1 == 0.0 ) )
	gasp("freq zero interdite pour balayage log");
double loginc = log( f1 / f0 ) / double( estpfr );

// variables temporaires
unsigned int i;
double phi, v, flog, f;

flog = log( f0 );
phi = 0.0; 
for	( i = 0; i < estpfr; ++i )
	{
	flog += loginc;
	f = exp( flog );
	v = amplitude * sin( phi );
	// sortir 1 ou 2 signaux
	p->add( v );
	if	( p->d->qchan == 2 )
		p->add( v );
	// mettre a jour les phases
	phi += ( ( f * 2.0 * M_PI ) / (double)p->d->fsamp );
	}
}


// generation signal sinus gamme a intervale uniforme specifie en demi-tons,
// debut et fin specifies en midi (A4 = 440Hz = midi 69)
// stereo, mouvement ascendant en L, descendant en R
// mono, mouvements montant et descendant superposes
void bal_gamme( pcmbuf *p, double duree_note, int interval, int midi0, int midi1 )
{
// parametres de generation
int qnotes = 1 + ( midi1 - midi0 ) / interval;
if	( qnotes <= 0 )
	gasp("trop peu de notes");
if	( p->d->qchan <= 1 )
	amplitude *= 0.5;	// 0.5 en mono, car on somme les 2 canaux !
unsigned int samp_per_note = (int)( ((double)p->d->fsamp) * duree_note );

// variables temporaires
unsigned int n[2];
double phi[2];		// left et right...
double f[2];

double vL, vR;
unsigned int i, j;

unsigned int estpfr = samp_per_note * qnotes;	// nombre total de frames

n[0] = midi0;
f[0] = midi2Hz( n[0] );
n[1] = midi1;
f[1] = midi2Hz( n[1] );
phi[0] = 0.0;
phi[1] = 0.0;

j = 0;
for	( i = 0; i < estpfr; ++i )
	{
	// produire 1 echantillon L ascendant et le sauver
	vL = amplitude * sin( phi[0] );
	// incrementer la phase
	phi[0] += ( ( f[0] * 2.0 * M_PI ) / (double)p->d->fsamp );
	if	( phi[0] > ( 2.0 * M_PI ) )
		phi[0] -= ( 2.0 * M_PI );
	// produire 1 echantillon descendant R et le sauver
	vR = amplitude * sin( phi[1] );
	if	( p->d->qchan > 1 )
		{
		p->add( vL );		// stereo
		p->add( vR );
		}
	else	p->add( vL + vR );	// mono
	// incrementer la phase
	phi[1] += ( ( f[1] * 2.0 * M_PI ) / (double)p->d->fsamp );
	if	( phi[1] > ( 2.0 * M_PI ) )
		phi[1] -= ( 2.0 * M_PI );
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
CLI_params clic;
wavio d;
pcmbuf p	;

clic.parse( argc, argv );
clic.dump();

if	( clic.fnam == NULL )
	gasp("manque nom de fichier");
if	( strlen( clic.fnam ) < 4 )
	gasp("bad nom de fichier");


d.monosamplesize = clic.mss;
d.type = (d.monosamplesize==4)?3:1;
d.fsamp = clic.f;
d.qchan = clic.c;

d.write_head( clic.fnam );
p.init( &d );

amplitude = clic.a;	// global amplitude

switch	( clic.type )
	{
	case 'L' : bal_f_log( &p, clic.d, clic.b, clic.e );
		break;
	case 'I' : bal_f_lin( &p, clic.d, clic.b, clic.e );
		break;
	case 'F' : gen_fix_f( &p, clic.b, clic.e, clic.d );
		break;
	case 'T' : gen_tri_f( &p, clic.b, clic.d );
		break;
	case 'G' : bal_gamme( &p, clic.d, 1, (int)clic.b, (int)clic.e );
		break;            
	case 'H' : bal_gamme( &p, clic.d, 4, (int)clic.b, (int)clic.e );
		break;            
	case 'Q' : bal_gamme( &p, clic.d, 5, (int)clic.b, (int)clic.e );
		break;            
	case 'D' : bal_gamme( &p, clic.d, 7, (int)clic.b, (int)clic.e );
		break;
	case 'P' : gen_pulse( &p, clic.b, clic.e, clic.d );
		break;
	}

p.flush();
// ici d.realpfr est a jour grace a d.write_data_p() appele par p->flush()
d.afclose();	// chucksize et filesize seront calcules en fonction de d.realpfr
 
printf("audio file closed, %d frames\n", d.realpfr );
return 0;
}


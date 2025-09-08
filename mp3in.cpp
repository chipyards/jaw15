/*
	mp3in : lecture fichier mp3 base sur libmpg123

pkg-config --cflags libmpg123 --> -I/mingw32/include (deja vu)
pkg-config --libs   libmpg123 --> -L/mingw32/lib -lmpg123 -lm -lshlwapi

-I/mingw32/include, -L/mingw32/lib, -lm sont implicites,
shlwapi = "The Shell Light-Weight API" une vieillerie de microsoft,
on s'en passera si on peut

https://www.mpg123.de/api/

g++ -Wall -o mp3in.exe mp3in.cpp -lmpg123 -O2 -mms-bitfields

*/

#include <mpg123.h>
#include <cstdlib>
#include <stdio.h>

#include "mp3in.h"

int mp3in::read_head( const char * fnam, int verbose )
{
int encoding;
int retval;

retval = mpg123_init();
if	( retval != MPG123_OK )		// note :  MPG123_OK=0
	{ errfunc = "init"; return retval; }

mhand = mpg123_new( NULL, &retval );
if	( mhand == NULL)
	{ errfunc = "new"; return retval; }

retval = mpg123_param( mhand, MPG123_RESYNC_LIMIT, -1, 0 );
if	( retval != MPG123_OK )
	{ errfunc = "param resync"; return retval; }

retval = mpg123_param( mhand, MPG123_VERBOSE, verbose, 0 );	// valeurs utiles 2, 3, 4
if	( retval != MPG123_OK )
	{ errfunc = "param verb"; return retval; }

retval = mpg123_format_none( mhand );
if	( retval != MPG123_OK )
	{ errfunc = "format none"; return retval; }

// ici on indique quel format de sortie on accepte, mpg123 va choisir le plus relevant
// ou convertir (ce qu'on veut eviter)
//	sample rate	: 0 = on accepte tout, i.e. selon input file
//	channels	: MPG123_STEREO | MPG123_MONO = on accepte tout, i.e. selon input file
//	encoding	: MPG123_ENC_FLOAT_32 (par defaut (non documente) etait MPG123_ENC_SIGNED_16)
retval = mpg123_format2( mhand, 0, MPG123_STEREO | MPG123_MONO, MPG123_ENC_FLOAT_32 );
if	( retval != MPG123_OK )
	{ errfunc = "format FLOAT_32"; return retval; }

retval = mpg123_open( mhand, fnam );
if	( retval != MPG123_OK )
	{ errfunc = "open"; return retval; }

retval = mpg123_getformat( mhand, (long *)&fsamp, (int *)&qchan, &encoding );
if	( retval != MPG123_OK )
	{ errfunc = "getformat"; return retval; }

// encoding = 0xD0 pour s16 ==> monosamplesize = 2 bytes/sample (mono)
monosamplesize = mpg123_encsize(encoding);

outblock = mpg123_outblock( mhand );

estpfr = mpg123_length( mhand );
if	( estpfr < 0 )		
	{ errfunc = "length"; return estpfr; }

return 0;
}

/* The buffer size qpcmbuf, in BYTES, should be a multiple of the PCM frame size (qchan * 2)
// retourne le nombre de BYTES lus ( 0 si fini ) ou -1 si erreur
int mp3in::read_data_b( void * pcmbuf, size_t qpcmbuf )
{
size_t cnt;
int retval;

retval = mpg123_read( mhand, pcmbuf, qpcmbuf, &cnt );
if	( ( retval != MPG123_OK ) && ( retval != MPG123_DONE ) )
	{
	errfunc = mpg123_plain_strerror(retval);
	return -1;
	}
realpfr += ( cnt * monosamplesize * qchan );
return (int)cnt;
} */

// The buffer size qpfr is in PCM frame
// retourne le nombre de pcm frames lus ( 0 si fini ) ou -1 si erreur
int mp3in::read_data_p( void * pcmbuf, unsigned int qpfr )
{
size_t cnt;
int retval;

qpfr *= ( monosamplesize * qchan );
retval = mpg123_read( mhand, pcmbuf, qpfr, &cnt );
if	( ( retval != MPG123_OK ) && ( retval != MPG123_DONE ) )
	{
	errfunc = mpg123_plain_strerror(retval);
	return -1;
	}
cnt /= ( monosamplesize * qchan );
realpfr += cnt; 
return (int)cnt;
}

/* just for separate test *
#define QPFR 1152

int main( int argc, char *argv[] )
{
int retval;
mp3in m3;
unsigned char* pcmbuf;
size_t qpcmbuf;
unsigned int qpfr = QPFR;

int verbose = 4;

if	( argc <= 1 )
	return 1;

if	( argc > 2 )
	verbose = atoi( argv[2] );

// 1ere etape : lire un premiere bloc pour avoir les parametres
retval = m3.read_head( argv[1], verbose );
if	( retval )
	{
	printf("error read_head: %s : %s\n", m3.errfunc, mpg123_plain_strerror(retval) );
	return -1;
	}
printf("JAW: got %d channels @ %d Hz, monosamplesize %d\n",
	m3.qchan, m3.fsamp, m3.monosamplesize );
printf("JAW: recommended buffer %d bytes (vs %d)\n", (int)m3.outblock, 1152 * m3.monosamplesize * m3.qchan );
fflush(stdout);

// 2eme etape : allouer de la memoire
qpcmbuf = m3.outblock;	// outblock = 1152 * monosamplesize * qchan <==> 1 bloc mpeg

pcmbuf = (unsigned char *)malloc( qpcmbuf );
if	( pcmbuf == NULL )
	return -666;

// 3eme etape : boucler sur le buffer
do	{
	retval = m3.read_data_p( pcmbuf, qpfr );
	if	( retval < 0 )
		{
		printf("error read_data: %s\n", m3.errfunc );
		}
	} while ( retval == QPFR );

printf("JAW: %u vs %u pcm frames decoded\n", m3.realpfr, m3.estpfr );

mpg123_close(m3.mhand); mpg123_delete(m3.mhand); mpg123_exit();

fflush(stdout);
return 0;
}
//*/

/*
	mp3in : lecture fichier mp3 base sur libmpg123

pkg-config --cflags libmpg123 --> -I/mingw32/include (deja vu)
pkg-config --libs   libmpg123 --> -L/mingw32/lib -lmpg123 -lm -lshlwapi

-I/mingw32/include, -L/mingw32/lib, -lm sont implicites,
shlwapi = "The Shell Light-Weight API" une vieillerie de microsoft,
on s'en passera si on peut

https://www.mpg123.de/api/

gcc -Wall -o mp3in.exe mp3in.cpp -lmpg123

*/

#include <mpg123.h>
#include <stdio.h>

#include "mp3in.h"

int mp3in::read_head( const char * fnam )
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

retval = mpg123_param( mhand, MPG123_VERBOSE, verbose, 0 );
if	( retval != MPG123_OK )
	{ errfunc = "param verb"; return retval; }

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

/*
int main( int argc, char *argv[] )
{
int retval;
mp3in m3;
unsigned char* pcmbuf;
size_t qpcmbuf;
size_t totbytes;

if	( argc <= 1 )
	return 1;

if	( argc > 2 )
	m3.verbose = atoi( argv[2] );

// 1ere etape : lire un premiere bloc pour avoir les parametres
retval = m3.read_head( argv[1] );
if	( retval )
	{
	printf("error read_head: %s : %s\n", m3.errfunc, mpg123_plain_strerror(retval) );
	return -1;
	}
printf("got %d channels @ %d Hz, monosamplesize %d\n",
	m3.qchan, m3.fsamp, m3.monosamplesize );
printf("recommended buffer %d bytes\n", (int)m3.outblock );
printf("estimated length : %u samples\n", (unsigned int)m3.estqsamp );
fflush(stdout);

// 2eme etape : allouer de la memoire
qpcmbuf = m3.outblock;	// outblock = 1152 * monosamplesize * qchan <==> 1 bloc mpeg

pcmbuf = (unsigned char *)malloc( qpcmbuf );
if	( pcmbuf == NULL )
	return -666;

// 3eme etape : boucler sur le buffer
totbytes = 0;
do	{
	retval = m3.read_data_b( pcmbuf, qpcmbuf );
	if	( retval > 0 )
		totbytes += retval;
	} while ( retval == (int)qpcmbuf );

if	( retval < 0 )
	{
	printf("error read_data: %s\n", m3.errfunc );
	}

printf("%u bytes decoded (%u vs %u (est.) samples)\n", (unsigned int)totbytes,
	((unsigned int)totbytes)/(m3.monosamplesize*m3.qchan), (unsigned int)m3.estqsamp );

mpg123_close(m3.mhand); mpg123_delete(m3.mhand); mpg123_exit();

fflush(stdout);
return 0;
}
*/

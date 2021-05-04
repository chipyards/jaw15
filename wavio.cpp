/* WAV est un cas de fichier RIFF.
 * le fichier RIFF commence par "RIFF" (4 caracteres) suivi de la longueur
 * du reste du fichier en long (4 bytes ordre Intel)
 * puis on trouve "WAVE" suivi de chucks.
 * chaque chuck commence par 4 caracteres (minuscules) suivis de la longueur
 * du reste du chuck en long (4 bytes ordre Intel)
 * le chuck "fmt " contient les parametres, sa longueur n'est pas tres fixe,
 * il peut y avoir un chuck "fact" contenant le nombre de samples
 * le chuck "data" contient les samples,
 * tout autre doit etre ignore.
 * ATTENTION : en float 32 : valeurs samples sur [-1.0 , 1.0]

 header mini :
	     _
 "RIFF"    4  |
 filesize  4  } == 12 bytes de pre-header
 "WAVE"    4 _|
 "fmt "    4     le chuck fmt coute  au moins 24 bytes en tout
 chucksize 4 _   == 16 bytes utile
 type      2  |				1 pour sl 16		3 pour float
 qchan     2  |
 fsamp     4  } == 16 bytes		(par exemple 44100 = 0xAC44)
 bpsec     4  |		bytes/s		block * fsamp
 block     2  |		bytes/frame	4			8
 resol     2 _|		bits/sample	16			32
 "data"    4
 chucksize 4    == 8 bytes de post-header
 --------------
          44 au total

 RIFF filesize = real filesize - 8
 chucksize     = real filesize - 44
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "wavio.h"

static unsigned int readlong( unsigned char *buf )
{
unsigned int l;
l  = (unsigned int)buf[3] << 24 ;
l += (unsigned int)buf[2] << 16 ;
l += (unsigned int)buf[1] << 8 ;
l += buf[0];
return(l);
}

static unsigned int readshort( unsigned char *buf )
{
unsigned int s;
s = buf[0] + ( buf[1] << 8 );
return(s);
}

#define QBUF 4096

int wavio::WAVreadHeader()
{
unsigned char buf[QBUF]; unsigned int chucksize, factsize, resol;
read( this->hand, buf, 4 );
if ( strncmp( (char *)buf, "RIFF", 4 ) != 0 ) return -2;
read( this->hand, buf, 4 ); // filesize = readlong( buf );
read( this->hand, buf, 4 );
if ( strncmp( (char *)buf, "WAVE", 4 ) != 0 ) return -3;
this->type = 0x10000;
factsize = 0;
chucksize = 0;
while	( read( this->hand, buf, 8 ) == 8 )	// boucle des chucks preliminaires
	{
	chucksize = readlong( buf + 4 );
	if	( strncmp( (char *)buf, "data", 4 ) == 0 )
		break;	// c'est le bon chuck, on le lira plus tard...
	buf[4] = 0;
	// printf("chuck %s : %d bytes\n", buf, chucksize );
	if	( chucksize > QBUF ) return -4;
	if	( strncmp( (char *)buf, "fmt ", 4 ) == 0 )		// chuck fmt
		{
		if ( chucksize > QBUF ) return -5;
		read( this->hand, buf, (int)chucksize );
		this->type = readshort( buf );
		if	( ( this->type == 1 ) || ( this->type == 3 ) || ( this->type == 0xFFFE ) )
			{
			this->qchan = readshort( buf + 2 );
			this->fsamp = readlong( buf + 4 );
			this->bpsec = readlong( buf + 8 );
			this->block = readshort( buf + 12 );
			resol = readshort( buf + 14 );
			this->monosamplesize = resol / 8;
			this->estpfr = 0;
			if	( this->type == 0xFFFE )		// WAVE_FORMAT_EXTENSIBLE
				{
				printf("WAVE_FORMAT_EXTENSIBLE\n");
				if	( this->qchan > 2 )
					return -6;
				int extsize = readshort( buf + 16 );
				if	( extsize == 22 )
					{
					unsigned int validbits = readshort( buf + 18 );
					if	( validbits != resol )
						return -7;
					int ext_type = readshort( buf + 20 );
					if	( ( ext_type != 1 ) && ( ext_type != 3 ) )
						return -8;
					this->type = ext_type;
					}
				else	return -9;
				}
			}
		else	return( -100 - this->type );
		}
	else if	( strncmp( (char *)buf, "fact", 4 ) == 0 )		// chuck fact
		{
		read( this->hand, buf, chucksize );
		factsize = readlong( buf );
		}
	else	read( this->hand, buf, chucksize );				// autre chuck
	}
if	( this->type == 0x10000 ) return -10;
if	( strncmp( (char *)buf, "data", 4 ) != 0 ) return -11;
					// on est bien arrive dans les data !
this->estpfr = chucksize / ( this->qchan * this->monosamplesize );

// printf("%u canaux, %u Hz, %u bytes/s, %u bytes/monosample\n",
//       this->qchan, this->fsamp, this->bpsec, this->monosamplesize );
// printf("%u echantillons selon data chuck\n", this->estpfr );
if	( factsize != 0 )
	{
	// printf("%u echantillons selon fact chuck\n", factsize );
	if ( this->estpfr != factsize ) return -200;
	}
// printf("fichier %u bytes, chuck data %u bytes\n", filesize, chucksize );
/*	{
	double durs, dmn, ds; int mn;
	durs = this->estpfr;  durs /= this->fsamp;
	dmn = durs / 60.0;  mn = (int)dmn; ds = durs - 60.0 * mn;
	printf("duree %.3f s soit %d mn %.3f s\n", durs, mn, ds );
	}
*/
return 0;
}

int wavio::read_head( const char * fnam )
{
hand = open( fnam, O_RDONLY | O_BINARY );
if	( hand == -1 )
	return -1;
return WAVreadHeader();
}

// The buffer size qpfr is in PCM frame
// retourne le nombre de pcm frames lus ( 0 si fini )
int wavio::read_data_p( void * pcmbuf, size_t qpfr )
{
int retval;
qpfr *= ( monosamplesize * qchan );
retval = read( hand, pcmbuf, qpfr );
retval /= ( monosamplesize * qchan );
realpfr += retval; 
return retval;
}

void wavio::afclose()
{ if ( hand >= 0 ) close( hand ); };

static void writelong( unsigned char *buf, unsigned int l )
{
buf[0] = (unsigned char)l; l >>= 8;
buf[1] = (unsigned char)l; l >>= 8;
buf[2] = (unsigned char)l; l >>= 8;
buf[3] = (unsigned char)l;
}
static void writeshort( unsigned char *buf, unsigned int s )
{
buf[0] = (unsigned char)s; s >>= 8;
buf[1] = (unsigned char)s;
}

static void gulp()
{ gasp("erreur ecriture fichier");
}

void wavio::WAVwriteHeader()
{
unsigned char buf[16]; long filesize, chucksize;

this->block = this->qchan * this->monosamplesize;
this->bpsec = this->fsamp * this->block;
chucksize = this->estpfr * this->block;
filesize = chucksize + 36;

if ( write( this->hand, "RIFF", 4 ) != 4 ) gulp();
writelong( buf, filesize );
if ( write( this->hand, buf, 4 ) != 4 ) gulp();

if ( write( this->hand, "WAVE", 4 ) != 4 ) gulp();
if ( write( this->hand, "fmt ", 4 ) != 4 ) gulp();
writelong( buf, 16 );
if ( write( this->hand, buf, 4 ) != 4 ) gulp();

writeshort( buf,      this->type  );
writeshort( buf + 2,  this->qchan  );
writelong(  buf + 4,  this->fsamp  );
writelong(  buf + 8,  this->bpsec );
writeshort( buf + 12, this->block );
writeshort( buf + 14, this->monosamplesize * 8 );
if ( write( this->hand, buf, 16 ) != 16 ) gulp();

if ( write( this->hand, "data", 4 ) != 4 ) gulp();
writelong( buf, chucksize );
if ( write( this->hand, buf, 4 ) != 4 ) gulp();
}

/* --------------------------------------- traitement erreur fatale */

/*
void gasp( const char *fmt, ... )
{
  char lbuf[2048];
  va_list  argptr;
  va_start( argptr, fmt );
  vsprintf( lbuf, fmt, argptr );
  va_end( argptr );
  printf("STOP : %s\n", lbuf ); exit(1);
}
*/

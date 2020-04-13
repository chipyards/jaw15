#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <pcre.h>
#include "pcreux.h"
#include "tk17.h"

void tk17::gen1key( FILE *fil, int ifram, double x, double y, double z, tangent_t tangent, int last )
{
fprintf( fil, " \"Class\": {\n \"$ID\": \"Key\",\n \"I32\": %d,\n ", ifram );
fprintf( fil, "\"VectorF32\": [%.10f, %.10f, %.10f],\n \"I32\": %d\n }", x, y, z, tangent );
if	( last )
	fprintf( fil, "\n}\n},\n");
else	fprintf( fil, ",\n");
}

void tk17::gen1trk_head( FILE *fil )
{
fprintf( fil, "\"Class\": {\n\"$ID\": \"Track\",\n\"I32\": %d,\n\"I32\": %d,\n\"Array\": {\n", perso, itrack );
}

// emettre une key pour chaque frame selon X[]
void tk17::gen1trk( FILE *fil )
{
gen1trk_head( fil );
int i;
for	( i = 0; i < qframes-1; ++i )
	{
	gen1key( fil, i, X[i], 0.0, 0.0, LIN, 0 );
	}
gen1key( fil, i, X[i], 0.0, 0.0, LIN, 1 );
printf("codage de %d frames termine\n", i+1 );
}

int tk17::json_load( const char * fnam )
{
FILE *fil;
fil = fopen( fnam, "r" );
if	( fil == NULL )
	return -1;
src_size = fread( json_src, 1, sizeof(json_src), fil );
if	( src_size <= 0 )
	return -2;
if	( src_size == sizeof(json_src) )
	return -3;
fclose( fil );
return 0;
}

int tk17::json_patch()
{
// lecture fichier de depart
int retval;

retval = json_load( src_fnam );

if	( retval < 0 )
	return retval;
printf("lu %s : %u bytes\n", src_fnam, src_size ); fflush(stdout);

// recherche de la track demandee
char lepat[256];
snprintf( lepat, sizeof(lepat),
	"(\"Class\":\\s+[{]\\s+\"[$]ID\":\\s+\"Track\""
	",\\s+\"I32\":\\s+%d,\\s+\"I32\":\\s+%d,"
	".*?)"	// anything, non greedy : capturer exactement 1 track
	"\\s+\"Class\":\\s+[{]\\s+\"[$]ID\":\\s+\"Track\"", perso, itrack );

printf("pattern : %s\n", lepat ); fflush(stdout);

pcreux * lareu;
lareu = new pcreux( lepat, PCRE_NEWLINE_ANY | PCRE_MULTILINE | PCRE_DOTALL );
lareu->letexte = json_src;
lareu->lalen = src_size;
lareu->start = 0;
if	( lareu->matchav() > 0 )
	{
	printf("%d(%d:%d)%d\n", lareu->ovector[0], lareu->ovector[2], lareu->ovector[3], lareu->ovector[1] ); fflush(stdout);
	}
else	return -10;

if	( lareu->ovector[2] > src_size )
	return -11;
if	( lareu->ovector[3] > src_size )
	return -12;

FILE * dfil = fopen( dst_fnam, "wb" );
if	( dfil == NULL )
	return -20;

int resu = fwrite( json_src, 1, lareu->ovector[2], dfil );
printf("ecrit %s : %u bytes\n", dst_fnam, resu );

gen1trk( dfil );

resu = fwrite( json_src+lareu->ovector[3], 1, src_size-lareu->ovector[3], dfil );
printf("ecrit %s : %u bytes\n", dst_fnam, resu );

fclose( dfil );
return 0;
}

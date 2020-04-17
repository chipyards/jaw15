#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <pcre.h>
#include "pcreux.h"
#include "tk17.h"

/** fonctions generiques ============================================================ */

// la difficulte c'est la virgule, il ne la faut pas a la fin de la derniere key
// on choisit de mettre la virgule devant, SAUF a la premiere key
// il DOIT exister une key a ifram == 0
void tk17::gen1key( FILE *fil, int ifram, double x, double y, double z, tangent_t tangent )
{
if	( ifram != 0 )
	fprintf( fil, ",\n");
fprintf( fil, " \"Class\": {\n \"$ID\": \"Key\",\n \"I32\": %d,\n ", ifram );
fprintf( fil, "\"VectorF32\": [%.10f, %.10f, %.10f],\n \"I32\": %d\n }", x, y, z, tangent );
}

void tk17::gen1trk_head( FILE *fil, int itrack )
{
fprintf( fil, "\n\"Class\": {\n\"$ID\": \"Track\",\n\"I32\": %d,\n\"I32\": %d,\n\"Array\": {\n", perso, itrack );
}

void tk17::gen1trk_tail( FILE *fil )
{
fprintf( fil, "\n}\n},\n");	// fin de l'array et fin de la track
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

// recherche de la track demandee dans json_src[], rend 0 si ok
int tk17::json_search( int itrack, int * pbegin, int * pend )
{
char lepat[256];
snprintf( lepat, sizeof(lepat),
	"(\"Class\":\\s+[{]\\s+\"[$]ID\":\\s+\"Track\""
	",\\s+\"I32\":\\s+%d,\\s+\"I32\":\\s+%d,"
	".*?)"	// anything, non greedy : capturer exactement 1 track
	"\\s+\"Class\":\\s+[{]\\s+\"[$]ID\":\\s+\"Track\"", perso, itrack );

// printf("pattern : %s\n", lepat ); fflush(stdout);

pcreux * lareu;
lareu = new pcreux( lepat, PCRE_NEWLINE_ANY | PCRE_MULTILINE | PCRE_DOTALL );
lareu->letexte = json_src;
lareu->lalen = src_size;
lareu->start = 0;
if	( lareu->matchav() > 0 )
	{
	// printf("regex match %d(%d:%d)%d\n", lareu->ovector[0], lareu->ovector[2], lareu->ovector[3], lareu->ovector[1] ); fflush(stdout);
	}
else	return -10;

if	( lareu->ovector[2] > src_size )
	return -11;
if	( lareu->ovector[3] > src_size )
	return -12;
*pbegin = lareu->ovector[2];
*pend   = lareu->ovector[3];
return 0;
}

// lire le premier vecteur qui se presente a partir de la position pos dans json_src[], rend 0 si ok
int tk17::json_search_1vect( int pos, double * x, double * y, double * z )
{
const char * lepat = "\"VectorF32\":\\s+\\[([-0-9.]+),\\s*([-0-9.]+),\\s*([-0-9.]+)\\]";
//printf("pattern : %s\n", lepat ); fflush(stdout);

pcreux * lareu;
lareu = new pcreux( lepat, PCRE_NEWLINE_ANY | PCRE_MULTILINE | PCRE_DOTALL );
lareu->letexte = json_src+pos;
lareu->lalen = src_size;
lareu->start = 0;
if	( lareu->matchav() > 0 )
	{
	//printf("regex match %d:%d: %d:%d %d:%d\n",
	//	lareu->ovector[2], lareu->ovector[3],
	//	lareu->ovector[4], lareu->ovector[5],
	//	lareu->ovector[6], lareu->ovector[7] ); fflush(stdout);
	}
else	return -10;

json_src[pos+lareu->ovector[3]] = 0;
json_src[pos+lareu->ovector[5]] = 0;
json_src[pos+lareu->ovector[7]] = 0;
//printf("|%s| |%s| |%s|\n",	json_src+pos+lareu->ovector[2],
//				json_src+pos+lareu->ovector[4], json_src+pos+lareu->ovector[6] );
*x = strtod( json_src+pos+lareu->ovector[2], NULL );
*y = strtod( json_src+pos+lareu->ovector[4], NULL );
*z = strtod( json_src+pos+lareu->ovector[6], NULL );
return 0;
}

/** fonctions specifiques de quelques tracks ============================================================ */

// animation discrete pour petits mouvements 3D (typiquement pour neck rotation 15)

void tk17::gen_3D_trk( FILE *fil, int itrack, double sigma_X, double sigma_Y, double sigma_Z, double Y0, int period )
{
double x, y, z;
gen1trk_head( fil, itrack );
int i=0, cnt=0, interval;
while	( i < qframes )
	{
	x = random_normal_double( 0.0, sigma_X );
	y = random_normal_double( Y0,  sigma_Y );
	z = random_normal_double( 0.0, sigma_Z );
	gen1key( fil, i, x, y, z, SMOOTH );
	++cnt;
	interval = random_normal_int( period, 8 );
	if	( interval < 6 )
		interval = 6;
	i += interval;
	}
// key terminale, pour eviter un rebouclage immediat
gen1key( fil, 1152-1, 0.0, Y0, 0.0, LIN );
gen1trk_tail( fil );
printf("track %d : periode moy. %d frames, 3D sigma %g %g %g\n", itrack, period, sigma_X, sigma_Y, sigma_Z );
printf("track %d : codage de %d frames de 0 a %d, + 1 frame @ 1151 : termine\n", itrack, cnt, qframes );
}

// animation rotation bassin :
void tk17::gen_hips_trk_1( FILE *fil, double y0 )
{		//	tangage		lacet	roulis
gen_3D_trk( fil, 1, hips_angle*0.4, hips_angle, hips_angle*0.4, y0, hips_period );
}

// animation neck : meme amplitude sur les 3 axes
void tk17::gen_neck_trk_15( FILE *fil )
{
gen_3D_trk( fil, 15, neck_angle, neck_angle, neck_angle, 0.0, neck_period );
}

void tk17::gen_shoulders_events()
{
// preparation des shoulders, a faire a l'avance car utilise par 2 tracks
int i, cnt=0, interval;
i = random_normal_int( shoulders_period, 7 );
while	( i < qframes )
	{
	shoulders[i] = random_normal_double( -shoulders_angle, shoulders_angle );
	++cnt;
	interval = random_normal_int( shoulders_period, 7 );
	if	( interval < 6 )
		interval = 6;
	i += interval;
	}
printf("prepared %d shoulders events\n", cnt );
}

// animation shoulders : -Y only, 2 epaules idem selon shoulders[]
void tk17::gen_shoulders_trk_17_18( FILE *fil, int itrack )
{
gen1trk_head( fil, itrack );
int i=0, cnt=0; double Z_init = 8.1544904709;	// valeur initiale pour pose "sitting"
gen1key( fil, i, 0.0, 0.0, Z_init, LIN );	// TOUJOURS une key @ 0
for	( i = 1; i < qframes; ++i )
	{
	if	( shoulders[i] != 0.0 )
		{
		gen1key( fil, i, 0.0, shoulders[i], Z_init, SMOOTH );
		++cnt;
		}
	}
// key terminale, pour eviter un rebouclage immediat
gen1key( fil, 1152-1, 0.0, 0.0, Z_init, SMOOTH );
gen1trk_tail( fil );
printf("track %d : codage de %d events de 0 a %d, + 1 frame @ 1151 : termine\n", itrack, cnt, qframes );
}

// animation "continue" (typiquement pour jaw open 126)
// emettre une key "scalaire" pour chaque frame de 0 a qframes-1 selon X[]
// le seuil sert a la fonction "economiseur"
void tk17::gen_jaw_trk_126( FILE *fil, double seuil )
{
int itrack = 126;
gen1trk_head( fil, itrack );
int i, cnt=0;
for	( i = 0; i < qframes; ++i )
	{
	if	( X[i] >= seuil )
		{ gen1key( fil, i, X[i], 0.0, 0.0, LIN ); ++cnt; }
	}
// key terminale, pour eviter un rebouclage immediat
if	( i < 1152 )
	gen1key( fil, 1152-1, X[0], 0.0, 0.0, LIN );
gen1trk_tail( fil );
printf("track %d : %g < X < %g\n", itrack, min_X, max_X );
printf("track %d : codage de %d frames de 0 a %d, + 1 frame @ 1151 : termine\n", itrack, cnt, i-1 );
}

void tk17::gen_blink_events()
{
// preparation du blink, a faire a l'avance car utilise par 2 tracks
int i, cnt=0, interval;
i = random_normal_int( blink_period, 7 );
while	( i < qframes )
	{
	blink_flag[i] = 1;
	++cnt;
	interval = random_normal_int( blink_period, 7 );
	if	( interval < 6 )
		interval = 6;
	i += interval;
	}
printf("prepared %d blink events\n", cnt );
}

// animation discrete pour mouvements 1D impulsionnels (typiquement eye blink 136+137)
void tk17::gen_blink_trk_136_137( FILE *fil, int itrack )
{
gen1trk_head( fil, itrack );
int i=0, cnt=0;
gen1key( fil, i, 0.0, 0.0, 0.0, LIN );	// TOUJOURS une key @ 0
for	( i = 1; i < qframes-2; ++i )
	{
	if	( blink_flag[i] )
		{
		gen1key( fil, i,   0.0, 0.0, 0.0, LIN );
		gen1key( fil, i+1, 0.7, 0.0, 0.0, LIN );
		gen1key( fil, i+2, 0.0, 0.0, 0.0, LIN );
		++cnt;
		}
	}
// key terminale, pour eviter un rebouclage immediat
gen1key( fil, 1152-1, 0.0, 0.0, 0.0, LIN );
gen1trk_tail( fil );
printf("track %d : periode moy. %d frames\n", itrack, blink_period );
printf("track %d : codage de %d events de 0 a %d, + 1 frame @ 1151 : termine\n", itrack, cnt, qframes );
}

void tk17::gen_breath_events( double seuil )
{
// preparation du breath, basee sur les silences
int i=0, cnt=0; float oldX = 1.0;
int ii = -1;	// frame de debut inspire
int ie;		// frame de fin inspire
for	( i = 1; i < qframes-1; ++i )
	{
	if	( ( X[i] < seuil ) && ( oldX >= seuil ) )
		{	// debut de silence
		ii = i-1;
		}
	if	( ( X[i] >= seuil ) && ( oldX < seuil ) )
		{	// fin de silence
		ie = i+1;
		if	( ( ie - ii ) > 6 )
			{
			if	( ii > 0 )
				breath_flag[ii] = -1;	// debut inspire
			breath_flag[ie] =  1;		// fin inspire
			cnt++;
			}
		}
	oldX = X[i];
	}
printf("prepared %d inspirations\n", cnt );
}

// animation discrete pour mouvements 1D a 2 temps (typiquement breath = vertebra 3 152)
void tk17::gen_breath_trk_152( FILE *fil )
{
int itrack = 152;
gen1trk_head( fil, itrack );
int i=0, cnt=0;
gen1key( fil, i, 0.0, 0.0, 0.0, LIN );	// TOUJOURS une key @ 0
for	( i = 1; i < qframes-2; ++i )
	{
	if	( breath_flag[i] == -1 )	// debut inspire
		{
		gen1key( fil, i, 0.0, 0.0, 0.0, SMOOTH );
		++cnt;
		}
	if	( breath_flag[i] == 1 )	// fin inspire
		{
		gen1key( fil, i, 0.0, 0.0, breath_breadth, SMOOTH );
		}
	}
// key terminale, pour eviter un rebouclage immediat
gen1key( fil, 1152-1, 0.0, 0.0, 0.0, LIN );
gen1trk_tail( fil );
printf("track %d : codage de %d respirations de 0 a %d, + 1 frame @ 1151 : termine\n", itrack, cnt, qframes );
}



/** fonction principale ============================================================ */

int tk17::json_patch()
{
int retval;
int ibegin, iend, ioldend;

// lecture fichier de depart
retval = json_load( src_fnam );
if	( retval < 0 )
	return retval;
printf("lu %s : %u bytes\n", src_fnam, src_size ); fflush(stdout);
// ouverture fichier dest
FILE * dfil = fopen( dst_fnam, "wb" );
if	( dfil == NULL )
	return -20;

ioldend = 0;

// recherche de la track 1 ------------------------------
retval = json_search( 1, &ibegin, &iend );
if	( retval != 0 ) 	return retval;
if	( ibegin < ioldend )	return -666;

double x, y, z;
retval = json_search_1vect( ibegin, &x, &y, &z );
if	( retval != 0 ) 	return retval;
printf("track 1 : 1er vecteur : [%g %g %g]\n", x, y, z );

// ecriture de ce qui la precede
retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 1
gen_hips_trk_1( dfil, y );

ioldend = iend;

// recherche de la track 15 ------------------------------
retval = json_search( 15, &ibegin, &iend );
if	( retval != 0 ) 	return retval;
if	( ibegin < ioldend )	return -666;

// ecriture de ce qui la precede
retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 15
gen_neck_trk_15( dfil );

ioldend = iend;

// recherche de la track 17 ------------------------------
retval = json_search( 17, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 17
gen_shoulders_events();
gen_shoulders_trk_17_18( dfil, 17 );

ioldend = iend;

// recherche de la track 18 ------------------------------
retval = json_search( 18, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 18
gen_shoulders_trk_17_18( dfil, 18 );

ioldend = iend;

// recherche de la track 126 ------------------------------
retval = json_search( 126, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 126
gen_jaw_trk_126( dfil );

ioldend = iend;

// recherche de la track 136 ------------------------------
retval = json_search( 136, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 136
gen_blink_events();
gen_blink_trk_136_137( dfil, 136 );

ioldend = iend;

// recherche de la track 137 ------------------------------
retval = json_search( 137, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 137
gen_blink_trk_136_137( dfil, 137 );

ioldend = iend;

// recherche de la track 152 ------------------------------
retval = json_search( 152, &ibegin, &iend );
if	( retval != 0 )		return retval;
if	( ibegin < ioldend )	return -666;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

// ecriture nouvelle track 152
gen_breath_events();
gen_breath_trk_152( dfil );

ioldend = iend;

// ecriture du reste --------------------------------------
ibegin = src_size;

retval = fwrite( json_src+ioldend, 1, ibegin-ioldend, dfil );
// printf("ecrit %s : %u bytes\n", dst_fnam, retval );

fclose( dfil );
printf("fini ecriture %s\n", dst_fnam ); fflush(stdout);
return 0;
}

/** utilitaires ====================================================================== */

// generation d'un double pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
double random_normal_double( double avg, double sigma )
{
double val = 0.0;
for	( int i = 0; i < 12; ++i )
	val += double(rand());
val /= double(RAND_MAX);
val -= 6.0;
val *= sigma;
val += avg;
return val;
}

// generation d'un entier pseudo gaussien (Irwin-Hall distribution)
// dans l'intervalle [avg-6*sigma, avg+6*sigma]
int random_normal_int( int avg, int sigma )
{
return int(round(random_normal_double( double(avg), double(sigma) )));
}

/* tests random
	{
	double val, vmin, vmax;
	vmin = 1000.0, vmax = -1000.0;
	for	( int i = 0; i < 100000; ++i )
		{
		val = random_normal_double( 0.0, 1.0 );
		if	( val < vmin ) vmin = val;
		if	( val > vmax ) vmax = val;
		}
	printf("gauss 0 1 : [%g %g]\n", vmin, vmax );

	vmin = 1000.0, vmax = -1000.0;
	for	( int i = 0; i < 1000; ++i )
		{
		val = random_normal_double( 5.0, 5.0/3.0 );
		if	( val < vmin ) vmin = val;
		if	( val > vmax ) vmax = val;
		}
	printf("gauss 5, 5/3 : [%g %g]\n", vmin, vmax );
	}
	{
	int val, vmin, vmax;
	vmin = 1000, vmax = -1000;
	for	( int i = 0; i < 100000; ++i )
		{
		val = random_normal_int( 0, 100 );
		if	( val < vmin ) vmin = val;
		if	( val > vmax ) vmax = val;
		}
	printf("gauss 0 100 : [%d %d]\n", vmin, vmax );
	}
*/

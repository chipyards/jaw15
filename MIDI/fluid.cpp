/* experimentation du fluid core
   n'utilise pas le player ni le file renderer de fluid ni ses drivers audio et midi

 -lfluidsynth

https://www.fluidsynth.org/api/
 */

#include <stdlib.h>
#include <stdio.h>
//#include <string.h>
//#include <math.h>

#include <fluidsynth.h>

#include "fluid.h"

// en cas de reiteration cette fonction preserve les settings mais re-cree le synth
int fluid::init( int fsamp )
{
// Create the settings.
if	( settings == NULL )
	settings = new_fluid_settings();

// certains settings ne peuvent pas etre changes apres demarrage du synth
// il faut les fixer maintenant
fluid_settings_setnum(settings, "synth.sample-rate", (double)fsamp );
fluid_settings_setint(settings, "synth.verbose", 1 );
fluid_settings_setint(settings, "synth.cpu-cores", 4 );
fluid_settings_setnum(settings, "synth.gain", 1.0 );

/* Create the synthesizer. */
if	( synth )
	{
	delete_fluid_synth( synth );
	synth = NULL;
	}
synth = new_fluid_synth( settings );
if	( synth == NULL )
	return -1;
return 0;
}

// N.B. fluid supporte d'avoir plusieurs fonts en memoire
// pour le moment cette classe supporte une font seulement
int fluid::load_sf2()
{
// Load a SoundFont
sfont_id = fluid_synth_sfload( synth, sf2file, 1 );
if	( sfont_id < 0 )
	return sfont_id;
return 0;
}





/*


printf("font %d loaded\n", sfont_id );

printf("Rendering audio to %d-bit / %dHz WAV file\n", resol, freq );

printf("Internal buffer : %d\n", fluid_synth_get_internal_bufsize( synth ) );

{
int dhand;
wavpars s;
void * buf;
const char * wnam =  argv[2];
int period_size;	// en frames, la taille des blocs demandes au synthe
int total_size;		// en frames
int buf_size;		// en bytes
int writecnt;		// en bytes
period_size = 64;	// de 64 a 8192 dans les settings d'origine;

// on postule que c'est en stereo
if	( resol == 32 )
	buf_size = 2 * period_size * sizeof(float);
else	buf_size = 2 * period_size * sizeof(short);
buf = malloc(buf_size);
printf("alloc %d (per. size=%d)\n", buf_size, period_size );

if	(buf == NULL)
	gasp("Failed malloc\n");

dhand = open( wnam, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, 0666 );
if	( dhand == -1 )
	gasp("echec ouverture ecriture %s", wnam );

total_size = 0;
printf("audio file %s opened\n", wnam );

s.type = (resol==32)?(3):(1);
s.chan = 2;
s.freq = freq;
s.resol = resol;
// block et bpsec seront calcules par WAVwriteHeader
s.wavsize = 0;	// provisoire !! sera mis a jour a la fin du rendering
WAVwriteHeader( &s, dhand );

//                   midi channel, bank #
fluid_synth_bank_select( synth,	0, 0 );
fluid_synth_bank_select( synth,	9, 0 );
fluid_synth_program_change( synth, 0, patch );
// fluid_synth_noteon( synth, 0, 60, 127 );	// C4 sur piano
// fluid_synth_noteon( synth, 9, 37, 127 );	// 9 = drum kit, 37 = rim shot aka side stick

// boucle de render
double fpeak = 0.0; int ipeak = 0; int j;
for	( j = 0; j < 3200; ++j )	// j periodes de period_size samples
	{
	if	( ( j % 640 ) == 0 )
		fluid_synth_noteon( synth, 0, 37 + j/160, 127 );		// genre d'arpege
	if	( resol == 32 )
		{
		// demander dev->period_size frames au synthe, en stereo entrelacee 32 bits dans 1 seul buffer
		fluid_synth_write_float( synth, period_size, (float *)buf, 0, 2, (float *)buf, 1, 2 );
		if	( stat_flag )
			{
			int i;
			for	( i = 0; i < ( period_size * 2 ); ++i )
				if	( fabs(((float *)buf)[i]) > fpeak )
					fpeak = fabs(((float *)buf)[i]);
			}
		}
	else	{
		// demander dev->period_size frames au synthe, en stereo entrelacee 16 bits dans 1 seul buffer
		fluid_synth_write_s16( synth, period_size, (short *)buf, 0, 2, (short *)buf, 1, 2 );
		if	( stat_flag )
			{
			int i;
			for	( i = 0; i < ( period_size * 2 ); ++i )
				if	( abs(((short *)buf)[i]) > ipeak )
					ipeak = abs(((short *)buf)[i]);
			}
		}
	// copier dans fichier
	writecnt = write( dhand, buf, buf_size );
	if	( writecnt != buf_size )
		gasp("erreur ecriture disque (plein?)");
	total_size += period_size;	// N.B. period_size <==> buf_size
	}
if	( stat_flag )
	{
	if	( resol == 32 )
		printf("peak value %.3f\n", fpeak );
	else	printf("peak value %d (%.1f%%)\n", ipeak, (double)ipeak / 327.68 );
	}
// cloture du travail
lseek( dhand, 0, SEEK_SET );
s.wavsize = total_size;
// chucksize et filesize seront calcules par WAVwriteHeader
WAVwriteHeader( &s, dhand );
close( dhand );
printf("audio file closed, %d frames\n", total_size );
if	( buf != NULL )
	free( buf );
}

delete_fluid_synth(synth);
delete_fluid_settings(settings);

return 0;
}
*/


#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "layers.h"
#include "strips.h"
#include "gluplot.h"

#include "wav_head.h"
#include "fftw3.h"
#include "spectro.h"
#include "process.h"

/** ============================ AUDIO data processing =============== */


// allocation memoire et lecture WAV 16 bits entier en memoire
// donnees stockées dans l'objet process
int process::wave_process_1()
{
int retval;

retval = read_full_wav16( &wavp, wnam, &Lbuf, &Rbuf );
if	( retval )
	return retval;

// calcul de puissance main wav (canal L)
pww = wavp.freq / 24;	// pour une video a 24 im/s

double avgpow, minpow;
qpow = 1 + wavp.wavsize / pww;
Pbuf = (float *)malloc( qpow * sizeof(float) );
if	( Pbuf == NULL )
	gasp("echec malloc Pbuf %d samples", (int)qpow );

qpow = compute_power( Lbuf, wavp.wavsize, pww, Pbuf, &avgpow, &minpow );
printf("computed power (L channel) on %u windows : avg = %g, min = %g\n", qpow, avgpow, minpow );


/* ecretage de la puissance pour visualiser les silences
noise_floor = 5e6;
for	( unsigned int i = 0; i < qpow ; ++i )
	if	( Pbuf[i] > noise_floor )
		Pbuf[i] = noise_floor;
//*/

return 0;
}

int process::wave_process_2()
{
// renoising
double old_noise = Pbuf[0];
double noise;
unsigned int istart = 0;
for	( unsigned int i = 0; i < qpow ; ++i )
	{
	noise = Pbuf[i];
	if	( noise < noise_floor )
		{
		if	( old_noise == noise_floor )
			{
			istart = i;
			printf("begin silence @ %g\n", double( i * pww )/double(wavp.freq) );
			}
		}
	else 	if	( old_noise < noise_floor )
			printf("end   silence @ %g\t%g\n", double( i * pww )/double(wavp.freq),
						  double( (i-istart) * pww )/double(wavp.freq) );
	old_noise = noise;
	}
fflush(stdout);
return 0;
}

// la partie du process en relation avec jluplot

// layout pour les waveforms - doit etre fait apres lecture wav data
void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_s16_lod * curcour;

panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;

// creer le strip pour les waves
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "mono";
curbande->optX = 0;
curbande->optretX = 1;

// creer un layer
curcour = new layer_s16_lod;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le layer pour le canal L ou mono
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
curcour->set_n0( 0.0 );
curcour->label = string("Left");
curcour->fgcolor.dR = 0.75;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.0;

// creer le layer pour le canal R si stereo
if	( wavp.chan > 1 )
	{
	panneau->bandes[0]->Ylabel = "stereo";

	// creer le layer
	curcour = new layer_s16_lod;	// wave a pas uniforme
	curbande->courbes.push_back( curcour );
	// parentizer a cause des fonctions "set"
	panneau->parentize();

	// configurer le layer
	curcour->set_km( 1.0 );
	curcour->set_m0( 0.0 );
	curcour->set_kn( 32767.0 );	// amplitude normalisee a +-1
	curcour->set_n0( 0.0 );
	curcour->label = string("Right");
	curcour->fgcolor.dR = 0.0;
	curcour->fgcolor.dG = 0.75;
	curcour->fgcolor.dB = 0.0;
	}

// creer le strip pour les envelopes
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );

// configurer le strip
curbande->bgcolor.dR = 0.8;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 1.0;
curbande->Ylabel = "env";
curbande->optX = 1;
curbande->optretX = 1;

// creer un layer
layer_f * curcour2;
curcour2 = new layer_f;	// float a pas uniforme
curbande->courbes.push_back( curcour2 );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le layer pour le canal L ou mono
curcour2->set_km( 1.0 / double( pww ) );
curcour2->set_m0( 0.0 );
curcour2->set_kn( 1.0 );
curcour2->set_n0( 0.0 );
curcour2->label = string("Pow");
curcour2->fgcolor.dR = 0.0;
curcour2->fgcolor.dG = 0.0;
curcour2->fgcolor.dB = 0.75;
printf("end prep layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
}

// connexion du layout aux data
int process::connect_layout( gpanel * panneau )
{
int retval;
// pointeurs locaux sur les 2 ou 3 layers
layer_s16_lod * layL, * layR = NULL;
layer_f * layP;
// connecter les layers de ce layout sur les buffers existants
layL = (layer_s16_lod *)panneau->bandes[0]->courbes[0];
layL->V = Lbuf;
layL->qu = wavp.wavsize;
if	( wavp.chan > 1 )
	{
	layR = (layer_s16_lod *)panneau->bandes[0]->courbes[1];
	layR->V = Rbuf;
	layR->qu = wavp.wavsize;
	}
retval = layL->make_lods( 4, 4, 800 );
if	( retval )
	{ printf("echec make_lods err %d\n", retval ); return -6;  }
if	( wavp.chan > 1 )
	{
	retval = layR->make_lods( 4, 4, 800 );
	if	( retval )
		{ printf("echec make_lods err %d\n", retval ); return -7;  }
	}
// enveloppe
layP = (layer_f *)panneau->bandes[1]->courbes[0];
layP->V = Pbuf;
layP->qu = qpow;
layP->scan();

panneau->kq = (double)(wavp.freq);	// pour avoir une echelle en secondes au lieu de samples

printf("end connect layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
return 0;
}


// calculer la puissance en fonction du temps, par fenetres de ww samples
// la memoire doit etre allouee pour destbuf
// rend le nombre effectif de samples mis dans destbuf
unsigned int compute_power( short * srcbuf, unsigned int qsamp, unsigned int ww, float * destbuf, double * avg_pow, double * min_pow )
{
unsigned int is, id, j;
double val, sum; *avg_pow = 0.0; *min_pow = 32768.0 * 32768.0;

id = 0; j = 0; sum = 0.0;
for	( is = 0; is < qsamp ; ++is )
	{
	val = double(srcbuf[is]);
	sum += ( val * val ); ++j;
	if	( j >= ww )
		{
		sum /= double(ww);
		destbuf[id++] = float(sum);
		*avg_pow += sum;
		if	( sum < *min_pow )
			*min_pow = sum;
		j = 0; sum = 0.0;
		}
	}
*avg_pow /= double(id);
return id;
}


// lecture WAV entier en RAM, assure malloc 1 ou 2 buffers
int read_full_wav16( wavpars * wavpp, const char * wnam, short ** pLbuf, short ** pRbuf )
{
printf("ouverture %s en lecture\n", wnam );
wavpp->hand = open( wnam, O_RDONLY | O_BINARY );
if	( wavpp->hand == -1 )
	{ printf("file not found %s\n", wnam ); return -1;  }

WAVreadHeader( wavpp );
if	(
	( ( wavpp->chan != 1 ) && ( wavpp->chan != 2 ) ) ||
	( wavpp->resol != 16 )
	)
	{ printf("programme seulement pour fichiers 16 bits mono ou stereo\n"); close(wavpp->hand); return -2;  }
printf("freq = %u, block = %u, bpsec = %u, size = %u\n",
	wavpp->freq, wavpp->block, wavpp->bpsec, wavpp->wavsize );
fflush(stdout);

// allouer memoire pour lire WAV
short * Lbuf = NULL;
short * Rbuf = NULL;

Lbuf = (short *)malloc( wavpp->wavsize * sizeof(short) );	// pour 1 canal
if	( Lbuf == NULL )
	gasp("echec malloc Lbuf %d samples", (int)wavpp->wavsize );

if	( wavpp->chan > 1 )
	{
	Rbuf = (short *)malloc( wavpp->wavsize * sizeof(short) );	// pour 1 canal
	if	( Rbuf == NULL )
		gasp("echec malloc Rbuf %d samples", (int)wavpp->wavsize );
	}

// lecture bufferisee
unsigned int rdbytes, remsamples, rdsamples, rdsamples1c, totalsamples1c = 0;	// "1c" veut dire "pour 1 canal"
#define QRAW 4096
#define Qs16 sizeof(short int)
short int rawsamples16[QRAW];	// buffer pour read()
unsigned int i, j;
j = 0;
while	( totalsamples1c < wavpp->wavsize )
	{
	remsamples = ( wavpp->wavsize - totalsamples1c ) * wavpp->chan;
	if	( remsamples > QRAW )
		remsamples = QRAW;
	rdbytes = read( wavpp->hand, rawsamples16, remsamples*Qs16 );
	rdsamples = rdbytes / Qs16;
	if	( rdsamples != remsamples )
		{ printf("truncated WAV data"); close(wavpp->hand); return -4;  }
	rdsamples1c = rdsamples / wavpp->chan;
	totalsamples1c += rdsamples1c;
	if	( wavpp->chan == 2 )
		{
		for	( i = 0; i < rdsamples; i += 2)
			{
			Lbuf[j]   = rawsamples16[i];
			Rbuf[j++] = rawsamples16[i+1];
			}
		}
	else if	( wavpp->chan == 1 )
		{
		for	( i = 0; i < rdsamples; ++i )
			Lbuf[j++] = rawsamples16[i];
		}
	}
if	( totalsamples1c != wavpp->wavsize )	// cela ne peut pas arriver, cette verif serait parano
	{ printf("WAV size error %u vs %d", totalsamples1c , (int)wavpp->wavsize ); close(wavpp->hand); return -5;  }
close( wavpp->hand );
* pLbuf = Lbuf;
* pRbuf = Rbuf;
printf("lu %d samples Ok\n", totalsamples1c ); fflush(stdout);
return 0;
}

// ecriture WAV entier en RAM, wavpp doit contenir les parametres requis par WAVwriteHeader()
// c'est a dire chan, wavsize et freq
// ici resol sera force a 16 et type a 1
// :-( provisoire, devrait etre membre de wavpars
int write_full_wav16( wavpars * wavpp, const char * wnam, short * Lbuf, short * Rbuf )
{
wavpp->hand = open( wnam, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, 0666 );
if	( wavpp->hand == -1 )
	gasp("echec ouverture ecriture %s", wnam );
int resol = 16;
wavpp->type = (resol==32)?(3):(1);
wavpp->resol = resol;
// block et bpsec seront calcules par WAVwriteHeader
WAVwriteHeader( wavpp );

unsigned int bytecnt, writecnt=0;

bytecnt = wavpp->wavsize * wavpp->chan * ((resol==32)?(sizeof(float)):(sizeof(short)));

if	( wavpp->chan == 1 )
	{
	writecnt = write( wavpp->hand, Lbuf, bytecnt );
	if	( writecnt != bytecnt )
		gasp("erreur ecriture %s", wnam );
	}
else	{
	// il faut entrelacer les samples L et R ...
	// methode ridiculement laborieuse, evite creation d'un buffer intermediaire
	for	( unsigned int i = 0; i < wavpp->wavsize; ++i )
		{
		writecnt += write( wavpp->hand, &Lbuf[i], resol/8 );
		writecnt += write( wavpp->hand, &Rbuf[i], resol/8 );
		}
	if	( writecnt != bytecnt )
		gasp("erreur ecriture %s : %u vs %u", wnam, writecnt, bytecnt );
	}

close( wavpp->hand );
printf("ecrit %d samples Ok\n", wavpp->wavsize ); fflush(stdout);

return 0;
}


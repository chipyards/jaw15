
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
#include "tk17.h"
#include "param.h"
#include "process.h"

/** ============================ AUDIO data processing =============== */


// prmeiere phase du process, pour visualisation des courbes
// lecture WAV 16 bits entier en memoire
// calcul puissance
// (donnees stockées dans l'objet process)
int process::wave_process_1( tk17 * tk )
{
int retval;
letk = tk;	//pour le seconde phase

retval = read_full_wav16( &wavp, wnam, &Lbuf, &Rbuf );
if	( retval )
	return retval;

// calcul de puissance main wav (canal L)
pww = wavp.freq / 24;	// pour une video a 24 im/s

qpow = 1 + wavp.wavsize / pww;
Pbuf = (float *)malloc( qpow * sizeof(float) );
if	( Pbuf == NULL )
	gasp("echec malloc Pbuf %d samples", (int)qpow );

qpow = compute_power( Lbuf, wavp.wavsize, pww, Pbuf, &maxpow, &minpow );
printf("computed power (L channel) on %u windows : max = %g, min = %g\n", qpow, maxpow, minpow );

fflush(stdout);
return 0;
}

// seconde phase du process, differee pour permettre ajustement des slider
int process::wave_process_2()
{
// recuperation des sliders
letk->min_X = lipmin.get_value();
letk->max_X = lipmax.get_value();

unsigned int i, j;
// animation continue : economiseur de keyframes :
// si une sequence de plus de 2 keys consecutives satisfont la condition,
// seules la premiere et la derniere sont conservees
// les clef a omettre sont signalees par une valeur speciale (ici -1.0)
int ocnt = 0;
double Plim = sil16.get_value(); Plim *= Plim;	// sil16 au carre

for	( i = 0; i < qpow ; ++i )
	{
	if	( Pbuf[i] < Plim )	// condition <==> silence
		{
		++ocnt;
		}
	else	{
		if	( ocnt > 2 )	// on est a la fin d'une sequence
			{		// qui va de i-ocnt (inclus) a i (exclu)
			for	( j = i - ocnt + 1; j < i - 1; ++j )
				Pbuf[j] = -1.0;	// signalisation pour omission
			}
		ocnt = 0;
		}
	}
if	( ocnt > 2 )	// terminer une eventuelle sequence en cours
	{
	for	( j = i - ocnt + 1; j < i - 1; ++j )
		Pbuf[i] = -1.0;
	}

// copie vers lipXbuf[] avec mise a l'echelle
double k = letk->max_X / maxpow;
unsigned int ii = floor(intro.get_value());
lipXbuf[0] = letk->min_X;
for	( j = 1; j < ii; ++j )
	lipXbuf[j] = -0.01; // signalisation pour omission
j = ii;
for	( i = 0; i < qpow ; ++i )
	{
	if	( Pbuf[i] == -1.0 )
		lipXbuf[j] = -0.01; // signalisation pour omission
	else	lipXbuf[j] = letk->min_X + Pbuf[i] * k;
	++j;
	if	( j >= 1152 )
		{
		printf("Warning : sequence tronquee a 1152 frames\n");
		break;
		}
	}
for	( i = 0; i < ii ; ++i )		// outtro
	{
	lipXbuf[j++] = -0.01;
	if	( j >= 1152 )
		break;
	}
letk->qframes = j;
letk->X = lipXbuf;

// mise a jour display
((layer_f *)lepanneau->bandes[1]->courbes[0])->V = lipXbuf;
((layer_f *)lepanneau->bandes[1]->courbes[0])->qu = letk->qframes;
((layer_f *)lepanneau->bandes[1]->courbes[0])->scan();
printf("scan X : %d frames, [%g, %g]\n", letk->qframes,
	lepanneau->bandes[1]->courbes[0]->get_Vmin(),
	lepanneau->bandes[1]->courbes[0]->get_Vmax() );
//lepanneau->bandes[1]->fullN();
lepanneau->fullMN();
lepanneau->force_repaint = 1; lepanneau->force_redraw = 1;
generate_timetable( ii );
printf("\n*** PHASE 2 FINIE ***\n\n");
fflush(stdout);
return 0;
}

// 3eme phase du process, differee pour permettre revue des courbes
int process::wave_process_3()
{
if	(!( letk->src_fnam && letk->dst_fnam ))
	return -200;

// recuperation des sliders (6 sliders pour 8 params) (4 ont deja ete recuperes par wave_process_2())
letk->hips_angle = hips_angle.get_value();
letk->hips_period = neck_period.get_value();		// cheat!
letk->neck_angle = neck_angle.get_value();
letk->neck_period = neck_period.get_value();
letk->blink_period = blink_period.get_value();
letk->shoulders_angle = shoulders_angle.get_value();
letk->shoulders_period = blink_period.get_value();	// cheat!
letk->breath_breadth = breath_breadth.get_value();

int retval = letk->json_patch();
if	( retval )
	printf("json_patch : %d\n", retval );

printf("\n*** PHASE 3 FINIE ***\n\n");
fflush(stdout);
return retval;
}

// la partie du process en relation avec jluplot

// layout pour les waveforms - doit etre fait apres lecture wav data
void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_s16_lod * curcour;

lepanneau = panneau;	// pour reference ulterieure

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
curcour->set_kn( 1.0 );
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
	curcour->set_kn( 1.0 );
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

static void phase_2_call( GtkWidget *widget, process * pro )
{
pro->wave_process_2();
}

static void phase_3_call( GtkWidget *widget, process * pro )
{
pro->wave_process_3();
}

// les param_analog appartiennent au process, ils sont juste packes dans un
// conteneur de la fenetre param
void process::prep_layout_pa( param_view * para )
{
GtkWidget *curwidg, *hbut;
if	( para->wmain == NULL )
	para->build();

intro.tag = "duree intro (frames)";
intro.amin = 0.0;
intro.amax = 72.0;
intro.decimales = 0;
curwidg = intro.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
intro.set_value( 24.0 );

lipmin.tag = "jaw anim min";
lipmin.amin = 0.0;
lipmin.amax = 0.3;
lipmin.decimales = 2;
curwidg = lipmin.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
lipmin.set_value( 0.00 );

lipmax.tag = "jaw anim max";
lipmax.amin = 0.3;
lipmax.amax = 1.0;
lipmax.decimales = 2;
curwidg = lipmax.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
lipmax.set_value( 0.60 );

sil16.tag = "seuil silence (en sl16)";
sil16.amin = 0.0;
sil16.amax = 2000.0;
sil16.decimales = 0;
curwidg = sil16.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
sil16.set_value( 900.0 );

hips_angle.tag = 	"sigma angle random hips (degres)";
hips_angle.amin = 0.0;
hips_angle.amax = 20.0;
hips_angle.decimales = 1;
curwidg = hips_angle.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
hips_angle.set_value( 0.8 );

neck_angle.tag = 	"sigma angle random neck (degres)";
neck_angle.amin = 0.0;
neck_angle.amax = 20.0;
neck_angle.decimales = 1;
curwidg = neck_angle.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
neck_angle.set_value( 1.0 );

neck_period.tag = 	"periode moy random neck (frames)";
neck_period.amin = 0.0;
neck_period.amax = 100.0;
neck_period.decimales = 0;
curwidg = neck_period.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
neck_period.set_value( 19.0 );

shoulders_angle.tag = 	"sigma angle haussement epaules (degres)";
shoulders_angle.amin = 0.0;
shoulders_angle.amax = 20.0;
shoulders_angle.decimales = 1;
curwidg = shoulders_angle.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
shoulders_angle.set_value( 1.4 );

blink_period.tag = 	"periode moyenne eye blink";
blink_period.amin = 0.0;
blink_period.amax = 100.0;
blink_period.decimales = 0;
curwidg = blink_period.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
blink_period.set_value( 40.0 );

breath_breadth.tag = 	"amplitude inspiration";
breath_breadth.amin = 0.0;
breath_breadth.amax = 0.02;
breath_breadth.decimales = 4;
curwidg = breath_breadth.build();
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
breath_breadth.set_value( 0.005 );


/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX(para->vpro), curwidg, FALSE, FALSE, 0 );
hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Phase 2 : courbe jaw down ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( phase_2_call ), (gpointer)this );
gtk_box_pack_start( GTK_BOX( hbut ), curwidg, TRUE, TRUE, 0 );

/* simple bouton */
curwidg = gtk_button_new_with_label (" Phase 3 : json patch ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( phase_3_call ), (gpointer)this );
gtk_box_pack_start( GTK_BOX( hbut ), curwidg, TRUE, TRUE, 0 );

gtk_widget_show_all( hbut );
}

// calculer la puissance en fonction du temps, par fenetres de ww samples
// la memoire doit etre allouee pour destbuf
// rend le nombre effectif de samples mis dans destbuf
unsigned int compute_power( short * srcbuf, unsigned int qsamp, unsigned int ww, float * destbuf, double * max_pow, double * min_pow )
{
unsigned int is, id, j;
double val, sum;
// *avg_pow = 0.0;
*max_pow = 0.0;
*min_pow = 32768.0 * 32768.0;

id = 0; j = 0; sum = 0.0;
for	( is = 0; is < qsamp ; ++is )
	{
	val = double(srcbuf[is]);
	sum += ( val * val ); ++j;
	if	( j >= ww )
		{
		sum /= double(ww);
		destbuf[id++] = float(sum);
		// *avg_pow += sum;
		if	( sum < *min_pow )
			*min_pow = sum;
		if	( sum > *max_pow )
			*max_pow = sum;
		j = 0; sum = 0.0;
		}
	}
// *avg_pow /= double(id);
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


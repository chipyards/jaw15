/**
	Experiences sur le "filtre passe bas ideal" base sur le sinus cardinal
	et l'application au resampling a bande limitee (interpolation et decimation)

- theoreme : le filtre ideal est un filtre dont la reponse impulsionnelle finie est la fonction sinc( k * i )
  avec k = 2 * pi * (Fc/Fsamp)
       k = pi / pispan, avec pispan = ( Fsamp / 2 ) / Fc = "intervalle entre deux zeros de sinc"

- cas limite : pispan = 1 <==> Fc = Fsamp/2 = limite de Nyquist
  Un tel filtre ne sert a rien pour filtrer un signal incident, mais il a un sens pour faire de l'interpolation
  en limitant le bruit d'interpolation a la limite de Nyquist.
  En pratique on prendra toujours pispan > 1

- filtre imparfait : on se limite a une reponse impulsionnelle finie symetrique, avec eventuellement un fenetrage

EXPERIENCE 1 : FT de la reponse impulsionnelle finie = reponse frequentielle

- amplitude : on doit diviser le module de la transformee par pispan pour reponse DC a 0dB
  (l'integrale de sinc correspond a un rectangle de largeur pispan)

- frequence : dans le cas ideal la transition est attendue à l'abcisse a = fftsize / ( 2 * pispan ) 
  (en effet Fc = a * (Fsamp/fftsize) = Fsamp * 1 / ( 2 * pispan ) )
  On peut donc demander a jluplot une echelle en frequence relative à l'ideal

- incidence de la longueur de la reponse impulsionnelle qfir = 1 + pispan * qpis (@ pispan = 777) (sans fenetre)

	qpis		stopband 1st zero	niveau 1ere bosse
	5		1.24			0.106
	6		1.21			0.0751
	10		1.125			0.0803
	10.5		1.122			0.0897
	11		1.112			0.098
	20		1.0625			0.085
	30		1.0422			0.086
	31		1.0407			0.093
	40		1.0318			0.087
  conclusion : l'ecart entre Fc et le premier zero est inversement proportionnel a qpis.
  de plus la transition passe par Fc toujours pres de 0.5 (-6dB)
  la largeur totale de la transition (de 1.0 a 0) est 25% pour qpis=10, 6.3% pour qpis=40
  La premiere bosse depend des conditions aux bords, toujours pire avec qpis impair

- fenetrage  (@ pispan = 777)

	qpis   fenetre		stopband 1st zero	niveau 1ere bosse
	10	rect		1.125			-21.9
	10	hann		1.336			-44.0 dB
	10	hamming		1.346			-51.7
	10	blackman	1.56 @ -75 dB		-75.6
	10	b-harris	1.66 @ -75 dB		n.a.

	20	rect		1.063			-21.4
	20	hann		1.167			-44.0 dB
	20	hamming		1.175			-52.7
	20	blackman	1.28 @ -75 dB		-75.4
	20	b-harris	1.34 @ -75 dB		n.a.

	30	rect		1.042			-21.3
	30	hann		1.113			-44
	30	hamming		1.117			-53.1
	30	blackman	1.187 @ -75 dB		-75.3
	30	b-harris	1.228 @ -75 dB		n.a.

La transition intercepte toujours Fc à -6dB !
Le niveau DC n'est pas affecté par la fenetre, aucune correction n'est requise !
Le premier zéro est inexistant avec Blackman et Blackman-Harris, on le remplace par un seuil arbitraire
L'écart entre Fc et le premier zero est toujours inversement proportionnel a qpis,
mais il est tres augmente par la fenetre (presque triple avec Hamming).

- coefficients herites de github.com/libsndfile/libsamplerate (Mr Castro)

  On ajoute a l'experience deux type de fenetre, dont les reponses impulsionnelles sont calcules avec Octave
  (un outil style Matlab) par application de fenetre Kaiser sur sinc.
  On ne sait pas traduire cela en C, alors on teste deux filtres pre-calcules vus dans libsamplerate.
  Alors pispan est impose par Castro.
	qpis   filtre		stopband @ -75 dB    stopband @-100dB	pispan		inc	fudge factor
	32	fast		1.188			1.200		153.9375	128	1.2026
	84	mid_qual	1.082			1.090		534.2143	491	1.0880
  La transition intercepte toujours Fc à -6dB !
  La reponse est hyper "lisse" : pas d'ondulation dans la bande passante, ni dans la bande coupee
  La reponse est similaire à blackman @ qpis = 32 de 0 a -75 dB
  Notes sur les parametres :
	- Castro ne stocke qu'une demi-table qui commence au top du sinc (commun aux 2 moities donc)
	- Castro ne donne pas pispan, on le lit directement comme distance entre les zeros sur les data
	- Castro ne donne pas qpis mais cycles = qpis/4 (nombre de 2PI dans un demi-FIR)
	- Castro definit un increment < pispan, qui est l'increment utilise pour prendre les coeffs dans la table
	  avec coupure à Fsamp/2 au niveau voulu t.q. -100dB pour le fast plutot que -6dB qu'on aurait avec
	  increment = pispan, i.e. deplacer le filtre vers la gauche ("fudge factor"), en fonction de l'attenuation desiree.
	  On observe exactement -100dB @ 1.202 pour le fast, mieux que 120dB pour le mid_qual
	  Rappel : en 16 bits signes, le bruit de quantification est a -90.3dB
	  Castro fixe l'increment en premier, et utilise son script Octave pour determiner le fudge factor
	  (par approximations successives) et en deduire pispan pour calculer le sinc.
	- Castro pre-divise les coeffs par le fudge factor pour que l'amplitude soit Ok avec l'increment
	  (pour comparer avec les autres fenetres, nous remultiplions)

- filtrage passe-bande : on multiplie simplement la reponse impulsionnelle par une fonction cosinus
  pour translater la reponse frequentielle sur l'axe F de B * Fc.
  A cet effet la periode du cos est 1/B fois celle du sin inclus dans le sinc.
  La reponse est abaissee de 6dB, mais B < 1.0 la reponse presente un "bump" a 0dB
  Le programme corrige les -6dB pour B > 1.0

- arguments de la ligne de commande le l'experience 1 (avec les valeurs par defaut) :
	-L 20	log de fftsize
	-P 512	pispan = taille de PI en samples pour calcul sinc
	-Z 6 	nombre de zeros (doit etre pair)
	-w 0 	0 = rect, etc...
	-B 0.0	translation band_center * Fc
  N.B. les normalisations effectuees sur la reponse frequentielles neutralisent l'effet de fftsize et pispan (-L et -P)
  ces deux params influent sur la qualite du rendu.

  exemple: comparaison entre blackman et castro 'fast' @ qpis = 32
	./demo5 -P 153.9375 -Z 32 -w 3 
	./demo5 -Z 32 -w 8
  N.B. avec -w 8 (castro), pispan est fixe a 153.969, -P serait ignore

  exemple: passe-bande, reponse de 1.5 Fc a 3.5 Fc (la bande a une largeur 2 Fc)
	./demo5 -P 6 -Z 32 -w 3 -B 2.5

EXPERIENCE 2 : filtrage d'un signal arbiraire (fichier WAV)

- on utilise un signal echantillonne a fsamp, on le filtre avec la reponse impulsionnelle de taille qfir,
  on s'attend a une coupure Fc = fsamp/(2*pispan)
  exemple fsamp = 44100, pispan = 154 ==> fc = 143 Hz
	  fsamp = 44100, pispan = 50.1136 ==> fc = 440 Hz
- a chaque extremite du signal, les qfir/2 echantillons manquants sont remplaces par des zeros 

- arguments de la ligne de commande 
	-o ""	fichier de sortie sauve
	-c 1	nombre de canaux sauves : 2 -> stereo {src-filtered}, 1 -> mono filtered
		(N.B. si le fichier d'entree est stereo, seul canal L est traite)
  exemple : passe-bande  [2205 Hz 5145 Hz] largeur 2940 Hz, resultat dans fichier mono 
	./demo5 -Z 32 -P 15 -w 3 -B 2.5 ../../JAW/WAV/logipoS.wav -o pipo.wav
  N.B. accepte fichier 16 bits ou 32 bits, sauve idem

*/
#include <gdk/gdkkeysyms.h>  
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <fftw3.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "gluplot.h"

#include "layer_u.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "../autobuf.h"
#include "../wavio.h"	// il inclut audiofile.h lui-meme
#include "fir.h"
#include "demo5.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;
static fir lefir;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{

// moderateur de drawing
// N.B. les pages du notebook sont toujours toutes "visibles" au sens de gtk_widget_get_visible()
// les pages cachees sont non realized au sens de gtk_widget_get_realized() jusqu'a etre affichees
// une premiere fois. Ceci est detecte par GDK_IS_DRAWABLE(larea->window) dans gluplot.
// Le test gtk_notebook_get_current_page() est la meilleure maniere de moderer l'activite. 
if	( glo->panneau1.force_repaint )
	glo->panneau1.paint();

if	( glo->panneau2.force_repaint )
	glo->panneau2.paint();

return( -1 );
}

gint close_event_call( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_main_quit();
return (TRUE);		// ne pas destroyer tout de suite
}

void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_main_quit();
}

// callbacks de widgets
void run_call( GtkWidget *widget, glostru * glo )
{
}

void clear_call( GtkWidget *widget, glostru * glo )
{
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back1( double M, double N, void * vglo )
{
printf("clic M N %g %g\n", M, N );
// glostru * glo = (glostru *)vglo;
}

void key_call_back1( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// la visibilite
	case GDK_KEY_KP_0 :
	case '0' : glo->panneau1.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau1.toggle_vis( 0, 1 ); break;
	// l'option offscreen drawpad
	case 'o' : glo->panneau1.offscreen_flag = 1; break;
	case 'n' : glo->panneau1.offscreen_flag = 0; break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau1.dump(); fflush(stdout);
		break;
	/* demo modpop3
	case 'k' :
		{
		char tbuf[32];
		snprintf( tbuf, sizeof(tbuf), "%g", glo->k );
		modpop_entry( "K setting", "rapport des frequences", tbuf, sizeof(tbuf), GTK_WINDOW(glo->wmain) );
		glo->k = strtod( tbuf, NULL );
		} break; */
	//
	case 't' :
		glo->panneau1.bandes[0]->subtk *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo5.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot FIFO" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
	}
}

void key_call_back2( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau2.dump(); fflush(stdout);
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo2.2.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot XY" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau2.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ l'application ==================== */

// calcul FFT pour visu reponse frequentielle
int glostru::fft_on_FIR( unsigned int firsize, double * firbuf )
{
printf("FFT size %u\n", qFFT );
// allocation pour FFT
if	( qFFT < firsize )	{ printf("sorry fftsize < FIR\n"); return -5; }
if	( FFTin == NULL )
	FFTin = (double *)fftw_malloc( qFFT * sizeof(double) );
if	( FFTout == NULL )
	FFTout = (double *)fftw_malloc( qFFT * sizeof(double) );
if	( ( FFTin == NULL ) || ( FFTout == NULL ) )
	{ printf("malloc failed\n"); return -1; }
// preparer le plan de travail de fftw3
if	( plan )
	fftw_destroy_plan(plan);
plan = fftw_plan_dft_r2c_1d( qFFT, FFTin, (fftw_complex*)FFTout, FFTW_ESTIMATE );
if	( plan == NULL )
	{ printf("fftw plan failed\n"); return -3; }

// copier reponse impulsionnelle
for	( unsigned int i = 0; i < firsize; ++i )
	FFTin[i] = firbuf[i];
// completer avec beaucoup de zeros pour une bonne resolution FFT 
for	( unsigned int i = firsize; i < qFFT; ++i )
	FFTin[i] = 0;

// execution FFT
fftw_execute( plan );

// calcul magnitudes sur place (FFTout contient des valeurs complexes)
unsigned int a = 0; double k;
// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
if	( lefir.window_type < 8 )
	k = 1.0 / lefir.pispan;
else	k = 1.0 / lefir.castro_inc;	// Castro a corrige la valeur centrale du sinc pour matcher son "increment"
if	( lefir.rB > 0.0 )
	k *= 2;	// bandes gauche et droite ne se recouvrent plus, on perd 6dB !
for	( unsigned int j = 0; j <= qFFT/2; ++j )
	{
	FFTout[j] = k * hypot( FFTout[a], FFTout[a+1] ); // magnitude (conversion en dB sera faite par layer_u)
	a += 2;
	}
return 0;
}

int glostru::audiofile_load( int verbose )
{
int retval = wavp.read_head( ifnam, verbose ); 
if	( retval )
	{
	if	( retval == -1 )
		printf("file not found %s\n", ifnam );
	else	printf("erreur wavio::read_head %d\n", retval );
	fflush(stdout); return retval;
	}
if	( verbose )
	{
	printf("got %d channels @ %d Hz, monosamplesize %d\n",
		wavp.qchan, wavp.fsamp, wavp.monosamplesize );
	printf("estimated length : %u PCM frames\n", (unsigned int)wavp.estpfr );
	fflush(stdout);
	}
// pre-allocation (facultative) des buffers pour l'audio entier
if	( wavp.estpfr > Wbuf.capa )
	{
	Wbuf.reset();	// pour eviter realloc, qui serait inefficace ici
	if	( Wbuf.more( wavp.estpfr ) )
		gasp("echec alloc Wbuf %d samples", (int)wavp.estpfr );
	}

// lecture via un petit buffer, pour extraction mono et/ou conversion float
#define QRAW 2048	// optimal pour WAV, en PCM frames
#define QMORE	(1<<21)	// quantum pour reallocation ( 1 page = 4M = (1<<21)shorts )
int i;
unsigned int j = 0; wavp.realpfr = 0;
if	( wavp.monosamplesize == 2 )
	{							// cas du sl16
	short pcmbuf[QRAW*2];	// supporte stereo
	do	{
		retval = wavp.read_data_p( (void *)pcmbuf, QRAW );
		if	( retval > 0 )
			{
			if	( wavp.realpfr > Wbuf.capa )
				{
				if	( Wbuf.more( QMORE ) )
					gasp("echec autobuf::more()");
				printf("realloc Wbuf\n"); fflush(stdout);
				}
			for	( i = 0; i < ( retval * (int)wavp.qchan ); i += (int)wavp.qchan )
				{	// on prend seulement le canal L si stereo
				Wbuf.data[j++] = float( (double)pcmbuf[i] / 32768.0 );
				}
			}
		} while ( retval > 0 );
	Wbuf.size = wavp.realpfr;
	}
else if	( wavp.monosamplesize == 4 )
	{							// cas du float32
	float pcmbuf[QRAW*2];	// supporte stereo
	do	{
		retval = wavp.read_data_p( (void *)pcmbuf, QRAW );
		if	( retval > 0 )
			{
			if	( wavp.realpfr > Wbuf.capa )
				{
				if	( Wbuf.more( QMORE ) )
					gasp("echec autobuf::more()");
				printf("realloc Wbuf\n"); fflush(stdout);
				}
			for	( i = 0; i < ( retval * (int)wavp.qchan ); i += (int)wavp.qchan )
				{	// on prend seulement le canal L si stereo
				Wbuf.data[j++] = pcmbuf[i];
				}
			}
		} while ( retval > 0 );
	Wbuf.size = wavp.realpfr;
	}
else	gasp("unsupported format %d bytes", wavp.monosamplesize );

if	( j != wavp.realpfr )	// cela ne peut pas arriver, cette verif est parano
	gasp("erreur JAW #213978");
printf("decoded PCM frames %u vs %u estimated\n", wavp.realpfr, wavp.estpfr );
wavp.afclose();
fflush(stdout);
return 0;
}


int glostru::audiofile_process()
{
double Fc = double(wavp.fsamp)/(2.0*lefir.pispan);
printf("Fc @ -6dB : %g Hz\n", Fc );
//if	( lefir.rB > 0.0 )
//	printf("bande [%g %g] largeur %g\n", Fc * ( lefir.band_center - 1.0 ), Fc * ( lefir.band_center + 1.0 ), Fc * 2.0 );
fflush(stdout);
// allocation buffer pour l'audio entier
if	( wavp.realpfr > Ybuf.capa )
	{
	Ybuf.reset();	// pour eviter realloc, qui serait inefficace ici
	if	( Ybuf.more( wavp.realpfr ) )
		gasp("echec alloc Ybuf %d samples", (int)wavp.realpfr );
	}
int i, j, j0, k;
double sum, K;
if	( lefir.window_type < 8 )
	K = 1.0 / lefir.pispan;
else	K = 1.0 / lefir.castro_inc;	// Castro a corrige la valeur centrale du sinc pour matcher son "increment"
if	( lefir.rB > 0.0 )
	K *= 2;	// bandes gauche et droite ne se recouvrent plus, on perd 6dB !

j0 = (lefir.qfir-1)/ 2;	// qfir est impair
for	( i = 0; i < (int)wavp.realpfr; ++i )
	{
	sum = 0.0;
	for	( j = 0; j < (int)lefir.qfir; ++j )
		{
		k = i + j - j0 ;
		if	( ( k >= 0 ) && ( k < (int)wavp.realpfr ) )
			sum += Wbuf.data[k] * lefir.FIRbuf[j];
		}
	Ybuf.data[i] = float( sum * K );
	}
Ybuf.size = wavp.realpfr;
return 0;
}

int glostru::audiofile_save( int monosamplesize, int qchan )
{
if	( ( ofnam == NULL ) || ( Ybuf.size == 0 ) || ( Ybuf.size != Wbuf.size ) )
	return 0;
int retval;
unsigned int qpfr, i, j;
wavio neww;

neww.qchan   = qchan;
neww.fsamp   = wavp.fsamp;
neww.realpfr = Ybuf.size;
neww.monosamplesize = monosamplesize;
neww.type = (monosamplesize==4)?3:1;

retval = neww.write_head( ofnam );	// remet realpfr a zero apres l'avoir ecrit
if	( retval )
	return retval;

neww.realpfr = 0; j = 0;
if	( neww.monosamplesize == 2 )
	{							// cas du sl16
	short pcmbuf[QRAW*2];	// supporte stereo
	while	( neww.realpfr < Ybuf.size )
		{
		qpfr = Ybuf.size - neww.realpfr;
		if	( qpfr > QRAW )
			qpfr = QRAW;
		if	( neww.qchan == 2 )
			{
			for	( i = 0; i < (qpfr*2); i += 2 )
				{
				pcmbuf[i]   = short(round(32767.0 * Wbuf.data[j]));
				pcmbuf[i+1] = short(round(32767.0 * Ybuf.data[j++]));
				}
			}
		else	{
			for	( i = 0; i < qpfr; i++ )
				{
				pcmbuf[i]   = short(round(32767.0 * Ybuf.data[j++]));
				}
			}
		retval = neww.write_data_p( pcmbuf, qpfr );
		if	( retval < 0 )
			return retval;
		}
	}
else if	( neww.monosamplesize == 4 )
	{							// cas du float32
	float pcmbuf[QRAW*2];	// supporte stereo
	while	( neww.realpfr < Ybuf.size )
		{
		qpfr = Ybuf.size - neww.realpfr;
		if	( qpfr > QRAW )
			qpfr = QRAW;
		if	( neww.qchan == 2 )
			{
			for	( i = 0; i < (qpfr*2); i += 2 )
				{
				pcmbuf[i]   = Wbuf.data[j];
				pcmbuf[i+1] = Ybuf.data[j++];
				}
			}
		else	{
			for	( i = 0; i < qpfr; i++ )
				{
				pcmbuf[i]   = Ybuf.data[j++];
				}
			}
		retval = neww.write_data_p( pcmbuf, qpfr );
		if	( retval < 0 )
			return retval;
		}
	}
else	gasp("unsupported format %d bytes", neww.monosamplesize );
neww.afclose();
printf("finished writing WAV %s, %d bits, %d ch.\n", ofnam, neww.monosamplesize * 8, neww.qchan ); fflush(stdout);
return 0;
}

// layout pour reponse impulsionnelle
void glostru::layout1()
{
panneau1.offscreen_flag = 0;

// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );
if	( lefir.classic )	// mettre le sommet du sinc a l'abcisse 0 
	panneau1.q0 = - double( (lefir.qfir-1) / 2 );	// l'abcisse 0 au sommet du sinc 
else	panneau1.q0 = - double( lefir.cnt_left + (lefir.A0/lefir.dA) ); 
// configurer le strip
curbande->bgcolor.set( 0.92, 0.98, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 1;

// creer un layer
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "fir" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour->V = lefir.FIRbuf;
curcour->qu = lefir.qfir;
curcour->scan();	// alors on peut faire un scan

if	( lefir.FENbuf == NULL )
	return;

// creer un layer
curcour = new layer_u<double>;
curbande->add_layer( curcour, "win" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.5, 0.3 );

// connexion layout - data
curcour->V = lefir.FENbuf;
curcour->qu = lefir.qfir;
curcour->scan();	// alors on peut faire un scan
}

// layout pour WAV
void glostru::layout1W()
{
panneau1.offscreen_flag = 0;

// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 1;

// creer un layer
layer_u<float> * curcour;
curcour = new layer_u<float>;
curbande->add_layer( curcour, "src" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour->V = Wbuf.data;
curcour->qu = Wbuf.size;
curcour->scan();	// alors on peut faire un scan

curcour = new layer_u<float>;
curbande->add_layer( curcour, "resu" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
curcour->V = Ybuf.data;
curcour->qu = Ybuf.size;
curcour->scan();	// alors on peut faire un scan

}


void glostru::layout2()
{
// layout jluplot pour panneau2 : reponse frequentielle = resultat FFT
panneau2.offscreen_flag = 0;
// normalisation d'echelle horizontale t.q. Fc theorique <==> 1.0
panneau2.kq = qFFT / ( 2 * lefir.pispan );
 
// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau2.add_strip( curbande );

// configurer le strip
curbande->Ylabel = "val";
curbande->optX = 1;

// creer un layer
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "real" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );
curcour->style = 2;			// echelle verticale en dB
// connexion layout - data
curcour->V = FFTout;
// normalisation d'echelle horizontale affichee, t.q. Fc theorique <==> 1.0
panneau2.kq = qFFT / ( 2 * lefir.pispan );
// limitation d'etendue horizontale via qu, notamment pour le full zoom horizontal (fullM)
double limit;				// limite en unites affichees (Fc <==> 1.0) 
limit = lefir.pispan;				// pour que le fullM (horiz.) se limite a Fsamp/2 (pispan = Fsamp/2 / Fc) 
if	( limit > 8.0 + lefir.rB )
	limit = 8.0 + lefir.rB;	// pour que le fullM (horiz.) se limite a 8 * Fc
// appliquer limit a qu
curcour->qu = floor( panneau2.kq * limit ); 
// precaution
if	( (unsigned int)curcour->qu > qFFT )
	curcour->qu = qFFT;
curcour->Vfloor = 1e-6;		// pour plancher a -120dB au lieu de -100dB
curcour->scan();		// alors on peut faire un scan
}


/** ============================ main, quoi ======================= */

void usage()
{
printf("// Usage //\n"
 " -L log de fftsize\n"
 " -P pispan = taille de PI en samples pour calcul RI\n"
 " -Z qpis = taille de RI en PIs\n"
 " -w fenetre 0 = rect, etc...\n"
"Radian:\n"
 " -a A0 decalage du centre de la RI (rd/samp)\n"
 " -d dA increment angulaire(rd/samp)\n"
"Relatif:\n"
 " -b rB pass band : translation band_center (relatif dA)\n"
 " -A decalage de la RI (relatif dA)\n"
 " -F frequ de coupure ou min, rel. Nyquist (Fsamp/2)\n"
 " -G frequ max, rel. Nyquist (Fsamp/2)(incompat. -P, -f)\n"
"Hertz:\n"
 " -r Fsamp en Hz\n"
 " -f frequence de coupure ou min en Hz\n"
 " -g frequence max bande en Hz (incompat. -P, -F)\n"
"Filtrage WAV:\n"
 " -o output file\n"
 " -c channels in saved file\n"
 " <input file> (sinon, seulement FFT)\n"
 "NOTE: -P, -F et -f sont incompatibles, et -P force le mode \"classic\"\n"
 "      -b, -g, -G transforment passe-bas en passe-bande\n"
 );
}

int main( int argc, char *argv[] )
{
glostru * glo = &theglo;
GtkWidget *curwidg;

gtk_init(&argc,&argv);
setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "JAW");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

// creer boite verticale pour : en haut plot panels, en bas boutons et entries
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

// creer boite verticale pour panel superieur et sa zoombar
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
// gtk_paned_pack1( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->vpan1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, VIEW_W, 200 );	// hauteur mini, la hauteur initiale fixee par parent
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, TRUE, TRUE, 0 );
glo->darea1 = curwidg;

/* creer une drawing area pour la zoombar */
curwidg = gtk_drawing_area_new();
glo->zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, FALSE, FALSE, 0 );
glo->zarea1 = curwidg;

/* creer une drawing area pour panel inferieur */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, VIEW_W, VIEW_H / 2 );	// hauteur mini, la hauteur initiale fixee par parent
// gtk_paned_pack2( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->darea2 = curwidg;
// option paned
	{		// paire verticale "paned"
	curwidg = gtk_vpaned_new ();
	gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
	// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
	gtk_widget_set_size_request( curwidg, VIEW_W, VIEW_H );
	glo->vpans = curwidg;
	// y placer les deux panels
	gtk_paned_pack1( GTK_PANED(glo->vpans), glo->vpan1, TRUE, FALSE ); // resizable, not shrinkable
	gtk_paned_pack2( GTK_PANED(glo->vpans), glo->darea2, TRUE, FALSE ); // resizable, not shrinkable
	}

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* texte descriptif */
curwidg = gtk_entry_new();
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_max_length( GTK_ENTRY(curwidg), 128 );
gtk_widget_set_size_request( curwidg, VIEW_W, -1 );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 5 );
glo->edesc = curwidg;

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;
// connecter les callbacks events --> appli
glo->panneau1.clic_callback_register( clic_call_back1, (void *)glo );
glo->panneau1.key_callback_register( key_call_back1, (void *)glo );
glo->panneau2.key_callback_register( key_call_back2, (void *)glo );

glo->panneau1.events_connect( GTK_DRAWING_AREA( glo->darea1 ) );
glo->panneau2.events_connect( GTK_DRAWING_AREA( glo->darea2 ) );

// traiter arguments
if	( argc < 2 )
	{ usage(); return 0; }
cli_parse * lepar = new cli_parse( argc, (const char **)argv, "LPZwadbAFGrfgoc" );
const char * val;
int qFFTlog = 20;
unsigned int saved_qchan = 1;
double relA0 = 0.0;
double F0_rny = 0.0;
double F1_rny = 0.0;
glo->Fsamp = 44100;
double F0_Hz  = 0.0;
double F1_Hz  = 0.0;

if	( ( val = lepar->get( 'L' ) ) )	qFFTlog = atoi( val );			// log de fftsize
if	( ( val = lepar->get( 'P' ) ) )	{
					lefir.pispan = strtod( val, NULL );	// taille de PI en samples pour calcul sinc
					lefir.classic = 1;
					}
if	( ( val = lepar->get( 'Z' ) ) )	lefir.qpis = atoi( val );		// nombre de zeros
if	( ( val = lepar->get( 'w' ) ) )	lefir.window_type = atoi( val );	// 0 = rect, etc...
if	( ( val = lepar->get( 'a' ) ) )	lefir.A0 = strtod( val, NULL );		// A0 decalage du centre de la RI (rd)
if	( ( val = lepar->get( 'd' ) ) )	lefir.dA = strtod( val, NULL );		// dA increment angulaire (rd/samp)
if	( ( val = lepar->get( 'b' ) ) )	lefir.rB = strtod( val, NULL );		// rB translation band_center (relatif dA)

if	( ( val = lepar->get( 'A' ) ) )	relA0 = strtod( val, NULL );		// decalage de la RI (relatif dA)
if	( ( val = lepar->get( 'F' ) ) )	F0_rny = strtod( val, NULL );		// frequ de coupure ou min, rel. Nyquist (Fsamp/2)
if	( ( val = lepar->get( 'G' ) ) )	F1_rny = strtod( val, NULL );		// frequ max, rel. Nyquist (Fsamp/2)

if	( ( val = lepar->get( 'r' ) ) )	glo->Fsamp  = atoi( val );		// Fsamp en Hz
if	( ( val = lepar->get( 'f' ) ) )	F0_Hz  = strtod( val, NULL );		// frequence de coupure ou min (Hz)
if	( ( val = lepar->get( 'g' ) ) )	F1_Hz  = strtod( val, NULL );		// frequence max (band) (Hz)

if	( ( val = lepar->get( 'o' ) ) )	glo->ofnam = val;			// output file
if	( ( val = lepar->get( 'c' ) ) )	saved_qchan = atoi( val );		// channels in saved file

glo->ifnam = lepar->get( '@' );		// naked string = input file

if	( ( qFFTlog < 8 ) || ( lefir.pispan < 1.0 ) || ( lefir.qpis < 4 ) || ( lefir.qpis & 1 ) || ( saved_qchan > 2 ) )
	{ printf("invalid argument\n"); return -1; }
glo->qFFT = 1 << qFFTlog;

if	( ( F0_rny > 0.0 ) || ( F1_rny > 0.0 ) )
	{
	if	( F1_rny == 0.0 )
		{			// low-pass
		lefir.dA = M_PI * F0_rny;
		}
	else	{			// band-pass
		lefir.dA = M_PI * 0.5 * (F1_rny-F0_rny);
		lefir.rB = M_PI * 0.5 * (F1_rny+F0_rny) / lefir.dA;
		}
	}

if	( ( ( F0_Hz > 0.0 ) || ( F1_Hz > 0.0 ) ) && ( glo->Fsamp > 0 ) )
	{
	if	( F1_Hz == 0.0 )
		{			// low-pass
		lefir.dA = 2.0 * M_PI * F0_Hz / (double)glo->Fsamp;
		}
	else	{			// band-pass
		lefir.dA = M_PI * (F1_Hz-F0_Hz) / (double)glo->Fsamp; 
		lefir.rB = M_PI * (F1_Hz+F0_Hz) / ( (double)glo->Fsamp * lefir.dA ); 
		}
	}
if	( relA0 != 0.0 )
	lefir.A0 = relA0 * lefir.dA;
// s'assurer que dA et pispan sont tous les deux definis
if	( lefir.dA == 0.0 )
	lefir.dA = M_PI / lefir.pispan;
else	lefir.pispan = M_PI / lefir.dA;

// generer FIR
int retval = lefir.generate();
if	( retval )
	gasp(" erreur %d", retval );
gtk_entry_set_text( GTK_ENTRY( glo->edesc ), lefir.description );

// FFT pour reponse freqentielle
retval = glo->fft_on_FIR( lefir.qfir, lefir.FIRbuf );
if	( retval )
	gasp(" erreur %d", retval );

// filtrage audio
if	( glo->ifnam )
	{
	printf("fichier a traiter: %s\n", glo->ifnam ); fflush(stdout);
	retval = glo->audiofile_load( 1 );
	if	( retval )
		glo->ifnam = NULL;	// abandon lecture fichier
	}

if	( glo->ifnam )
	{
	glo->audiofile_process();
	glo->layout1W();
	if	( glo->ofnam )
		glo->audiofile_save( glo->wavp.monosamplesize, saved_qchan );
	}
else	{
	glo->layout1();
	}

gtk_widget_show_all( glo->wmain );

glo->layout2();

glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

fflush(stdout);

gtk_main();

g_source_remove( glo->idle_id );

return(0);
}


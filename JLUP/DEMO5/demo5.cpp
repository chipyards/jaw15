/**
	Experiences sur le "filtre passe bas ideal" base sur le sinus cardinal
	et l'application au resampling a bande limitee (interpolation et decimation)

Ce programme rend les services suivants :

1) calcul des coeffs d'un filtre FIR base sur le sinus cardinal et une fenetre choisie parmi
   plusieurs formules classiques, visualisation de la fenetre et de la réponse impulsionnelle fenetree
	- ce calcul peut appliquer un offset A0 (nul par defaut), dans le but d'évaluer l'impact (idéalement nul)
	  de cet offset sur la réponse du filtre
	- il supprime chaque coeff extreme si nul
   le calcul est disponible en 2 modes :
	- ANALYTIC : les calculs utilisent les fonctions trigo standard
	- INTERPOL : les coeffs calculés analytiquement sont stockés en grand nombre dans une demi-table,
	  puis la RI demandée est calculée en extrayant les coeffs de la table avec interpolation.
	  Le but de ce mode est d'évaluer une opportunité d'accélération du filtrage et du resampling.
	  Ce mode est activé avec l'option -i qui introduit la taille (arbitraire) de la demi-table.
	  Une évaluation de l'erreur d'interpolation (pire cas) est affichée dans le terminal
	  Une variante du mode INTERPOL est le mode CASTROL, qui au lieu de générer une table utilise verbatim
	  une parmi deux tables de libsamplerate de Erik de Castro Lopo, alors qpis est imposé (32 ou 84).
	  Cette variante est activée pour les fenêtres 8 et 9.

2) calcul et affichage de la réponse frequentielle de ce filtre obtenue par FFT
   l'axe de fréquence offre 3 options de graduation :
	- fréquence normalisée Fn relative à la fréquence de Nyquist Fsamp/2,  de 0 a 1 (option -N)
	- fréquence relative à la fréquence Fc de coupure à -6dB, de 0 a pispan (option -6)
	- fréquence en Hz, pour la frequ. d'échantillonnage courante (defaut 44100Hz) (option -H)
   Il y a aussi 3 options de zoom : -2, -4, -8 pour un largeur de 2, 4 ou 8 fois Fc 
   Ces 6 options sont aussi accessibles a chaud avec les touches N, H, 6, 2, 4, 8

3) calculs annexes :
	- réponse DC en fonction de A0 : la fluctuation de cette réponse est un effet parasite distinct des
	  non idéalités visibles sur la réponse fréquentielle, et se traduit par des signaux parasites présents
	  sur les signaux de basse fréquence ou DC non nul (n'impacte que le resampling)
	  La courbe est dispo dans le panel superieur (cachée par defaut), l'amplitude de la fluctuation est
	  affichée dans le terminal

4) filtrage de fichier WAV :
	- utilise les coeffs du 1), en mode ANALYTIC ou INTERPOL
	- activé si présence d'un nom de fichier source valide
	- alors les waveforms in et out sont affichées dans le panel sup à la place de la RI
	- le résultat est sauvé sur disk en f32 si un nom de fichier de sortie est fourni

5) resampling de fichier WAV
	- en mode ANALYTIC, n'utilise pas les coeffs du 1), mais les recalcule a chaud (car A0 varie)
	- en mode INTERPOL, utilise la table du 1), mais interpole a chaud
	- activé si présence d'un nom de fichier source valide et de l'option -K suivie de la valeur de Kr
	  Kr = Fs / Fd, tel que si Kr > 1, on a decimation, ou augmentation de la frequence apparente
	  si le fichier est joué à la meme Fsamp que l'original
	- alors les waveforms in et out sont affichées dans le panel sup à la place de la RI
	- le résultat est sauvé sur disk en f32 si un nom de fichier de sortie est fourni

6) filtrage d'image PNG :
	- 2 passes de filtrage 1D, horizontale (ligne par ligne), puis verticale (colonne par colonne)
	- le filtrage 1D est semblable a celui du WAV, sauf le traitement des bords (duplication au lieu de zero)
	- utilise les coeffs du 1), en mode ANALYTIC ou INTERPOL
	- activé si -c 3 et présence d'un nom de fichier source valide
	- accepte image monochrome ou RGB 
	- le résultat est sauvé sur disk en PNG RBG si un nom de fichier de sortie est fourni

7) resampling d'image PNG :
	- en mode ANALYTIC, n'utilise pas les coeffs du 1), mais les recalcule a chaud (car A0 varie)
	- en mode INTERPOL, utilise la table du 1), mais interpole a chaud
	- activé si -c 3, option -K suivie de la valeur de Kr, et présence d'un nom de fichier source valide
	- 2 passes de resampling 1D, horizontale (ligne par ligne), puis verticale (colonne par colonne)
	- le filtrage 1D est semblable a celui du WAV, sauf le traitement des bords (duplication au lieu de zero)
	- accepte image monochrome ou RGB
	- les valeurs de pixel in et out sont affichées dans le panel sup à la place de la RI, en fonction de X
	  pour la ligne Y = H/2 de l'image originale, composante B, apres resampling horizontal
	  (avant resampling vertical, ecretage [0-255] et quantification 8 bits)
	- le résultat est sauvé sur disk en PNG RBG si un nom de fichier de sortie est fourni

- Note sur la resolution :
	- les RI sont stockees en double
	- la FFT est effectuee en double
	- la résolution fréquentielle de la FFT est configurable indépendamment (1048576 points par defaut)
	- filtrage et resampling wav : chaque sample est calcule en double puis stocke en float
- Note sur traitement audiofile :
	- la classe wavio (wavio.h, wavio.cpp) assure lecture et ecriture du header WAV, et des data brutes
	  cette classe est derivee de la classe audiofile qui est juste une interface (tout y est virtuel)
	  sont supportes s16le et f32 (pas de 24 bits) 
	- wavio lit et ecrit les data par paquets de taille libre, la bufferisation est a charge de l'appli
	- les conversions s16le <-> f32 sont a charge de l'appli :
	  demo5 convertit tout en f32 a la lecture, et ne retient que le canal L.
	- demo5 stocke le fichier entier dans 2 buffers f32 Wbuf (src) et Ybuf (filtered) de la classe autobuf (autobuf.h)
	- demo5 sauve le resultat toujours en f32 (cependant audiofile_save() saurait convertir f32 en s16le si on voulait)

- notations
		qfir	= nombre de coeffs
		qpis	= nombre d'intervalles d'angle PI (qui est justement le nombre de zeros car il en manque un au milieu)
		pispan	= demi-periode du sinc exprimee en Tsamp = Fsamp/2Fc	(s'agissant de Fc à 6dB)
			= distance en samples entre deux zeros de sinc (peut etre fractionnaire, alors les zeros sont virtuels)
		dA	= intervalle (en radian) entre les coeffs prélevés dans sinc(A)
		A0	= offset (en radian)
		Fc	= frequence de coupure à -6dB
		Fcn	= frequence de coupure normalisée (relative Nyquist) = Fc/(Fsamp/2)
- formulaire
		dA	= 2PI * Fc / Fsamp	= PI/pispan		= PI * Fcn
		pispan	= (Fsamp/2) / Fc	= PI/dA 		= 1/Fcn
		Fc	= (Fsamp/2)/pispan	= (dA/2PI)/Fsamp	= Fcn * (Fsamp/2)
		Fcn	= 1/pispan		= dA/PI 		= Fc/(Fsamp/2)
--------------------------------------------------------------------------------------------------------------
petit resumé de la theorie de bandlimited interpolation (sinc resampling) :
- le filtre sinc :
	- un filtre passe-bas ideal est le produit de convolution par la fonction sinc(A),
	  sa frequence de coupure theorique Fc est celle dont la periode est egale a la periode du sin dans sinc(A)
	- si le signal est échantillonné, on aura seulement besoin d'échantillons de la fonction sinc(A), separes par
	  un intervalle dA = 2 * PI * ( Fc / Fsamp )
	  on peut aussi definir pispan = "longueur de PI en Tsamp" = PI/dA = Fsamp/2Fc
	- l'implementation sous forme de filtre FIR va impliquer une RI (réponse impulsionnelle) à support borné,
	  dont les non-idéalités pourront être atténuées par un fenetrage (bien centré sur le sinc)
	- on intuite qu'il est bon d'aligner les bornes de ce support sur les zeros de sinc, en definissant
		qpis = nombre (pair) d'intervalles de valeur PI, t.q le support va de -(qpis*PI)/2 à +(qpis*PI)/2
	- alors on observe qu'a la coupure Fc, l'attenuation du signal est -6dB (par rapport a la reponse DC),
	  pour toutes les fenetres usuelles y compris la rectangulaire
	- en augmentant qpis on augmente la pente de part et d'autre de Fc, tendant vers l'ideal
	- on observe des "bosses" dans la bande atténuée, qui ne dependent pas ou peu de qpis (-21dB pour la fenetre rect!)
	- les fenetres servent a abaisser ces bosses, au prix d'une dégradation de la pente
- l'interpolateur ideal :
	- la théorie (Shannon) dit que le filtrage ideal à Fsamp/2 a le pouvoir de reconstituer le signal "originel" a partir d'échantillons,
	  si ce signal originel avait lui-meme une bande limitée à Fsamp/2 avant d'être échantillonné.
	- avec un filtrage non-ideal, on devra abaisser Fc en dessous de Fsamp/2 pour que l'attenuation a Fsamp/2 soit meilleure que 6dB  
	  au prix d'un sacrifice bande passante utile. On traduira cela par un coeff > 1 dit "fudge_factor" = (Fsamp/2)/Fc
	  (Le fudge factor est arbitraire, un compromis entre anti-aliasing et bande passante, qui depend aussi de la fenetre et de qpis)
- l'application au resampling :
	- dans le cas général, les instants d'échantillonnage du signal DEST tombent "entre" les échantillons du signal SRC, on peut alors
	  calculer chacun avec un filtre FIR, en plaçant le "sommet" de ce filtre (A=0) entre les échantillons, ce qui revient à décaler
	  les échantillons pris sur la fonction sinc(A) fenetrée d'un offset A0 (par convention, |A0| < dA)
	- ce qui est admirable, c'est que la réponse fréquentielle (en module) n'est pas altérée par cet offset
	  (si on néglige quelques non-idéalités subtiles)
	- 2 situations de resampling sont a considerer, notons les 2 frequ. d'echantillonnage Fs (src) et Fd (dest) :
	  le filtre soit couper à la plus basse des deux limites de Nyquist, Fd ou Fs 
		- interpolation ( upsampling : Fd > Fs ) :
		  le filtre doit couper a Fs/2 et les echantillons a filtrer sont à Fs
		  ==> dA est indépendant de Fd/Fs) et pispan = fudge factor
		- decimation ( downsampling : Fd < Fs ) :
		  le filtre doit couper a Fd/2 et les echantillons a filtrer sont à Fs
		  ==> dA est multiplié par Fd/Fs, pispan = fudge factor * Fs/Fd
		  le nombre de produits est plus grand, le filtrage est plus couteux
	- Contrairement a un filtrage simple, le resampling ne peut pas utiliser une table de coeffs calculee a l'avance,
	  a moins d'avoir une table suréchantillonnée dans laquelle on pique des coeffs approches en temps réel, en faisant ou pas
	  une interpolation lineaire
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
#include "layer_lod.h"

#include "../modpop3.h"
#include "../cli_parse.h"
#include "../autobuf.h"
#include "../wavio.h"	// il inclut audiofile.h lui-meme
#include "fir.h"
#include "imago.h"
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
// WARNING : les chiffres marchent SEULEMENT sur la top row du clavier, PAS sur le num keypad avec num lock
switch	( v )
	{
	// horiz. scale pour panneau 2
	case '6' :
	case 'N' :
	case 'H' :
		glo->hscale = v; glo->update_f_scale();
		glo->panneau2.fullMN();
		glo->panneau2.force_repaint = 1;
		glo->panneau2.force_redraw = 1;
		break;
	// zoom sur multiple de Fc pour comparaison fenetres (passe en echelle relative Fc)
	case '8' :
	case '4' :
	case '2' : {
		glo->hscale = '6'; glo->update_f_scale();
		double maxM = glo->panneau2.kq * double(v-'0');	// Note : kq = dM / dQ selon JLUPLOT
		glo->panneau2.zoomM( 0.0, maxM );		
		glo->panneau2.force_repaint = 1;
		glo->panneau2.force_redraw = 1;
		} break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau2.dump(); fflush(stdout);
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo5.pdf" );
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
	{
	FFTin[i] = firbuf[i];
	}
// completer avec beaucoup de zeros pour une bonne resolution FFT 
for	( unsigned int i = firsize; i < qFFT; ++i )
	FFTin[i] = 0;

// execution FFT
fftw_execute( plan );

// calcul magnitudes sur place (FFTout contient des valeurs complexes)
unsigned int a = 0;
// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
double k = lefir.getK0();
// le calcul
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

// filtrage simple d'un fichier audio
// utilise une RI pre-calculee dans lefir.FIRbuf
int glostru::audiofile_filter()
{
double Fc = double(wavp.fsamp)/(2.0*lefir.pispan);
printf("Fc @ -6dB : %g Hz\n", Fc );
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
// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
K = lefir.getK0();
j0 = (lefir.qfir-1)/ 2;	// qfir est impair (sauf si A0 != 0.0)
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

// resampling d'un fichier audio, ce qui peut avoir 2 interpretations :
// - changer Fsamp sans changer le contenu, alors Kr = Fsamp_src / Fsamp_dest
// - transposer le contenu sans changer Fsamp, alors :
//	- frequence des composantes du contenu multipliee par Kr
//	- duree divisee par Kr
// le process est identique dans les 2 cas, seule change la declaration de Fsam_dest
// au moment de sauver le resultat
//	Kr < 1 : interpolation : la bande doit etre limitee a Fsamp_src/2
//		on veut simplement interpoler des samples du signal continu "ideal" sous-jacent
//		==> pispan  = fudge factor (legerement superieur a 1), indep. de Kr
//	Kr > 1 : decimation : la bande doit etre limitee a Fsamp_dest/2 < Fsamp_src/2
//		pour eviter un aliasing genre effet stroboscopique
//		==> multiplier pispan par Kr (<==> diviser dA)
// N.B. resampling n'utilise pas de RI pre-calculee, mais est base sur les memes params (objet lefir)
// N.B. on doit avoir fourni lefir.dA = M_PI/fudge_factor, en accord avec les choix de qpis et fenetre.
// Alors cette fonction effectue sur lefir.dA et lefir.pispan la correction dans le cas decimation.
int glostru::audiofile_resamp()
{
// correction pour decimation (abaissement de la bande passante) :
if	( lefir.Kr > 1 )
	{ lefir.dA /= lefir.Kr; lefir.pispan *= lefir.Kr; }
lefir.init_window();

// allocation buffer pour l'audio entier
unsigned int destsize = floor( double(Wbuf.size) / lefir.Kr ); 
if	( destsize > Ybuf.capa )
	{
	Ybuf.reset();	// pour eviter realloc, qui serait inefficace ici
	if	( Ybuf.more( destsize ) )
		gasp("echec alloc Ybuf %d samples", (int)destsize );
	}
Ybuf.size = Ybuf.capa;

printf("resampling: %s par %g, %d output samples\n", ((lefir.Kr>1)?("decimation"):("interpolation")), lefir.Kr, Ybuf.size );

// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
double k = lefir.getK0();

// la boucle va "tirer" chaque sample dest,
// resamp_one() va tirer les sample src selon ses besoins, en gerant les bord sans debordement
double spos;	// index source fractionnaire
for	( int id = 0; id < (int)Ybuf.size; ++id )
	{
	spos = double(id) * lefir.Kr;
	Ybuf.data[id] = (float)( k * lefir.resamp_one( Wbuf.data, spos, 0, Wbuf.size ) );
	}
return 0;
}

// cette fonction evalue la flucuation de la reponse DC en fonction de A0
// donne le min et le max, et la fonction dans Zbuf
void glostru::dc_noise_eval( unsigned int cnt )
{
// buffer pour resultat
if	( cnt > Zbuf.capa )
	{
	Zbuf.reset();	// pour eviter realloc, qui serait inefficace ici
	if	( Zbuf.more( cnt ) )
		gasp("echec alloc Zbuf %d samples", (int)cnt );
	}
Zbuf.size = Zbuf.capa;
// init
double dA0 = lefir.dA / cnt;
double DCmin = 2000000000.0;
double DCmax = 0.0;
// ici un coeff pour ramener la reponse DC a 1.0 (0dB)
double k = lefir.getK0();

// boucle principale
for	( int i = 0; i < (int)cnt; i++ )
	{
	double A0v = double(i) * dA0;		// A0 variable
	double Y = k * lefir.DCsamp_one( A0v );
	Zbuf.data[i] = Y;
	if	( DCmin > Y )
		DCmin = Y;
	if	( DCmax < Y )
		DCmax = Y;
	}
double dBval = 20.0 * log10(DCmax - DCmin);
printf("fluctuations de la reponse DC = (somme coeffs) / pispan (ou equiv Castro)\n"); 
printf("DCmin = %.10f, DCmax = %.10f, diff = %g (%.2f dB)\n", DCmin, DCmax, DCmax - DCmin, dBval );
}

int glostru::audiofile_save( int monosamplesize, int qchan )
{
if	( ( ofnam == NULL ) || ( Ybuf.size == 0 ) )
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

// layout pour reponse impulsionnelle et fenetre
void glostru::layout1()
{
panneau1.offscreen_flag = 0;

// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );
// mettre le sommet du sinc a l'abcisse 0 
panneau1.q0 = - ( double(lefir.cnt_left) + (lefir.A0/lefir.dA) ); 
// configurer le strip
curbande->bgcolor.set( 0.92, 0.98, 1.0 );
curbande->Ylabel = "coef";
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

if	( lefir.FENbuf )
	{
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

if	( Zbuf.size )
	{
	// creer un layer
	curcour = new layer_u<double>;
	curbande->add_layer( curcour, "dc_noise" );

	// configurer le layer
	curcour->set_km( 1.0 );			// sets APRES add_layer
	curcour->set_m0( 0.0 );
	curcour->set_kn( 1.0 );
	curcour->set_n0( 0.0 );
	curcour->fgcolor.set( 0.0, 0.0, 0.8 );

	// connexion layout - data
	curcour->V = Zbuf.data;
	curcour->qu = Zbuf.size;
	curcour->scan();	// alors on peut faire un scan

	// invisible par defaut
	unsigned int ic = curbande->courbes.size() - 1;	// index de ce layer
	panneau1.set_layer_vis( 0, ic, 0 );
	}
}

// layout pour WAV (in et out)
void glostru::layout1wav()
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
layer_lod<float> * curcour;
curcour = new layer_lod<float>;
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
int retval = curcour->make_lods( 4, 4, 2000 );
if	( retval )
	{ gasp("echec make_lods err %d", retval ); }
// N.B. make_lods() inclut scan()
// curcour->scan();	// alors on peut faire un scan

curcour = new layer_lod<float>;
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
retval = curcour->make_lods( 4, 4, 2000 );
if	( retval )
	{ gasp("echec make_lods err %d", retval ); }
}

// layout pour IMG (in et out) ligne centrale de la derniere composante (B)
void glostru::layout1img(imago * limag)
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
layer_u<double> * curcour;
curcour = new layer_u<double>;
curbande->add_layer( curcour, "src" );

// configurer le layer
curcour->set_km( 1.0 );			// sets APRES add_layer
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
int sw = gdk_pixbuf_get_width( limag->pix1 );
int sy = gdk_pixbuf_get_height( limag->pix1 ) / 2;	// milieu
curcour->V = limag->Abuf + sy * sw;
curcour->qu = sw;
curcour->scan();	// alors on peut faire un scan

curcour = new layer_u<double>;
curbande->add_layer( curcour, "resu" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
// N.B. on prend le resu apres resampling horizontal
// pour visualiser le resampling 1D sur une ligne avant que les lignes bougent
int dw = gdk_pixbuf_get_width( limag->pix2 );
curcour->V = limag->Bbuf + sy * dw;
curcour->qu = dw;
curcour->scan();	// alors on peut faire un scan

// echelle verticale n = ( r - r0 ) * kr
curbande->kr = 1.0;
curbande->r0 = 128.0;
}

// echelle graduations axe horizontal sortie FFT  
void glostru::update_f_scale()
{
// Note : kq = dM / dQ selon JLUPLOT
// M est en increments de FFT out, i.e. 1 unite = Fsamp/qFFT 
switch	( hscale )
	{
 	case 'N': panneau2.kq = double(qFFT) / 2.0;			// relatif Fnyquist = Fsamp/2
		break;
	case 'H': panneau2.kq = double(qFFT) / double(Fsamp);		// Hertz
		break;
	case '6':
	default:  panneau2.kq = double(qFFT) / ( 2.0 * lefir.pispan );	// t.q. Fc "-6dB" <==> 1.0
	}
}

void glostru::layout2()
{
// layout jluplot pour reponse frequentielle = resultat FFT
panneau2.offscreen_flag = 0;
// creer le strip
gstrip * curbande;
curbande = new gstrip;
panneau2.add_strip( curbande );
panneau2.bandes[0]->subtk = 2.0;

// configurer le strip
curbande->Ylabel = "dB";
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
// limitation d'etendue horizontale via qu, notamment pour le full zoom horizontal (fullM)
curcour->qu = qFFT/2;	// DFT de real rend un spectre pair -> fftw3 rend seulement une moitié
// pour plancher a -120dB 
curcour->Vfloor = 1e-6;		
// normalisation d'echelle horizontale
update_f_scale();
// CLI options -2, -4, -8 : limiteur  de full zoom special pour comparaison fenetres
// (contrairement aux touches 2, 4, 6 ces CLI options limitent irreversiblement le zoom via qu)
switch	( hscale )
	{
	case '2' :
	case '4' :
	case '8' :
		double maxM = panneau2.kq * double(hscale-'0');	// Note : kq = dM / dQ selon JLUPLOT
		if	( maxM < curcour->qu )
			curcour->qu = maxM;
	}
curcour->scan();		// alors on peut faire un scan
}

void glostru::build_gui()
{
GtkWidget *curwidg;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "JAW");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

// creer boite verticale pour : en haut plot panels, en bas boutons et entries
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( wmain ), curwidg );
vmain = curwidg;

// creer boite verticale pour panel superieur et sa zoombar
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
// gtk_paned_pack1( GTK_PANED(vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
vpan1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, VIEW_W, 200 );	// hauteur mini, la hauteur initiale fixee par parent
gtk_box_pack_start( GTK_BOX( vpan1 ), curwidg, TRUE, TRUE, 0 );
darea1 = curwidg;

/* creer une drawing area pour la zoombar */
curwidg = gtk_drawing_area_new();
zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( vpan1 ), curwidg, FALSE, FALSE, 0 );
zarea1 = curwidg;

/* creer une drawing area pour panel inferieur */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, VIEW_W, VIEW_H / 2 );	// hauteur mini, la hauteur initiale fixee par parent
// gtk_paned_pack2( GTK_PANED(vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
darea2 = curwidg;
// option paned
	{		// paire verticale "paned"
	curwidg = gtk_vpaned_new ();
	gtk_box_pack_start( GTK_BOX( vmain ), curwidg, TRUE, TRUE, 0 );
	// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
	gtk_widget_set_size_request( curwidg, VIEW_W, VIEW_H );
	vpans = curwidg;
	// y placer les deux panels
	gtk_paned_pack1( GTK_PANED(vpans), vpan1, TRUE, FALSE ); // resizable, not shrinkable
	gtk_paned_pack2( GTK_PANED(vpans), darea2, TRUE, FALSE ); // resizable, not shrinkable
	}

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( vmain ), curwidg, FALSE, FALSE, 0 );
hbut = curwidg;

/* texte descriptif */
curwidg = gtk_entry_new();
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_max_length( GTK_ENTRY(curwidg), 128 );
gtk_widget_set_size_request( curwidg, VIEW_W, -1 );
gtk_box_pack_start( GTK_BOX( hbut ), curwidg, FALSE, FALSE, 5 );
edesc = curwidg;

// connecter la zoombar au panel et inversement
panneau1.zoombar = &zbar;
panneau1.zbarcall = gzoombar_zoom;
zbar.panneau = &panneau1;
// connecter les callbacks events --> appli
panneau1.clic_callback_register( clic_call_back1, (void *)this );
panneau1.key_callback_register( key_call_back1, (void *)this );
panneau2.key_callback_register( key_call_back2, (void *)this );

panneau1.events_connect( GTK_DRAWING_AREA( darea1 ) );
panneau2.events_connect( GTK_DRAWING_AREA( darea2 ) );
}

/** ============================ main, quoi ======================= */

void usage()
{
printf("// Usage //\n"
 " -L log de fftsize\n"
 " -P pispan = taille de PI en samples pour calcul RI\n"
 " -Z qpis = taille de RI en PIs\n"
 " -w fenetre 0 = rect, etc...\n"
 " -B param Beta de la fenetre de Kaiser\n"
 " -i taille table pour interpolation des coeffs\n"
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
"Filtrage WAV ou PNG:\n"
 " -K coeff de resampling ( Kr > 1 ==> decimation )\n"
 " -o output file\n"
 " -c channels in saved file (1 ou 2 pour WAV, 3 pour PNG)\n"
 " <input file> (sinon, seulement FFT)\n"
"Echelle horiz. frequence (aussi au clavier a chaud)\n"
 " -6 rel Fc a -6dB\n"
 " -N rel. Nyquist\n"
 " -H Hertz\n"
 " -2 zoom horiz limite a 2 Fc\n"
 " -4 zoom horiz limite a 4 Fc\n"
 " -8 zoom horiz limite a 8 Fc\n"
 " -n no GUI\n"
 "NOTE: -P, -F et -f sont incompatibles\n"
 "      -b, -g, -G transforment passe-bas en passe-bande\n"
 );
}

int main( int argc, char *argv[] )
{
glostru * glo = &theglo;

setlocale( LC_ALL, "C" );       // kill the frog, to be sure

// traiter arguments
if	( argc < 2 )
	{ usage(); return 0; }
cli_parse * lepar = new cli_parse( argc, (const char **)argv, "LPZwBiadbAFGrfgKoc" );
const char * val;
int qFFTlog = 20;
unsigned int saved_qchan = 1;
double relA0 = 0.0;
double F0_rny = 0.0;
double F1_rny = 0.0;
glo->Fsamp = 44100;
double F0_Hz  = 0.0;
double F1_Hz  = 0.0;
int qtable = 0;

if	( ( val = lepar->get( 'L' ) ) )	qFFTlog = atoi( val );			// log de fftsize
if	( ( val = lepar->get( 'P' ) ) )	lefir.pispan = strtod( val, NULL );	// taille de PI en samples pour calcul sinc
if	( ( val = lepar->get( 'Z' ) ) )	lefir.qpis = atoi( val );		// nombre de zeros
if	( ( val = lepar->get( 'w' ) ) )	lefir.window_type = atoi( val );	// 0 = rect, etc...
if	( ( val = lepar->get( 'B' ) ) )	lefir.Kbeta = strtod( val, NULL );	// param Beta de la fenetre de Kaiser
if	( ( val = lepar->get( 'i' ) ) )	qtable = atoi( val );			// taille table pour interpolation


if	( ( val = lepar->get( 'a' ) ) )	lefir.A0 = strtod( val, NULL );		// A0 decalage du centre de la RI (rd)
if	( ( val = lepar->get( 'd' ) ) )	lefir.dA = strtod( val, NULL );		// dA increment angulaire (rd/samp)
if	( ( val = lepar->get( 'b' ) ) )	lefir.rB = strtod( val, NULL );		// rB translation band_center (relatif dA)

if	( ( val = lepar->get( 'A' ) ) )	relA0 = strtod( val, NULL );		// decalage de la RI (relatif dA)
if	( ( val = lepar->get( 'F' ) ) )	F0_rny = strtod( val, NULL );		// frequ de coupure ou min, rel. Nyquist (Fsamp/2)
if	( ( val = lepar->get( 'G' ) ) )	F1_rny = strtod( val, NULL );		// frequ max, rel. Nyquist (Fsamp/2)

if	( ( val = lepar->get( 'r' ) ) )	glo->Fsamp  = atoi( val );		// Fsamp en Hz
if	( ( val = lepar->get( 'f' ) ) )	F0_Hz  = strtod( val, NULL );		// frequence de coupure ou min (Hz)
if	( ( val = lepar->get( 'g' ) ) )	F1_Hz  = strtod( val, NULL );		// frequence max (band) (Hz)

if	( ( val = lepar->get( 'K' ) ) )	lefir.Kr = strtod( val, NULL );		// coeff de resampling ( Kr > 1 ==> decimation )
if	( ( val = lepar->get( 'o' ) ) )	glo->ofnam = val;			// output file
if	( ( val = lepar->get( 'c' ) ) )	saved_qchan = atoi( val );		// channels in saved file

if	( lepar->get( '6' ) )	glo->hscale = '6';	// echelle frequ relative Fc (-6dB)
if	( lepar->get( 'N' ) )	glo->hscale = 'N';	// echelle frequ relative Nyquist
if	( lepar->get( 'H' ) )	glo->hscale = 'H';	// echelle frequ Hertz
if	( lepar->get( '2' ) )	glo->hscale = '2';	// zoom horiz limite a 2 Fc
if	( lepar->get( '4' ) )	glo->hscale = '4';	// zoom horiz limite a 4 Fc
if	( lepar->get( '8' ) )	glo->hscale = '8';	// zoom horiz limite a 8 Fc

if	( lepar->get( 'n' ) )	glo->nogui = 1;		// no GUI

glo->ifnam = lepar->get( '@' );		// naked string = input file

if	( ( qFFTlog < 8 ) || ( lefir.pispan < 1.0 ) || ( lefir.qpis < 4 ) || ( lefir.qpis & 1 ) ||
	  ( lefir.Kbeta > 13.0 ) || ( saved_qchan > 3 ) || ( lefir.window_type > 12 ) )
	{ printf("invalid argument\n"); return -1; }
glo->qFFT = 1 << qFFTlog;

switch	( lefir.window_type ) {
	case 6:
	case 7:	 lefir.firmode = CASTROL; break;
	default: lefir.firmode = ((qtable)?(INTERPOL):(ANALYTIC));
	}

if	( lefir.firmode == INTERPOL )
	{
	lefir.deftable.qtable = qtable;
	lefir.deftable.qpis = lefir.qpis;
	}

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
// s'assurer que dA et pispan sont tous les deux definis et coherents
if	( lefir.dA == 0.0 )
	lefir.dA = M_PI / lefir.pispan;
else	lefir.pispan = M_PI / lefir.dA;
// alors calculer A0 qui depend de dA
if	( relA0 != 0.0 )
	lefir.A0 = relA0 * lefir.dA;

// on a fini avec les arguments... on cree une fenetre ?
if	( glo->nogui == 0 )
	{
	gtk_init(&argc,&argv);
	setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init
	glo->build_gui();
	}

// generer FIR
int retval = lefir.generate();
if	( retval )
	gasp(" erreur %d", retval );
if	( glo->nogui == 0 )
	gtk_entry_set_text( GTK_ENTRY( glo->edesc ), lefir.description );

// FFT pour reponse frequentielle
retval = glo->fft_on_FIR( lefir.qfir, lefir.FIRbuf );
if	( retval )
	gasp(" erreur %d", retval );

if	( glo->nogui == 0 )
	glo->layout2();			// afficher FFT

// possible traitement sur fichiers d'échantillons, declenche par la presence de glo->ifnam
if	( glo->ifnam )
	{
	printf("fichier a traiter: %s\n", glo->ifnam ); fflush(stdout);
	if	( saved_qchan < 3 )
		{			// // // traitement audio // // //
		retval = glo->audiofile_load( 1 );
		if	( retval )
			{ printf("echec lecture fichier audio\n"); exit(1); }	// abandon
		else	{
			if	( lefir.Kr != 0.0 )
				glo->audiofile_resamp();
			else	glo->audiofile_filter();
			if	( glo->nogui == 0 ) glo->layout1wav();	// visu wav
			if	( glo->ofnam )
				// glo->audiofile_save( glo->wavp.monosamplesize, saved_qchan );
				glo->audiofile_save( 4, saved_qchan );	// on veut f32, et type sera mis a jour par audiofile_save
			}
		}
	else	{			// // // traitement image // // //
		imago limag;
		retval = limag.read( glo->ifnam );
		if	( retval )
			{ printf("echec lecture fichier image\n"); exit(1); }	// abandon
		else	{
			if	( lefir.Kr != 0.0 )
				{
				limag.resamp( &lefir );
				if	( glo->nogui == 0 ) glo->layout1img(&limag);	// visu pixels
				}
			else	{
				limag.filter( &lefir );
				if	( glo->nogui == 0 ) glo->layout1();		// visu fenetre et RI
				}
			if	( glo->ofnam )
				limag.save_png( glo->ofnam );
			}
		}
	}
else	{
	glo->dc_noise_eval( 500 );
	if	( glo->nogui == 0 ) glo->layout1();			// visu fenetre et RI
	}

if	( glo->nogui == 0 )
	{
	gtk_widget_show_all( glo->wmain );
	glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
	// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );
	fflush(stdout);
	gtk_main();
	g_source_remove( glo->idle_id );
	}
return(0);
}


// les classes pour le projet JLUPLOT
// doc dans jluplot.h

using namespace std;
#include <string>
#include <vector>

#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "jluplot.h"

// cree une couleur parmi 8 (noir inclus, pas de blanc)
void jcolor::arc_en_ciel( int i )
{
switch( i % 8 )
   {
   case 0 : dR = 0.0; dG = 0.0; dB = 0.0; break;
   case 1 : dR = 1.0; dG = 0.0; dB = 0.0; break;	// rouge
   case 2 : dR = 0.9; dG = 0.4; dB = 0.0; break;	// orange
   case 3 : dR = 0.6; dG = 0.6; dB = 0.0; break;	// jaune-vert
   case 4 : dR = 0.0; dG = 0.6; dB = 0.1; break;	// vert
   case 5 : dR = 0.0; dG = 0.0; dB = 0.8; break;	// bleu
   case 6 : dR = 0.5; dG = 0.0; dB = 0.5; break;	// violet
   case 7 : dR = 0.5; dG = 0.5; dB = 0.5; break;	// gris
   }
}

// une courbe (ou autre graphique) superposable : classe de base abstraite

// mettre a jour la transformation XY <--> UV soit la cascade XY <--> MN <--> UV
void layer_base::refresh_proxy()
{
u0 = UdeM( parent->parent->MdeX( 0.0 ) );
ku = 1.0 / ( km * parent->parent->get_kx() );
v0 = VdeN( parent->NdeY( 0.0 ) );
kv = 1.0 / ( kn * parent->get_ky() );
}

// un strip, pour heberger une ou plusieurs courbes superposees
// dessin (la largeur dx est lue chez le panel parent)

void strip::refresh_proxies()
{
for	( unsigned int ic = 0; ic < courbes.size(); ic++ )
	courbes.at(ic)->refresh_proxy();
force_redraw = 1;
}

void strip::set_y0( double Y0 )
{
y0 = Y0;
refresh_proxies();
}

void strip::set_ky( double kY )
{
ky = kY;
refresh_proxies();
}

// zoom absolu
void strip::zoomN( double nmin, double nmax )
{
if	( ( nmax - nmin ) <= 0.0 )
	return;
// premiere etape : mise a jour des coeffs de conversion N <--> Y t.q. n = ( y - y0 ) * ky
ky = ( nmax - nmin ) / (double)ndy;
y0 = - nmin / ky;
// seconde etape : mise a jour des proxies dans chaque layer
refresh_proxies();
// troisieme etape : mise a jour des ticks
double rmin = RdeN( nmin );
double rmax = RdeN( nmax );
tdr = autotick( rmax - rmin, qtky );
}

// zoom relatif
void strip::zoomY( double ymin, double ymax )
{
zoomN( NdeY( ymin ), NdeY( ymax ) );
}

// pan relatif, par un deplacement Y
void strip::panY( double dy )
{
zoomN( NdeY( dy ), NdeY( dy + (double)ndy ) );
}

// zoom par un facteur, t.q. 0.5 pour zoom in, 2.0 pour zoom out
void strip::zoomYbyK( double k )
{
double yl, yr, wy;
wy = (double)ndy;
yl = wy * 0.5 * ( 1 - k );
yr = yl + ( wy * k );
zoomY( yl, yr );
}

// pan par un facteur, t.q. 0.5 pour la moitie de la hauteur
void strip::panYbyK( double k )
{
double dy = k * (double)ndy;
panY( dy );
}

void strip::fullN()
{
// premiere etape : acquisition des valeurs limites
double nmin, nmax, nn;
layer_base * la;
nmin = HUGE_VAL;
nmax = -HUGE_VAL;
for	( unsigned int ic = 0; ic < courbes.size(); ic++ )
	{
	la = courbes.at(ic);
	if	( la->visible )
		{
		nn = la->NdeV(la->get_Vmin());
		if	( nn < nmin )
			nmin = nn;
		nn = la->NdeV(la->get_Vmax());
		if	( nn > nmax )
			nmax = nn;
		}
	}
// seconde etape : action
if	( ( nmin != HUGE_VAL ) && ( nmax != -HUGE_VAL ) )
	{
	double dn = nmax - nmin;
	dn *= kmfn;	// marges t.q. 5% pour compatibilité visuelle avec jluplot 0
	nmin -= dn; nmax += dn;
	zoomN( nmin, nmax );
	}
}

// met a jour la hauteur nette en pixels sans se soucier du zoom
void strip::presize( int rendy )
{
ndy = rendy;
fdy = ndy + ((optX)?(parent->my):(0));
}

// met a jour la hauteur puis le zoom pour conserver la meme zone visible
void strip::resize( int rendy )
{
double N0, N1;
N0 = NdeY( 0.0 ); N1 = NdeY( ndy );
// printf("strip::resize N de %g a %g, ndy de %u a %d\n", N0, N1, ndy, rendy );
ndy = rendy;
fdy = ndy + ((optX)?(parent->my):(0));
zoomN( N0, N1 );
}

void strip::draw( cairo_t * cai )
{
// printf("strip::begin draw\n");
cairo_save( cai );
// faire le fond
cairo_set_source_rgb( cai, bgcolor.dR, bgcolor.dG, bgcolor.dB );
if	( optcadre )
	{
	double linew = cairo_get_line_width( cai );
	cairo_rectangle( cai, linew, linew, parent->ndx - linew * 2, ndy - linew * 2 );
	cairo_stroke( cai );
	}
else	{
	cairo_rectangle( cai, 0, 0, parent->ndx, ndy );
	cairo_fill( cai );
	}
// translater l'origine Y en bas de la zone utile des courbes
// l'origine X est deja au bord gauche de cette zone
cairo_translate( cai, 0, ndy );

// les textes de l'axe X (fond blanc inclus)
if	( optX )
	gradu_X( cai );

// preparer le clip, englobant les graduations Y car il peut leur arriver de deborder
// mais pas celles de X car on ne veut pas que les courbes debordent dessus
cairo_rectangle( cai, -parent->mx, -ndy, parent->fdx, ndy );
cairo_clip( cai );

// le reticule sous les layers
if	( optretX == 1 )
	reticule_X( cai );
// les textes de l'axe Y, soumis au clip (fond blanc inclus)
gradu_Y( cai );
// le reticule sous les layers (doit etre apres gradu_Y a cause de son fond blanc)
if	( optretY == 1 )
	reticule_Y( cai );

// tracer les courbes
int i;
for	( i = ( courbes.size() - 1 ); i >= 0; i-- )
	{
	if	( courbes.at(i)->visible )
		{
		courbes.at(i)->ylabel = ( 20 * i ) + 20;
		courbes.at(i)->draw( cai );
		}
	}

cairo_reset_clip( cai );

// tracer les reticules sur les layers
if	( optretX == 2 )
	reticule_X( cai );
if	( optretY == 2 )
	reticule_Y( cai );

cairo_restore( cai );

force_redraw = 0;
// printf("strip::end draw\n");
}

// cette fonction interprete la coordonnee y (mouse) relatives au strip
// et la convertit en y (jluplot) relative au strip
// rend 0 si ok ou -1 si hors strip en Y
// *py sera negatif si on est dans la marge inferieure (c'est a dire dans la graduation X)
int strip::clicY( double y, double * py )
{
// N.B. x est deja verifie au niveau panel
if ( ( y < 0.0 ) || ( y >= fdy ) || ( courbes.size() == 0 ) )
   return -1;
*py = ndy - y;	// translater origine Y en bas de la zone
return 0;
}

// tracer le reticule x : la grille de barres verticales
void strip::reticule_X( cairo_t * cai )
{
cairo_set_source_rgb( cai, lncolor.dR, lncolor.dG, lncolor.dB );
double curq, curx;
if	( parent->optLog10 == 0 )
	{				// ECHELLE LINEAIRE (normale)
	if	( subtk > 1 )
		{
		double subtdq = parent->tdq / subtk;
		// chercher le premier subtick
		curq = subtdq * ceil( parent->QdeM( parent->MdeX( 0 ) ) / subtdq );
		cairo_save( cai );
		cairo_set_line_width( cai, 0.5 * cairo_get_line_width( cai ) );
		// couleur moyennee avec le fond
		cairo_set_source_rgb( cai, 0.5*(bgcolor.dR+lncolor.dR), 0.5*(bgcolor.dG+lncolor.dG), 0.5*(bgcolor.dB+lncolor.dB) );
		while	( ( curx = parent->XdeM(parent->MdeQ(curq)) ) < (double)parent->ndx )
			{
			cairo_move_to( cai, curx, -((double)ndy ) );	// top
			cairo_line_to( cai, curx, 0.0 );		// bottom
			cairo_stroke( cai );
			curq += subtdq;
			}
		cairo_restore( cai );
		}
	// chercher le premier tick
	curq = parent->tdq * ceil( parent->QdeM( parent->MdeX( 0 ) ) / parent->tdq );
	while	( ( curx = parent->XdeM(parent->MdeQ(curq)) ) < parent->ndx )
		{
		cairo_move_to( cai, curx, -((double)ndy ) );	// top
		cairo_line_to( cai, curx, 0.0 );		// bottom
		cairo_stroke( cai );
		curq += parent->tdq;
		}
	}
else	{	// ECHELLE LOG STYLE BODE (see panel::logscale_helper() )
	parent->bode_log_scale( cai, false, - double(ndy), 0.0 );
	}
}

// les textes de l'axe X avec les taquets
void strip::gradu_X( cairo_t * cai )
{
char lbuf[32];
cairo_set_source_rgb( cai, 1, 1, 1 );
cairo_rectangle( cai, -parent->mx, 0, parent->fdx, fdy - ndy );
cairo_fill( cai );
cairo_set_source_rgb( cai, 0, 0, 0 );
if	( parent->optLog10 == 0 )
	{				// ECHELLE LINEAIRE (normale)
	double curq = parent->tdq * ceil( parent->QdeM( parent->MdeX( 0 ) ) / parent->tdq );
	double curx = parent->XdeM(parent->MdeQ(curq));
	// preparation format selon premier point
	scientout( lbuf, curq, parent->tdq );
	while	( curx < parent->ndx )
		{
		scientout( lbuf, curq );
		cairo_move_to( cai, curx - 20, 15 );
		cairo_show_text( cai, lbuf );
		cairo_move_to( cai, curx, 0.0 );	// top
		cairo_line_to( cai, curx, 3.0 ); 	// bottom
		cairo_stroke( cai );
		curq += parent->tdq;
		curx = parent->XdeM(parent->MdeQ(curq));	// la transformation
		}
	}
else	{	// ECHELLE LOG STYLE BODE (see panel::logscale_helper() )
	parent->bode_log_scale( cai, true, 0.0, 3.0 );
	}
}

// tracer le reticule y : la grille de barres horizontales
void strip::reticule_Y( cairo_t * cai )
{
cairo_set_source_rgb( cai, lncolor.dR, lncolor.dG, lncolor.dB );
double curr, cury;
if	( subtk > 1 )
	{
	double subtdr = tdr / subtk;
	// chercher le premier subtick
	curr = subtdr * ceil( RdeN( NdeY( 0 ) ) / subtdr );
	cairo_save( cai );
	cairo_set_line_width( cai, 0.5 * cairo_get_line_width( cai ) );
	// couleur moyennee avec le fond
	cairo_set_source_rgb( cai, 0.5*(bgcolor.dR+lncolor.dR), 0.5*(bgcolor.dG+lncolor.dG), 0.5*(bgcolor.dB+lncolor.dB) );
	while	( ( cury = -YdeN(NdeR(curr)) ) > -((double)ndy ) )		// le haut (<0)
		{
		cairo_move_to( cai, 0, cury );
		cairo_line_to( cai, parent->ndx, cury );
		cairo_stroke( cai );
		curr += subtdr;
		}
	cairo_restore( cai );
	}
// chercher le premier tick
curr = tdr * ceil( RdeN( NdeY( 0 ) ) / tdr );
while	( ( cury = -YdeN(NdeR(curr)) ) > -((double)ndy ) )		// le haut (<0)
	{
	cairo_move_to( cai, 0, cury );
	cairo_line_to( cai, parent->ndx, cury );
	cairo_stroke( cai );
	curr += tdr;
	}
}

// les textes de l'axe Y avec les taquets
void strip::gradu_Y( cairo_t * cai )
{
char lbuf[32]; int mx;
mx = parent->mx;
cairo_set_source_rgb( cai, 1, 1, 1 );
cairo_rectangle( cai, -mx, -ndy, mx, ndy );
cairo_fill( cai );
cairo_set_source_rgb( cai, 0, 0, 0 );
double curr = tdr * ceil( RdeN( NdeY( 0 ) ) / tdr );	// chercher le premier tick
double cury = -YdeN(NdeR(curr));		// la transformation
// preparation format selon premier point
scientout( lbuf, curr, tdr );
while	( cury > ( -((double)ndy) + 19.0 ) )		// petite marge pour label axe
	{
	scientout( lbuf, curr );
	cairo_move_to( cai, -mx + 10.0, cury + 5 );
	cairo_show_text( cai, lbuf );
	cairo_move_to( cai, -6, cury );	// le -6 c'est pour les graduations aupres des textes
	cairo_line_to( cai, 0, cury );
	cairo_stroke( cai );
	curr += tdr;
	cury = -YdeN(NdeR(curr));		// la transformation
	}
// label de l'axe (unites)
cairo_move_to( cai, -mx + 10.0, -((double)ndy) + 14.0 );
cairo_show_text( cai, Ylabel.c_str() );
}

// le panel, pour heberger plusieurs strips (axe X commun ou identique)

void panel::refresh_proxies()
{
strip * b;
for	( unsigned int ib = 0; ib < bandes.size(); ib++ )
	{
	b = bandes.at(ib);
	for	( unsigned int ic = 0; ic < b->courbes.size(); ic++ )
		b->courbes.at(ic)->refresh_proxy();
	}
force_redraw = 1;
// printf("panel::refresh_proxies()\n"); fflush(stdout);
}

void panel::set_x0( double X0 )
{
x0 = X0;
refresh_proxies();
}

void panel::set_kx( double kX )
{
kx = kX;
refresh_proxies();
}

// zoom absolu
void panel::zoomM( double mmin, double mmax )
{
if	( ( mmax - mmin ) <= 0.0 )
	return;
// premiere etape : mise a jour des coeffs de conversion M <--> X t.q. m = ( x - x0 ) * kx
kx = ( mmax - mmin ) / (double)ndx;
x0 = - mmin / kx;
// seconde etape : mise a jour des proxies dans chaque layer de chaque strip
refresh_proxies();
// troisieme etape : mise a jour des ticks
double qmin = QdeM( mmin );
double qmax = QdeM( mmax );
tdq = autotick( qmax - qmin, qtkx );
// quatrieme etape : mise a jour scrollbar si elle existe
if	( zbarcall )
	zbarcall( zoombar, ( mmin - fullmmin ) / ( fullmmax - fullmmin ),
			   ( mmax - fullmmin ) / ( fullmmax - fullmmin ) );
}

// zoom relatif
void panel::zoomX( double xmin, double xmax )
{
zoomM( MdeX( xmin ), MdeX( xmax ) );
}

// pan relatif
void panel::panX( double dx )
{
zoomM( MdeX( dx ), MdeX( dx + (double)ndx ) );
}

// zoom par un facteur, t.q. 0.5 pour zoom in, 2.0 pour zoom out
void panel::zoomXbyK( double k )
{
double xl, xr, wx;
wx = (double)ndx;
xl = wx * 0.5 * ( 1 - k );
xr = xl + ( wx * k );
zoomX( xl, xr );
}

// pan par un facteur, t.q. 0.5 pour la moitie de la largeur a droite
void panel::panXbyK( double k )
{
double dx = k * (double)ndx;
panX( dx );
/* double xl, xr, wx;
wx = (double)ndx;
xr = wx;
wx *= k;
xl = wx;
xr += wx;
zoomX( xl, xr ); */
}

// met a jour les dimensions en pixels (sans se soucier des transformations)
void panel::presize( int redx, int redy )
{
int ndy, bndy; unsigned int ib, cnt;
fdx = redx;
ndx = fdx - mx;
ndy = fdy = redy;
if	( bandes.size() )
	{
	// compter les bandes visibles
	cnt = 0;
	for	( ib = 0; ib < bandes.size(); ib++ )
		{
		if	( bandes.at(ib)->visible )
			{
			cnt++;
			// soustraire l'espace pour les graduations optionnelles
			ndy -= ((bandes.at(ib)->optX)?(my):(0));
			}
		}
	if	( cnt == 0 )
		{
		// printf("panel::presize : 0 bande visible\n" );
		return;
		}
 	// bandes d'egale hauteur
	bndy = ndy / cnt;
	// reste de la division applique a la premiere bande
	bandes.at(0)->presize( bndy + ( ndy % cnt ) );
	// les autres bandes
	for	( ib = 1; ib < bandes.size(); ib++ )
		bandes.at(ib)->presize( bndy );
	// printf("panel::presize : %d x %d, par bande %d puis %d\n", redx, redy, bndy + ( ndy % cnt ), bndy );
	}
}

// met a jour les dimensions en pixels puis les zooms pour conserver les memes zones visibles
void panel::resize( int redx, int redy )
{
// test pour savoir si on doit faire un full
if	( full_valid == 0 )
	{
	presize( redx, redy );
	fullMN();	// printf("auto fullNM\n" );
	return;
	}
// restituer zooms en cours sur chaque layer
double M0, M1; int ndy, bndy; unsigned int ib, cnt;
// horizontalement
M0 = MdeX( 0.0 ); M1 = MdeX( ndx );
fdx = redx;
ndx = fdx - mx;
zoomM( M0, M1 );
// verticalement : sera fait par chaque strip::resize
ndy = fdy = redy;
if	( bandes.size() )
	{
	// compter les bandes visibles
	cnt = 0;
	for	( ib = 0; ib < bandes.size(); ib++ )
		{
		if	( bandes.at(ib)->visible )
			{
			cnt++;
			// soustraire l'espace pour les graduations optionnelles
			ndy -= ((bandes.at(ib)->optX)?(my):(0));
			}
		}
	if	( cnt == 0 )
		{
		// printf("panel::resize : 0 bande visible\n" );
		return;
		}
	// bandes d'egale hauteur
	bndy = ndy / cnt;
	// reste de la division applique a la premiere bande
	bandes.at(0)->resize( bndy + ( ndy % cnt ) );
	// les autres bandes
	for	( ib = 1; ib < bandes.size(); ib++ )
		bandes.at(ib)->resize( bndy );
	// printf("panel::resize : %d x %d, par bande %d puis %d\n", redx, redy, bndy + ( ndy % cnt ), bndy );
	}
}

void panel::fullM()
{
strip * b;
layer_base * la;
// premiere etape : acquisition des valeurs limites
double mmin, mmax, mm;
mmin = HUGE_VAL;
mmax = -HUGE_VAL;
for	( unsigned int ib = 0; ib < bandes.size(); ib++ )
	{
	b = bandes.at(ib);
	if	( b -> visible )
		{
		for	( unsigned int ic = 0; ic < b->courbes.size(); ic++ )
			{
			la = b->courbes.at(ic);
			if	( la -> visible )
				{
				mm = la->MdeU(la->get_Umin());
				if	( mm < mmin )
					mmin = mm;
				mm = la->MdeU(la->get_Umax());
				if	( mm > mmax )
					mmax = mm;
				// printf("ib=%d, ic=%d, Umax=%g / km=%g -> mm=%g\n", ib, ic, la->get_Umax(), la->km, mm );
				}
			}
		}
	}
// seconde etape : action
if	( ( mmin != HUGE_VAL ) && ( mmax != -HUGE_VAL ) )
	{
	fullmmin = mmin; fullmmax = mmax;
	// printf("in fullM, full is %g to %g\n", fullmmin, fullmmax );
	zoomM( mmin, mmax );
	}
}

void panel::fullMN()
{
for	( unsigned int ib = 0; ib < bandes.size(); ib++ )
	bandes.at(ib)->fullN();
fullM();
full_valid = 1;
}

void panel::draw( cairo_t * cai )
{
cairo_save( cai );
cairo_translate( cai, mx, 0 );
// fill the background red JUST TO CHECK THAT THEY PAINT EVERYWHERE
// cairo_set_source_rgb( cai, 1, 0, 0 );
// cairo_paint(cai);	// paint the complete clip area
int strip_draw_cnt = 0;
// on veut la courbe 0 en haut...
for	( unsigned int ib = 0; ib < bandes.size(); ib++ )
	{
	if	( bandes.at(ib)->visible )
		{
		bandes.at(ib)->draw( cai );
		cairo_translate( cai, 0, bandes.at(ib)->fdy );
		++strip_draw_cnt;
		}
	}
if	( strip_draw_cnt == 0 )	// pas de strip ? fond gris
	{
	cairo_set_source_rgb( cai, 0.8, 0.8, 0.8 );
	cairo_paint(cai);	// paint the complete clip area
	}
force_redraw = 0;
cairo_restore( cai );
}

// ECHELLE LOG STYLE BODE "f = pow( 10, q )" (see panel::logscale_helper() )
// fonction au service de strip::gradu_X() et strip::reticule_X()
void panel::bode_log_scale(  cairo_t * cai, bool gradu, double ytop, double ybot )
{
double curq, curx;
double leftq  = QdeM(MdeX(0));
double rightq = QdeM(MdeX(ndx));
// nombre de decades visibles
double qdec = rightq - leftq;
// printf("%g decades visibles, %g pixels par decade\n", qdec, pixperdecade ); fflush(stdout);
// iq = exposant de la puissance de 10 immediatement inferieure ou egale au bord gauche
double iq = floor( leftq );
char lbuf[32];
if	( qdec > 0.95 )		// style "papier log"
	{
	int loglod;
	if	( gradu )
		{
		double pixperdecade = kq / get_kx();
		if	( pixperdecade > 700 ) loglod = 3;
		else if	( pixperdecade > 280 ) loglod = 2;
		else if	( pixperdecade > 150 ) loglod = 1;
		else	loglod = 0;
		// printf("gradu_X: start %g\n", pow( 10, iq ) ); fflush(stdout);
		}
	curx = XdeM(MdeQ(iq));
	while	( curx < ndx )
		{
		if	( gradu && ( curx >= 0 ) )	// texte pour puissance de 10
			{
			// scientout( lbuf, pow( 10, iq ) );
			snprintf( lbuf, sizeof(lbuf), "%.0e", pow( 10, iq ) );
			cairo_move_to( cai, curx - 20, 15 );
			cairo_show_text( cai, lbuf );
			}
		for	( int j = 1; j < 10; j++ )	// 9 ticks pour 1 decade
			{
			curq = iq + log10(double(j));
			curx = XdeM(MdeQ(curq));	// la transformation
			if	( curx >= ndx )
				break;
			if	( curx >= 0 )
				{
				cairo_move_to( cai, curx, ytop );	// top
				cairo_line_to( cai, curx, ybot ); 	// bottom
				cairo_stroke( cai );
				if	( gradu && (
					( ( loglod >= 1 ) && ( (j==2) || (j==5) ) ) ||
					( ( loglod >= 2 ) && ( (j==3) || (j==4) || (j==6) || (j==7) ) ) ||
					( ( loglod >= 3 ) && (  j>1 ) )
					) )
					{
					snprintf( lbuf, sizeof(lbuf), "%d", j );
					cairo_move_to( cai, curx - 4, 15 );
					cairo_show_text( cai, lbuf );
					}
				}
			}
		iq += 1.0;
		curx = XdeM(MdeQ(iq));
		}
	}
else	{			// zoomed-in, style quasi lin
	// frequence des bords
	double leftf  = pow( 10, leftq );
	double rightf = pow( 10, rightq );
	// intervalle des ticks en frequence
	double tdf = autotick( rightf - leftf, qtkx );
	// frequence du premier tick = premier multiple de tdf >= leftf
	double f = ceil( leftf / tdf ) * tdf;
	do	{
		curq = log10(f);
		if	( fabs( curq - ( iq + 1.0 ) ) < 0.00001 )
			{		// recalage sur la decade suivante
			iq += 1.0;
			curq = iq;
			f = pow( 10, iq );
			tdf *= 10.0;
			}		
		curx = XdeM(MdeQ(curq));	// la transformation
		if	( curx >= ndx )
			break;
		cairo_move_to( cai, curx, ytop );	// top
		cairo_line_to( cai, curx, ybot ); 	// bottom
		cairo_stroke( cai );
		if	( gradu )
			{
			snprintf( lbuf, sizeof(lbuf), "%g", f );
			cairo_move_to( cai, curx - 4, 15 );
			cairo_show_text( cai, lbuf );
			}
		f += tdf;
		} while ( curx < ndx );
	}
}

// rend 0 si Ok
int panel::pdfplot( const char * fnam, const char * caption )
{
cairo_surface_t * cursurf;
cairo_status_t cairo_status;
cairo_t * lecair;
unsigned int saved_fdx, saved_fdy, saved_optcadre[bandes.size()];
jcolor saved_bgcolor[bandes.size()];

double pdf_w, pdf_h, pdf_margin;

// format A4 landscape, en "dots"
pdf_w = ( 297.0 / 25.4 ) * pdf_DPI;
pdf_h = ( 210.0 / 25.4 ) * pdf_DPI;
pdf_margin = ( 9.0 / 25.4 ) * pdf_DPI;

// save draw context
saved_fdx = fdx; saved_fdy = fdy;
for	( unsigned int iban = 0; iban < bandes.size(); iban++ )
	{
	saved_optcadre[iban] = bandes[iban]->optcadre;
	saved_bgcolor[iban] = bandes[iban]->bgcolor;
	}

cursurf = cairo_pdf_surface_create( fnam, pdf_w, pdf_h );

cairo_status = cairo_surface_status( cursurf );
if   ( cairo_status != CAIRO_STATUS_SUCCESS )
     return( cairo_status );

lecair = cairo_create( cursurf );

// un peu de marge (en points)
cairo_translate( lecair, pdf_margin, pdf_margin );

// set PDF draw context
resize( (int)pdf_w - ( 2 * pdf_margin ), (int)pdf_h - ( 3 * pdf_margin ) );
for	( unsigned int iban = 0; iban < bandes.size(); iban++ )
	{
	bandes[iban]->optcadre = 1;
	if	( bandes[iban]->subtk > 1 )
		bandes[iban]->bgcolor.set( 1.0, 1.0, 1.0 );
	else	bandes[iban]->bgcolor.set( 0.0, 0.0, 0.0 );
	}

// preparer font (attention aux pb de compatiblite PDF... 'monospace' ==> bug sporadique)
cairo_select_font_face( lecair, "Courier", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
cairo_set_font_size( lecair, 12.0 );	// en dots
cairo_set_line_width( lecair, 0.5 );

// draw the curves
draw( lecair );

// the caption
cairo_move_to( lecair, 10.0, pdf_h - ( 2 * pdf_margin ) );
cairo_show_text( lecair, caption );

cairo_destroy( lecair );
cairo_surface_destroy( cursurf );

// restore draw context
resize( saved_fdx, saved_fdy );
for	( unsigned int iban = 0; iban < bandes.size(); iban++ )
	{
	bandes[iban]->optcadre = saved_optcadre[iban];
	bandes[iban]->bgcolor = saved_bgcolor[iban];
	}

return 0;
}

// cette fonction interprete les coordonnees x,y (mouse) relatives a la drawing-area
// et les convertit en x, y (jluplot) relatives au strip,
// rend une valeur negative si hors graphique,
// rend l'indice du strip avec eventuellement un flag bit si clic dans une marge
int panel::clicXY( double x, double y, double * px, double * py )
{
// pre-traitement X
if	( ( x < 0.0 ) || ( x >= (double)fdx ) )
	return CLIC_OUT_X;
*px = x - (double)mx;			// x < 0 si on est dans les graduations Y
// identification strip
int istrip;
for	( istrip = 0; istrip < (int)bandes.size(); istrip++ )
	if	( bandes.at(istrip)->visible )
		{
		if	( bandes.at(istrip)->clicY( y, py ) == 0 )
			break;
		y -= bandes.at(istrip)->fdy;	// transformation inverse (cf panel::draw)
		}
if	( istrip >= (int)bandes.size() )
	return CLIC_OUT_Y;
// ici on a identifie un strip, *px et *py sont a jour, on va signaler si on est dans les marges
if	( *px < 0.0 )
	istrip |= CLIC_MARGE_GAUCHE;
if	( *py < 0.0 )
	istrip |= CLIC_MARGE_INF;
return istrip;
}

// cette fonction interprete les coordonnees x,y relatives a la drawing-area
// et les convertit en M,N
// rend une valeur negative si hors graphique,
// rend l'indice du strip avec eventuellement un flag bit si clic dans une marge
int panel::clicMN( double x, double y, double * pM, double * pN )
{
int istrip = clicXY( x, y, &x, &y );
if	( istrip < 0 )
	return istrip;
*pM = MdeX(x);	// les transformations
*pN = bandes.at(istrip&(~CLIC_MARGE))->NdeY(y);
return istrip;
}

/* cette fonction interprete les coordonnees x,y relatives a la drawing-area
// et les convertit en Q,R
// rend une valeur negative si hors graphique,
// rend l'indice du strip avec eventuellement un flag bit si clic dans une marge
int panel::clicQR( double x, double y, double * pQ, double * pR )
{
double m, n;
int istrip = clicMN( x, y, &m, &n );
if	( istrip < 0 )
	return istrip;
*pQ = QdeM(m);
*pR = bandes.at(istrip&(~CLIC_MARGE))->RdeN(n);
return istrip;
} */

// trouver une valeur de tick appropriee
double autotick( double range, unsigned int nmax )
{
static const double coef[] = { 5.0, 2.0, 1.0, 0.5, 0.2, 0.1, 0.05, 0.02, 0.01, 0.005, 0.002, 0.001 };
// puissance de 10 immediatement inferieure a range
double tickbase = exp( floor( log10( range ) ) * log(10.0) );
// note : log10(x) equals log(x) / log(10).
// note : exp10(x) is the same as exp(x * log(10)), this functions is GNU extension.

// augmenter progressivement le nombre de ticks
unsigned int i = 0; double cnt, tickspace;
while ( i < ( sizeof(coef) / sizeof(double) ) )
   {
   tickspace = tickbase * coef[i];
   cnt = floor( range / tickspace );
   if ( (unsigned int)cnt > nmax )
      break;
   ++i;
   }
if ( i ) --i;
// printf("autotick( %g, %d ) --> %g\n", range, nmax, tickbase * coef[i] );
return( tickbase * coef[i] );
}

// conversion double-->texte, avec un format de precision lie a la valeur du tick.
// si la valeur du tick est <= 0.0 ou absente, utilisation du format anterieur memorise
// par rapport au %g, il y a 3 differences :
//	- le format est choisi en fonction du tick, pas seulement de la valeur elle-meme,
//	  de maniere a pouvoir toujours differencier un ecart d'un tick
//	- la memorisation du format assure un format homogene sur une echelle
//	- les exposants "eN" sont remplaces par les suffixes "fpnum kMGT"
// la valeur est supposee etre multiple du tick (i.e. elle sera souvent arrondie au tick)
//	- retour = nombre de chars utiles
//	- lbuf doit pouvoir contenir 32 bytes
//
#define ANTI_MK		// option moderant l'usage des sufixes m et k
int scientout( char * lbuf, double val, double tick )
{
//                          0123456789
static const char *osuff = "fpnum kMGT";		// de femto a Tera
static const double opow[] = {
	1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10,
	1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2,
	0.1, 1, 10, 100, 1000,
	1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15 };

int v_exp;		// ordre de grandeur de la valeur, arrondi par defaut
static int v_exp3 = 0;	// ordre de grandeur de la valeur, arrondi par defaut au multiple de 3
int t_exp; 		// ordre de grandeur du tick, arrondi par defaut
static int qfrac = 1;	// resolution de la partie fractionnaire (nombre digits apres dot)

	/// ----------------------- preparation format --------------------------------------------
if	( tick > 0.0 )
	{
	double aval;
	// normalisation
	aval = fabs(val);
	if ( aval < ( 5.0 * tick ) )	// couvre le cas val == 0.0
	   aval = ( 5.0 * tick );	// et les valeurs autour de 0 peu representatives
	// ordre de grandeur de la valeur
	aval = log10( aval );
	v_exp  = (int)floor( aval );
	v_exp3 = (int)floor( aval / 3 );
	v_exp3 *= 3;	// exemple : -3 de 0.001 a 0.999, 0 de 1 a 999; 3 de 1000 a 9999
	// ordre de grandeur du tick (en evitant arrondi du 1 vers 0.99999)
	t_exp = (int)floor( log10(tick) + 1e-12 );
	// estimation de la resolution necessaire
	qfrac = v_exp3 - t_exp;
	if ( qfrac < 0 ) qfrac = 0;
	// printf("\nv_exp = %d   v_exp3 = %d    t-exp = %d --> qfrac = %d\n", v_exp, v_exp3, t_exp, qfrac );
	#ifdef ANTI_MK
	switch	( v_exp )
		{
		case -1 : v_exp3 = 0;
			break;
		case 3: v_exp3 = 0;
			break;
		case 4:
		case 5: if ( qfrac > 0 ) v_exp3 = 0;
			break;
		}
	qfrac = v_exp3 - t_exp;
	if ( qfrac < 0 ) qfrac = 0;
	// printf("v_exp = %d   v_exp3 = %d    t-exp = %d --> qfrac = %d\n", v_exp, v_exp3, t_exp, qfrac );
	#endif
	}
	/// ----------------------- formatage -----------------------------------------------------
int cnt;
// premier cas trivial
if	( val == 0.0 )
	{ sprintf( lbuf, "0" ); return 1; }
// cas extremes : suffixes non disponibles
if	( ( v_exp3 < -15 ) || ( v_exp3 > 12 ) )
	{
	cnt = snprintf( lbuf, 30, "%g", val );
	return cnt;
	}
// application de l'exposant
if	( v_exp3 )
	{
	// val *= exp( -v_exp3 * log(10) );
	val *= opow[15 - v_exp3];
	}
// formatage en virgule fixe
cnt = snprintf( lbuf, 30, "%.*f", qfrac, val );
if	( cnt < 0 )	// debordement ?
	cnt = snprintf( lbuf, 30, "%g", val );
// second cas trivial : pas besoin de suffixe
if	( v_exp3 == 0 )
	return cnt;
// ajouter suffixe
sprintf( lbuf + cnt, "%c", osuff[ ( v_exp3/3 ) + 5 ] );
return( cnt + 1 );
}

/*
void scientout_test1( double val, double tick=0.0 )
{
char tbuf[32]; int cnt;
cnt = scientout( tbuf, val, tick );
printf("%10g, %10g, --> %2d : %s\n", val, tick, cnt, tbuf );
}

void scientout_tests()
{
// test v_exp3
scientout_test1( 100, 0.01 );
scientout_test1( 999.9, 0.01 );
scientout_test1( 1000, 0.01 );
scientout_test1( 0.001, 0.0001 );
scientout_test1( 0.999, 0.0001 );

scientout_test1( 1.1, 0.01 ); scientout_test1( 1.2 ); scientout_test1( 1.25 ); scientout_test1( 0 );
scientout_test1( 11.123, 0.02 ); scientout_test1( 13 ); scientout_test1( 25.01 ); scientout_test1( 0 );
scientout_test1( 500, 2 ); scientout_test1( 1200 ); scientout_test1( 12000 ); scientout_test1( 0 );
scientout_test1( -500, 2 ); scientout_test1( -1200 ); scientout_test1( -12000 ); scientout_test1( 0 );
scientout_test1( 0.1, 0.001 ); scientout_test1( -0.1 ); scientout_test1( -0.2 ); scientout_test1( 0 );

// test ANTI_MK (k)
scientout_test1( 1000, 1 ); scientout_test1( 11000 ); scientout_test1( -200 ); scientout_test1( 0 );
scientout_test1( 10000, 10 ); scientout_test1( 11000 ); scientout_test1( -200 ); scientout_test1( 0 );
scientout_test1( 10000, 1000 ); scientout_test1( 11000 ); scientout_test1( -200000 ); scientout_test1( 0 );
scientout_test1( 100000, 1 ); scientout_test1( 11001 ); scientout_test1( -99999 ); scientout_test1( 0 );
scientout_test1( 100000, 1000 ); scientout_test1( 11000 ); scientout_test1( -200000 ); scientout_test1( 0 );
// test ANTI_MK (m)
scientout_test1( 0.5, 0.001 ); scientout_test1( 1 ); scientout_test1( -2 ); scientout_test1( 0 );
scientout_test1( 0.0, 0.005 ); scientout_test1( 1 ); scientout_test1( -2 ); scientout_test1( 0 );
scientout_test1( 0.0, 0.01 ); scientout_test1( 1 ); scientout_test1( -2 ); scientout_test1( 0 );
scientout_test1( 0.0, 0.02 ); scientout_test1( 1 ); scientout_test1( -2 ); scientout_test1( 0 );
scientout_test1( 3, 0.001 ); scientout_test1( 1 ); scientout_test1( -2 ); scientout_test1( 0 );

// cas extremes
scientout_test1( -120e-15 , 5e-15 ); scientout_test1( 10e-15 ); scientout_test1( 33e-15 ); scientout_test1( 0 );
scientout_test1( -120e-19 , 5e-19 ); scientout_test1( 10e-18 ); scientout_test1( 33e-19 ); scientout_test1( 0 );
scientout_test1( 33e15, 2e15 ); scientout_test1( -1e15 ); scientout_test1( 200e15 ); scientout_test1( 0 );
scientout_test1( 33e12, 2e12 ); scientout_test1( -1e12 ); scientout_test1( 200e12 ); scientout_test1( 0 );
scientout_test1( 200000000 , 5e-15 ); scientout_test1( 100000 ); scientout_test1( 1e15 ); scientout_test1( 0 );
//
// scientout_test1(  ); scientout_test1(  ); scientout_test1(  ); scientout_test1( 0 );
}
*/



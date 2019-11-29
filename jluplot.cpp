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
   case 1 : dR = 1.0; dG = 0.0; dB = 0.0; break;
   case 2 : dR = 0.9; dG = 0.4; dB = 0.0; break;
   case 3 : dR = 0.7; dG = 0.9; dB = 0.0; break;
   case 4 : dR = 0.0; dG = 0.8; dB = 0.1; break;
   case 5 : dR = 0.0; dG = 0.0; dB = 1.0; break;
   case 6 : dR = 0.4; dG = 0.0; dB = 0.7; break;
   case 7 : dR = 0.7; dG = 0.0; dB = 0.3; break;
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
// premier multiple de tdr superieur ou egal a vmin
ftr = ceil( rmin / tdr );
ftr *= tdr;
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
	nn = la->NdeV(la->get_Vmin());
	if	( nn < nmin )
		nmin = nn;
	nn = la->NdeV(la->get_Vmax());
	if	( nn > nmax )
		nmax = nn;
	}
// seconde etape : action
double dn = nmax - nmin;
dn *= kmfn;	// marges t.q. 5% pour compatibilité visuelle avec jluplot 0
nmin -= dn; nmax += dn;
zoomN( nmin, nmax );
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
cairo_rectangle( cai, 0, 0, parent->ndx, ndy );
if   ( optcadre )
     cairo_stroke( cai );
else cairo_fill( cai );

// preparer le clip, englobant les graduations Y car il peut leur arriver de deborder
// mais pas celles de X car on ne veut pas que les courbes debordent dessus
cairo_rectangle( cai, -parent->mx, 0, parent->fdx, ndy );
cairo_clip( cai );

// translater l'origine Y en bas de la zone utile des courbes
// l'origine X est deja au bord gauche de cette zone
cairo_translate( cai, 0, ndy );


// les textes de l'axe Y
gradu_Y( cai );

// tracer les courbes
int i;
for ( i = ( courbes.size() - 1 ); i >= 0; i-- )
    {
    courbes.at(i)->ylabel = ( 20 * i ) + 20;
    courbes.at(i)->draw( cai );
    }

// tracer les reticules
cairo_set_source_rgb( cai, lncolor.dR, lncolor.dG, lncolor.dB );
reticule_Y( cai );
reticule_X( cai );

cairo_reset_clip( cai );

// les textes de l'axe X
if	( optX )
	gradu_X( cai );

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
double curq = parent->ftq;
double curx = parent->XdeM(parent->MdeQ(curq));		// la transformation
while	( curx < parent->ndx )
	{
	cairo_move_to( cai, curx, -((double)ndy ) );	// top
	// cairo_line_to( cai, curx, (optX)?(5.0):(0.0) ); // bottom (le 5 sert a rien sur drawpad)
	cairo_line_to( cai, curx, 0.0 );	// bottom
	cairo_stroke( cai );
	curq += parent->tdq;
	curx = parent->XdeM(parent->MdeQ(curq));	// la transformation
	}
}

// les textes de l'axe X
void strip::gradu_X( cairo_t * cai )
{
char lbuf[32];
cairo_set_source_rgb( cai, 1, 1, 1 );
cairo_rectangle( cai, -parent->mx, 0, parent->fdx, fdy - ndy );
cairo_fill( cai );
cairo_set_source_rgb( cai, 0, 0, 0 );
double curq = parent->ftq;
double curx = parent->XdeM(parent->MdeQ(curq));		// la transformation
// preparation format selon premier point
scientout( lbuf, curq, parent->tdq );
while	( curx < parent->ndx )
	{
	scientout( lbuf, curq );
	cairo_move_to( cai, curx - 20, 15 );
	cairo_show_text( cai, lbuf );
	curq += parent->tdq;
	curx = parent->XdeM(parent->MdeQ(curq));	// la transformation
	}
}

// tracer le reticule y : la grille de barres horizontales
void strip::reticule_Y( cairo_t * cai )
{
double curr = ftr;
double cury = -YdeN(NdeR(curr));		// la transformation
while	( cury > -((double)ndy ) )		// le haut (<0)
	{
	cairo_move_to( cai, -6, cury );// le -6 c'est pour les graduations aupres des textes
	//cairo_move_to( cai, 0.0, cury );	// sauf que c'est clippe par le drawpad !!
	cairo_line_to( cai, parent->ndx, cury );
	cairo_stroke( cai );
	curr += tdr;
	cury = -YdeN(NdeR(curr));		// la transformation
	}
}

// les textes de l'axe Y
void strip::gradu_Y( cairo_t * cai )
{
char lbuf[32]; int mx;
mx = parent->mx;
cairo_set_source_rgb( cai, 1, 1, 1 );
cairo_rectangle( cai, -mx, -ndy, mx, ndy );
cairo_fill( cai );
cairo_set_source_rgb( cai, 0, 0, 0 );
double curr = ftr;
double cury = -YdeN(NdeR(curr));		// la transformation
// preparation format selon premier point
scientout( lbuf, curr, tdr );
while	( cury > ( -((double)ndy) + 19.0 ) )		// petite marge pour label axe
	{
	scientout( lbuf, curr );
	cairo_move_to( cai, -mx + 10.0, cury + 5 );
	cairo_show_text( cai, lbuf );
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
// premier multiple de tdq superieur ou egal a qmin
ftq = ceil( qmin / tdq );
ftq *= tdq;
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
int ndy, bndy; unsigned int ib;
fdx = redx;
ndx = fdx - mx;
ndy = fdy = redy;
if	( bandes.size() )
	{
	// soustraire l'espace pour les graduations optionnelles
	for	( ib = 0; ib < bandes.size(); ib++ )
       		ndy -= ((bandes.at(ib)->optX)?(my):(0));
	// bandes d'egale hauteur
	bndy = ndy / bandes.size();
	// reste de la division applique a la premiere bande
	bandes.at(0)->presize( bndy + ( ndy % bandes.size() ) );
	// les autres bandes
	for	( ib = 1; ib < bandes.size(); ib++ )
		bandes.at(ib)->presize( bndy );
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
double M0, M1; int ndy, bndy; unsigned int ib;
// horizontalement
M0 = MdeX( 0.0 ); M1 = MdeX( ndx );
fdx = redx;
ndx = fdx - mx;
zoomM( M0, M1 );
// verticalement
ndy = fdy = redy;
if	( bandes.size() )
	{
	// soustraire l'espace pour les graduations
	for	( ib = 0; ib < bandes.size(); ib++ )
       		ndy -= ((bandes.at(ib)->optX)?(my):(0));
	// bandes d'egale hauteur
	bndy = ndy / bandes.size();
	// reste de la division applique a la premiere bande
	bandes.at(0)->resize( bndy + ( ndy % bandes.size() ) );
	// les autres bandes
	for	( ib = 1; ib < bandes.size(); ib++ )
		bandes.at(ib)->resize( bndy );
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
	for	( unsigned int ic = 0; ic < b->courbes.size(); ic++ )
		{
		la = b->courbes.at(ic);
		mm = la->MdeU(la->get_Umin());
		if	( mm < mmin )
			mmin = mm;
		mm = la->MdeU(la->get_Umax());
		if	( mm > mmax )
			mmax = mm;
		}
	}
// seconde etape : action
fullmmin = mmin; fullmmax = mmax;
printf("in fullM, full is %g to %g\n", fullmmin, fullmmax );
zoomM( mmin, mmax );
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
unsigned int i;

cairo_save( cai );
cairo_translate( cai, mx, 0 );
// on veut la courbe 0 en haut...
for ( i = 0; i < bandes.size(); i++ )
    {
    bandes.at(i)->draw( cai );
    cairo_translate( cai, 0, bandes.at(i)->fdy );
    }
cairo_restore( cai );
}

// cadrage auto si dvtot <= 0.0, sinon "centerY"
// rend 0 si Ok
int panel::pdfplot( const char * fnam, const char * caption )
{
cairo_surface_t * cursurf;
cairo_status_t cairo_status;
cairo_t * lecair;
double pdf_w, pdf_h;

// format A4 landscape
pdf_w = ( 297.0 / 25.4 ) * 72;
pdf_h = ( 210.0 / 25.4 ) * 72;

cursurf = cairo_pdf_surface_create( fnam, pdf_w, pdf_h );

cairo_status = cairo_surface_status( cursurf );
if   ( cairo_status != CAIRO_STATUS_SUCCESS )
     return( cairo_status );

lecair = cairo_create( cursurf );

// un peu de marge (en points)
cairo_translate( lecair, 24.0, 24.0 );

resize( (int)pdf_w - 48, (int)pdf_h - 72 );

// preparer font a l'avance
cairo_select_font_face( lecair, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
cairo_set_font_size( lecair, 12.0 );
cairo_set_line_width( lecair, 0.5 );

// draw the curves
draw( lecair );

// the caption
cairo_move_to( lecair, 10.0, pdf_h - 72.0 + 24 );
cairo_show_text( lecair, caption );

cairo_destroy( lecair );
cairo_surface_destroy( cursurf );
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
for ( istrip = 0; istrip < (int)bandes.size(); istrip++ )
    {
    if  ( bandes.at(istrip)->clicY( y, py ) == 0 )
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

// conversion double-->texte, avec un format de precision
// en rapport avec la valeur du tick.
// si la valeur du tick est nulle ou absente, utilisation
// du format anterieur memorise
// retour = nombre de chars utiles
// lbuf doit pouvoir contenir 32 bytes
int scientout( char * lbuf, double val, double tick )
{
//                          0123456789
static const char *osuff = "fpnum kMGT";
// ces deux valeurs memorisent le format
static int triexp = 0;	// exposant, multiple de 3
static int preci = 1;	// nombre digits apres dot
if ( tick > 0.0 )	// preparation format
   {
   int tikexp; double aval;
   // normalisation
   aval = fabs(val);
   if ( aval < tick )	// couvre le cas val == 0.0
      aval = tick;
   triexp = (int)floor( log10( aval ) / 3 );
   triexp *= 3;
   // ordre de grandeur du tick (en evitant arrondi du 1 vers 0.99999)
   tikexp = (int)floor( log10(tick) + 1e-12 );
   preci = triexp - tikexp;
   if ( preci < 1 ) preci = 1;
   // printf("~>%d <%d> .%d\n", triexp, tikexp, preci );
   }
if ( val == 0.0 )
   { sprintf( lbuf, "0" ); return 1; }
unsigned int cnt;	// affichage selon format
if ( triexp )
   val *= exp( -triexp * log(10) );
// int snprintf (char *s, size t size, const char *template, . . . )
// rend le nombre de chars utiles
cnt = snprintf( lbuf, 20, "%.*f", preci, val );
if ( triexp == 0 )
   return cnt;
if ( ( triexp < -15 ) || ( triexp > 12 ) )
   {
   cnt += snprintf( lbuf + cnt, 8, "e%d", triexp );
   return cnt;
   }
sprintf( lbuf + cnt, "%c", osuff[ ( triexp/3 ) + 5 ] );
return( cnt + 1 );
}

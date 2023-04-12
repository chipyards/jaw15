// layer_u : une courbe a pas uniforme (classe derivee de layer_base)
// supporte 3 styles :
// 0 : style par defaut : courbe classique
// 1 : style spectre en barres verticales
// 2 : courbe classique en dB
// donnees allouees a l'exterieur, de type Tsamp parametrable
// teste avec double, float, short, unsigned short, Tsamp
// (no destructor needed , Tsamp * V is allocated and freed by app)
template <typename Tsamp> class layer_u : public layer_base {
public :

Tsamp * V;
double Vmin, Vmax;
int qu;		// nombre de points effectif
int curi;	// index point courant
int style;
int ecostroke;	// limite de stroke context
double k0dB;	// specifique style 2, k0dB = 1 / V0dB, a fournir par l'appli
		// exemple FFT sur audio : k0dB = 1.0 / ( 32767.0 * ( qu-1 ) ) car qu-1 = fftsize/2
double Vfloor;	// specifique style 2, plancher pour dB, a fournir par l'appli

// constructeur
layer_u<Tsamp>() : layer_base(),
	  Vmin(0), Vmax(1), qu(0), curi(0), style(0), ecostroke(400), k0dB(1.0), Vfloor(1e-5) {};

// methodes propres a cette classe derivee

int goto_U( double U0 ) {		// chercher le premier point U >= U0
	curi = (int)ceil(U0);
	if ( curi < 0 ) curi = 0;
	if ( curi < qu ) return 0;
	else		 return -1;
	};

int get_pi( double & rU, double & rV ) {	// get XY then post increment
	if ( curi >= qu ) return -1;
	rU = (double)curi; rV = (double)V[curi]; ++curi;
	return 0;
	};

void scan();				// mettre a jour les Min et Max

// les methodes qui sont virtuelles dans la classe de base
double get_Umin() { return (double)0; };
double get_Umax() { return (double)(qu-1); };
double get_Vmin() { return Vmin; };
double get_Vmax() { return Vmax; };

void draw( cairo_t * cai );	// dessin
};	// class

// chercher les Min et Max
template <typename Tsamp> void layer_u<Tsamp>::scan()
{
if	( qu )
	{
	Vmin = Vmax = V[0];
	}
int i; double v;
if	( style != 2 )
	{
	for ( i = 1; i < qu; i++ )
	    {
	    v = (double)V[i];
	    if ( v < Vmin ) Vmin = v;
	    if ( v > Vmax ) Vmax = v;
	    }
	}
else	{
	Vmin = 1000.0; Vmax = -1000.0;
	for	( i = 1; i < qu; i++ )
		{
		v = k0dB * (double)V[i];
		if ( v < Vfloor ) v = Vfloor;
		v = 20.0 * log10( v );
		if ( v < Vmin ) Vmin = v;
		if ( v > Vmax ) Vmax = v;
		}
	}
// printf("%g < V < %g\n", Vmin, Vmax );
}


// dessin (ses dimensions dx et dy sont lues chez les parents)
template <typename Tsamp> void layer_u<Tsamp>::draw( cairo_t * cai )
{
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
cairo_show_text( cai, label.c_str() );

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double tU, tV, curx, cury, maxx;
int cnt = 0;	// limiteur de stroke context

if ( goto_U( u0 ) )
   return;
maxx = (double)(parent->parent->ndx);

if	( style == 0 )
	{				// style par defaut : courbe classique
	if	( get_pi( tU, tV ) )
   		return;
	curx =  XdeU( tU );			// on a le premier point ( tU, tV )
	cury = -YdeV( tV );			// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );		// les transformations
		cury = -YdeV( tV );
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		if	( ++cnt >= ecostroke )
			{
			cairo_stroke( cai );
			cnt = 0;
			cairo_move_to( cai, curx, cury );
			}
		}
	}
else if	( style == 1 )
	{				// style spectre en barres verticales
	double zeroy = -YdeV( 0.0 );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );		// les transformations
		cury = -YdeV( tV );
		cairo_move_to( cai, curx, zeroy );	// special spectro en barres
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		if	( ++cnt >= ecostroke )
			{
			cairo_stroke( cai );
			cnt = 0;
			}
		}
	}
else if	( style == 2 )
	{				// courbe classique en dB
	if	( get_pi( tU, tV ) )
   		return;
	curx =  XdeU( tU );
	tV = k0dB * tV;
	if ( tV < Vfloor ) tV = Vfloor;
	cury = -YdeV( 20.0 * log10( tV ) );	// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	while	( get_pi( tU, tV ) == 0 )
		{
		curx =  XdeU( tU );
		tV = k0dB * tV;
		if ( tV < Vfloor ) tV = Vfloor;
		cury = -YdeV( 20.0 * log10( tV ) );
		cairo_line_to( cai, curx, cury );
		if	( curx >= maxx )
			break;
		if	( ++cnt >= ecostroke )
			{
			cairo_stroke( cai );
			cnt = 0;
			cairo_move_to( cai, curx, cury );
			}
		}
	}
if	( cnt )
	cairo_stroke( cai );
}

// classe derivee de gstrip, pour reticule midi keyboard
//	les "touches blanches" sont blanches
class strip_x_midi : public gstrip {
public :

// constructeur
strip_x_midi() : gstrip() { optcadre = 1; };

// methode
void draw( cairo_t * cai ) {
	// printf("0) ndx=%u, ndy=%u\n", parent->ndx, ndy );
	cairo_save( cai );
	// cairo_reset_clip( cai );
	// cairo_rectangle( cai, 0, 0, parent->fdx, ndy );
	// cairo_clip( cai );
	// fond provisoire
	// cairo_set_source_rgb( cai, 1.0, 1.0, 0.0 );
	// cairo_rectangle( cai, 0, 0, parent->ndx, ndy );
	// cairo_fill( cai );

	// '0' = blanche, '1' = noire, '2' = blanche speciale : Do et Fa, pour distinguer de Si et Mi
	const char * blacknotes = "210102101010";	// midinote = 0 est un Do
	// Q = graduation en midinotes, comme M (au fine tuning q0 près)
	double curq = floor( parent->QdeM( parent->MdeX( 0 ) ) );
	// printf("q=%g\n", curq ); fflush(stdout);
	// on a recupere la premiere midinote dans le cadre ( avec l'aide de ceil() )
	int midinote = (int)curq;
	// ses X
	double x0 = parent->XdeM(parent->MdeQ(curq-0.5) );
	double x1 = parent->XdeM(parent->MdeQ(curq+0.5) );
	double dx = x1 - x0;
	while	( x1 < 0 )			// skip note fully out
		{ x0 = x1; x1 += dx; midinote += 1; }
	if	( x0 < 0 )				// clip note partially out
		x0 = 0;
	while	( x0 < parent->ndx )
		{
		if	( x1 > parent->ndx )	// clip note partially out
			x1 = parent->ndx;
		int selector = blacknotes[ midinote % 12 ] & 3;
		if	( selector == 1 )				// noire
			cairo_set_source_rgb( cai, bgcolor.dR, bgcolor.dG, bgcolor.dB );
		else if	( selector == 2 )
			cairo_set_source_rgb( cai, 0.94, 1.0, 0.94 );	// blanche speciale	
		else	cairo_set_source_rgb( cai, 1.0, 1.0, 1.0 );	// blanche
		cairo_rectangle( cai, x0, 0, x1-x0, ndy );
		cairo_fill( cai );
		x0 = x1; x1 += dx; midinote += 1;
		}
	cairo_restore( cai );
	gstrip::draw( cai );
	};

};


/* tout est la yapuka

// les textes de l'axe X avec les taquets
void strip::gradu_X( cairo_t * cai )
{
char lbuf[32];
cairo_set_source_rgb( cai, 1, 1, 1 );
cairo_rectangle( cai, -parent->mx, 0, parent->fdx, fdy - ndy );
cairo_fill( cai );
cairo_set_source_rgb( cai, 0, 0, 0 );
double curq = parent->tdq * ceil( parent->QdeM( parent->MdeX( 0 ) ) / parent->tdq );
double curx = parent->XdeM(parent->MdeQ(curq));		// la transformation
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
*/

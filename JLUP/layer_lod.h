// layer_lod : une courbe a pas uniforme (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)
// teste avec short et float
// (no destructor needed , Tsamp * V is allocated and freed by app)

// chaque lod a un buffer qu'il alloue, lequel sera libere iterativement
// par le destructeur de layer_lod

template <typename Tsamp> class lod {	// un niveau de detail
public :
Tsamp * max;
Tsamp * min;	// des couples min_max
int qc;		// nombre de couples min_max
int kdec;	// decimation par rapport a l'audio original
// constructeur
lod() : max(NULL), min(NULL), qc(0), kdec(1) {
	// printf("lod %p constructor\n", this ); fflush(stdout);
	};
// destructeur TRES MAUVAISE IDEE : les lods mis dans un vector sont sujet a copies intempestives
//~lod() { if (min) free(min); }; // a chaque copie le destructor de l'original est appelé

// methodes
void allocMM( size_t size ) {	// allouer buffers min et max (max est contenu dans min pour simplifier)
	min = (Tsamp *)malloc( 2 * size * sizeof(Tsamp) );
	max = min + size;
	// printf("lod %p alloc  %p : %p, size %08x\n", this, min, max, size ); fflush(stdout);
	};
};

template <typename Tsamp> class layer_lod : public layer_base {
public :

Tsamp * V;
Tsamp Vmin, Vmax;
int qu;			// nombre de points effectif
int curi;		// index point courant
double linewidth;	// 0.5 pour haute resolution, 1.0 pour bon contraste
double spp_max;		// nombre max de sample par pixel (samples audio ou couple minmax)
			// s'il est trop petit, on a une variation de contraste au changement de LOD
			// s'il est trop grand on ralentit l'affichage
			// bon compromis  : 0.9 / linewidth
vector <lod<Tsamp> > lods;	// autant le LODs que necessaire
int ilod;		// indice du LOD courant, -1 pour affichage a pleine resolution
			// -2 pour inconnu (choix ilod a faire)

// constructeur
layer_lod() : layer_base(), V(NULL), Vmin(0), Vmax(1),
		qu(0), curi(0), linewidth(0.7), spp_max(0.9/linewidth),
		ilod(-2) {};
// destructeur : il doit liberer la memoire des lods - ceux-ci seront detruits ensuite automatiquement
~layer_lod() {
	printf("layer_lod destructor, about to free %d lods\n", lods.size() ); fflush(stdout);
	for	( unsigned int i = 0; i < lods.size(); ++i )
		if	( lods[i].min ) free( lods[i].min );
	};

// methodes propres a cette classe derivee
int make_lods( unsigned int klod1, unsigned int klod2, unsigned int maxwin );	// allouer et calculer les LODs
void scan();			// mettre a jour les Min et Max
				// aussi fait  par make_lods()
void find_ilod();		// choisir le LOD optimal (ilod) en fonction du zoom
int goto_U( double U0 );	// chercher le premier point U >= U0
void goto_first() {
	curi = 0;
	};
int get_pi( double & rU, double & rV ) {	// get XY then post increment
	if ( curi >= qu ) return -1;
	rU = (double)curi; rV = (double)V[curi]; ++curi;
	return 0;
	};
void draw( cairo_t * cai, double tU0, double tU1 );	// dessin partiel

// les methodes qui sont virtuelles dans la classe de base
void refresh_proxy() {
	ilod = -2;
	layer_base::refresh_proxy();
	};
double get_Umin() { return (double)0; };
double get_Umax() { return (double)(qu-1); };
double get_Vmin() { return (double)Vmin; };
double get_Vmax() { return (double)Vmax; };

void draw( cairo_t * cai ) {				// dessin full strip
	double u1 = UdeX( (double)(parent->parent->ndx) );
	draw( cai, u0, u1 );
	// printf("d lod %d\n", ilod ); fflush(stdout);
	};
};

// layer_lod : une courbe a pas uniforme en Tsamp (classe derivee de layer_base)
// supporte multiples LOD (Level Of Detail)

// allouer et calculer les LODs - un LOD est une fonction enveloppe representable par des barres verticales
//	klod1 = premiere decimation, utilisee pour passer de l'audio a la premiere enveloppe
//	klod2 = decimations ulterieures
//	maxwin = plus grande largeur de fenetre typiquement rencontree
// on cree des lods de plus en plus petits jusqu'a ce que
// la taille passe en dessous de klod2 * maxwin, alors c'est fini.
template <typename Tsamp> int layer_lod<Tsamp>::make_lods( unsigned int klod1, unsigned int klod2, unsigned int maxwin )
{
unsigned int lodsize;
unsigned int i;		// indice source
unsigned int j;		// sous-indice decimation
unsigned int k;		// indice destination
Tsamp min=0, max=0;
lod<Tsamp> * curlod, * prevlod;
lods.reserve(12);	// evite plein de copies de lods

// ------------------------------ premiere decimation
lodsize = qu / klod1;
if	( lodsize <= maxwin )
	{ scan(); return 0; }	// meme pas besoin de lod dans ce cas
lods.push_back( lod<Tsamp>() );
curlod = &lods.back();
curlod->kdec = klod1;
curlod->qc = lodsize;
printf("lod %p #0 : k = %6d size = %d\n", curlod, curlod->kdec, curlod->qc ); fflush(stdout);
curlod->allocMM( lodsize );
if	( ( curlod->min == NULL ) || ( curlod->max == NULL ) )
	return -1;
i = j = k = 0;
if	( qu )
	Vmin = Vmax = V[0];
while	( i < ( (unsigned int)qu - 1 ) )
	{
	if	( V[i] < Vmin ) Vmin = V[i];	// remplace scan()
	else if	( V[i] > Vmax ) Vmax = V[i];
	if	( j == 0 )
		min = max = V[i];
	else	{
		if	( V[i] < min ) min = V[i];
		else if	( V[i] > max ) max = V[i];
		}
	i += 1; j += 1;
	if	( j == klod1 )
		{		// c'est aussi le premier pt de la prochaine decimation...
		if	( V[i] < min ) min = V[i];	// c'est plus esthetique, c'est tout
		else if	( V[i] > max ) max = V[i];
		curlod->min[k] = min;
		curlod->max[k] = max;
		k += 1;
		if	( k >= lodsize )
			break;
		j = 0;
		}
	}
if	( k < lodsize )
	{
	curlod->min[k] = min;
	curlod->max[k] = max;
	++k;
	}
// printf("lod %p #0 : fini size = %d(%d)\n", curlod, curlod->qc, k );
// printf("fini @ %p : %p, size %d\n", curlod->min, curlod->max, curlod->qc );
// ------------------------------ decimations suivantes
while	( ( lodsize = lodsize / klod2 ) > maxwin )
	{
	lods.push_back( lod<Tsamp>() );
	// prevlod = curlod;				// marche PO curlod est invalide
	prevlod = &lods.at( lods.size() - 2 );		// APRES le push_back sinon t'es DEAD
	curlod = &lods.back();
	curlod->kdec = prevlod->kdec * klod2;
	curlod->qc = lodsize;
	printf("lod %p #%d : k = %6d size = %d\n", curlod, (int)lods.size()-1, curlod->kdec, curlod->qc ); fflush(stdout);
	curlod->allocMM( lodsize );
	if	( ( curlod->min == NULL ) || ( curlod->max == NULL ) )
		return -1;
	i = j = k = 0;
	while	( i < (unsigned int)prevlod->qc )
		{
		if	( j == 0 )
			{ min = prevlod->min[i]; max = prevlod->max[i]; }
		else	{
			if	( prevlod->min[i] < min ) min = prevlod->min[i];
			if	( prevlod->max[i] > max ) max = prevlod->max[i];
			}
		i += 1; j += 1;
		if	( j == klod2 )
			{
			curlod->min[k] = min;
			curlod->max[k] = max;
			k += 1;
			if	( k >= lodsize )
				break;
			j = 0;
			}
		}
	if	( k < lodsize )
		{
		curlod->min[k] = min;
		curlod->max[k] = max;
		++k;
		}
	// printf("lod %p : fini size = %d(%d)\n", curlod, curlod->qc, k );
	}
return 0;
}

template <typename Tsamp> void layer_lod<Tsamp>::scan()
{
if	( qu )
	Vmin = Vmax = V[0];
for	( int i = 1; i < qu; ++i )
	{
	if	( V[i] < Vmin ) Vmin = V[i];
	else if	( V[i] > Vmax ) Vmax = V[i];
	}
}

// chercher le premier point X >= X0
template <typename Tsamp> int layer_lod<Tsamp>::goto_U( double U0 )
{
curi = (int)ceil(U0);
if	( curi < 0 )
	curi = 0;
if	( curi < qu )
	return 0;
else	return -1;
}

// choisir le LOD optimal (ilod) en fonction du zoom horizontal c'est a dire ( u1 - u0 )
template <typename Tsamp> void layer_lod<Tsamp>::find_ilod()
{
double u1, maxx, spp=100000.0;

maxx = (double)(parent->parent->ndx);
u1 = UdeX( maxx );
// commencer par le LOD le plus grossier, affiner jusqu'a avoir assez de samples
// pour un bon effet visuel
ilod = lods.size() - 1;
while	( ilod >= 0 )
	{
	spp = ( u1 - u0 ) / ( lods.at(ilod).kdec * maxx );	// samples par pixel
	if	( spp >= spp_max )
		break;
	ilod -= 1;
	}
if	( ilod < 0 )
	spp = ( u1 - u0 ) / maxx;
// printf("choix lod %d, spp = %g\n", ilod, spp );
}

// dessin partiel de tU0 a tU1
template <typename Tsamp> void layer_lod<Tsamp>::draw( cairo_t * cai, double tU0, double tU1 )
{
// printf("layer_lod::begin draw\n");
cairo_set_source_rgb( cai, fgcolor.dR, fgcolor.dG, fgcolor.dB );
cairo_set_line_width( cai, linewidth );
if	( tU0 == u0 )
	{
	cairo_move_to( cai, 20, -(parent->ndy) + ylabel );
	cairo_show_text( cai, label.c_str() );
	}

// l'origine est en bas a gauche de la zone utile, Y+ est vers le bas (because cairo)
double tU, tV, curx, cury, cury2;
int cnt;	// limiteur de stroke context

if	( ilod < -1 )
	find_ilod();

if	( ilod < 0 )
	{			// affichage de courbe classic
	if ( goto_U( tU0 ) )
	   return;
	if ( get_pi( tU, tV ) )
	   return;
	// on a le premier point ( tU, tV )
	curx =  XdeU( tU );			// les transformations
	cury = -YdeV( tV );			// signe - ici pour Cairo
	cairo_move_to( cai, curx, cury );
	cnt = 0;
	while ( get_pi( tU, tV ) == 0 )
	   {
	   curx =  XdeU( tU );		// les transformations
	   cury = -YdeV( tV );
	   cairo_line_to( cai, curx, cury );
	   if ( tU >= tU1 )	// anciennement if ( curx >= maxx )
	      break;
	   if	( ++cnt >= 400 )
		{
		cairo_stroke( cai );
		// printf("courbe %d lines\n", cnt );
		cnt = 0;
		cairo_move_to( cai, curx, cury );
		}
	   }
	if	( cnt )
		cairo_stroke( cai );
	// printf("courbe %d lines\n", cnt );
	}
else	{			// affichage enveloppe
	lod<Tsamp> * curlod = &lods.at(ilod);
	int qc = curlod->qc;
	int k = curlod->kdec;
	int i0, i1, i;		// indices source
	i0 = (int)ceil(tU0) / k;
	i1 = (int)floor(tU1) / k;
	if	( i0 < 0 ) i0 = 0;
	if	( i1 > ( qc - 1 ) ) i1 = qc - 1;
	cnt = 0;
	for	( i = i0; i < i1; ++i )
		{
		curx =  XdeU( i * k );
		/* version de base : l'enveloppe est remplie de hachures verticales 
		cury = -YdeV( curlod->min[i] );
		cairo_move_to( cai, curx, cury );
		cury = -YdeV( curlod->max[i] );
		cairo_line_to( cai, curx, cury );
		*/
		// pb : une ligne horizontale a une enveloppe d'epaisseur nulle...
		// version améliorée destinée a éviter l'évanescence des traces proches de l'horizontale
		cury2 = -YdeV( curlod->min[i] );	// ainsi normalement cury2 > cury
		cury  = -YdeV( curlod->max[i] );
		if	( cury2 < ( cury + linewidth ) ) // epaisseur ligne horizontale
			cury2 = cury + linewidth;	 // coherente avec les verticales
		cairo_move_to( cai, curx, cury );
		cairo_line_to( cai, curx, cury2 );
		if	( ++cnt >= 400 )
			{
			cairo_stroke( cai );
			// printf("striken %d lines\n", cnt );
			cnt = 0;
			cairo_move_to( cai, curx, cury2 );
			}
		}
	if	( cnt )
		cairo_stroke( cai );
	// printf("enveloppe %d lines\n", i1 - i0 );
	}
// printf("end layer_lod::draw\n");
}

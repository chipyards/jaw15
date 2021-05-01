// simple buffer a allocation automatique

template <typename Tsamp> class autobuf {
public :
Tsamp * data;
size_t capa;	// en samples
size_t size;

// constructeur
autobuf() :  data(NULL), capa(0), size(0) {};

// destructeur
~autobuf() { 
	if	( data )
		free( data );
	};

// methode d'allocation (amount est en samples)
// rend 0 si ok
int more( unsigned int amount ) {
	if	( capa == 0 )
		{
		capa = amount;
		data = (Tsamp *)malloc( capa * sizeof(Tsamp) );
		}
	else	{
		capa += amount;
		data = (Tsamp *)realloc( (void *)data, capa * sizeof(Tsamp) );
		}
	if	( data == NULL )
		{ capa = 0; return -1; }
	return 0;
	};
void reset() {
	if	( capa )
		free( data );
	capa = 0; data = NULL;
	};
};

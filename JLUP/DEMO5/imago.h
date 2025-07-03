class imago {
public:
GdkPixbuf * pix1;	// image originale
GdkPixbuf * pix2;	// image retouchee
unsigned int w1;	// dimensions original
unsigned int h1;

// constructeur
imago() : pix1(NULL), pix2(NULL) {
	};

int filter( fir * zefir );		// filtrer pix1 -> pix2
int read( const char * fnam );		// lire pix1 (rend 0 si ok)
int save_png( const char * fnam );	// sauver pix2 (rend 0 si ok)
};

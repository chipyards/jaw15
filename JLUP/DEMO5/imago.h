class imago {
public:
GdkPixbuf * pix1;	// image originale
GdkPixbuf * pix2;	// image retouchee
double * Abuf;		// buffer image entiere 1 composante
double * Bbuf;		// buffer image entiere 1 composante 
fir * lefir;		// proxy pour le filtre

// constructeur
imago() : pix1(NULL), pix2(NULL), Abuf(NULL), Bbuf(NULL) {
	};

// filtrer une ligne (verticale ou horizontale), tout en double
void filter_one_line( double * Dbuf, int di0, int di1, int dstride,
		      double * Sbuf, int si0, int si1, int sstride ) {
	double K = lefir->getK0();
	int di, si, sif;	// dest index, src index, src index pour 1 coeff
	int j, j0;	// coeff index
	double sum;
	j0 = (lefir->qfir-1)/2;		// qfir est impair (sauf si A0 != 0.0)
	si = si0;
	for	( di = di0; di < di1; di += dstride )
		{
		sum = 0.0;
		sif = si - j0 * sstride ;
		// N.B. j n'est pas utilise pour calculer un index, c'est juste pour compter
		for	( j = 0; j < (int)lefir->qfir; j += 1 )
			{
			/*
			if	( ( sif >= si0 ) && ( sif < si1 ) )
				sum += Sbuf[sif] * lefir->FIRbuf[j];
			// else rien, on complete au bord avec 0 = gris median
			*/
			if	( sif < si0 )	// traitement des bords plus approprie pour image
				sum += Sbuf[si0] * lefir->FIRbuf[j];		// copier bord min
			else if ( sif >= si1 )
				sum += Sbuf[si1-sstride] * lefir->FIRbuf[j];	// copier bord max
			else	sum += Sbuf[sif] * lefir->FIRbuf[j];		// normal
			sif += sstride;
			}
		Dbuf[di] = sum * K;
		si += sstride;
		}
	};

// filtrer un plan (une composante) horizontalement), tout en double
// src et dest de dimensions identiques
void filter_one_planeH( double * Dbuf, double * Sbuf, int W, int H ) {
	int Q = W * H;
	int si = 0;	// start fo each line
	for	( si = 0; si < Q; si += W )
		{
		filter_one_line( Dbuf, si, si + W, 1,
				 Sbuf, si, si + W, 1 );
		}
	};

// filtrer un plan (une composante) verticalement), tout en double
// src et dest de dimensions identiques
// NE FAIT RIEN (tout gris) mais le call est ok
void filter_one_planeV( double * Dbuf, double * Sbuf, int W, int H ) {
	int Q = W * H;
	int si = 0;	// start fo each line
	for	( si = 0; si < W; si += 1 )
		{
		filter_one_line( Dbuf, si, si + Q, W,
				 Sbuf, si, si + Q, W );
		}
	};

/* solution plus generale mais bon *
// filtrer un plan (une composante) (verticalement ou horizontalement), tout en double
// istride pour inner loop, ostride pour outer loop 
void filter_one_plane( double * Dbuf, int idstride, int odstride, int idsize, int odsize,
		       double * Sbuf, int isstride, int osstride, int issize, int ossize ) {
	int di, si;	// dest index, src index
	// outer loop
	si = 0;
	for	( di = 0; di < odsize; di += odstride )
		{
		// inner loop
		filter_one_line( Dbuf, di, di + idsize * idstride, idstride,
				 Sbuf, si, si + issize * isstride, isstride );
		si += osstride;
		}
	}
//*/

void filter( fir * zefir );		// filtrer pix1 -> pix2
int read( const char * fnam );		// lire pix1 (rend 0 si ok)
int save_png( const char * fnam );	// sauver pix2 (rend 0 si ok)
};

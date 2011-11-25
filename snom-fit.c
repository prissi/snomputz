#define NRANSI

#include <math.h>
#include "nrutil.h"

#include "myportab.h"
#include "snom-typ.h"
#include "snom-win.h"
#define nrerror( a )	  Fehler( a )


#undef	SWAP
#define SWAP( a, b ) {temp = ( a ); ( a ) = ( b ); ( b ) = temp;}
BOOL gaussj( double **a, int n, double **b, int m )
{
	int indxc[10], indxr[10], ipiv[10];
	int i, icol, irow, j, k, l, ll;
	double big, dum, pivinv, temp;

	for( j = 0; j < n; j++ ) {
		ipiv[j] = 0;
	}
	for( i = 0; i < n; i++ ) {
		big = 0.0;
		for( j = 0; j < n; j++ ) {
			if( ipiv[j] != 1 ) {
				for( k = 0; k < n; k++ ) {
					if( ipiv[k] == 0 ) {
						if( fabs( a[j][k] ) >= big ) {
							big = fabs( a[j][k] );
							irow = j;
							icol = k;
						}
					}
					else if( ipiv[k] > 1 ) {
						nrerror( "gaussj: Singular Matrix (1)" );
						return ( FALSE ) ;
					}
				}
			}
		}
		++( ipiv[icol] );
		if( irow != icol ) {
			for( l = 0; l < n; l++ ) {
				SWAP( a[irow][l], a[icol][l] )
			}
			for( l = 0; l < m; l++ ) {
				SWAP( b[irow][l], b[icol][l] )
			}
		}
		indxr[i] = irow;
		indxc[i] = icol;
		if( a[icol][icol] == 0.0 ) {
			nrerror( "gaussj: Singular Matrix (2)" );
			return ( FALSE );
		}
		pivinv = 1.0/a[icol][icol];
		a[icol][icol] = 1.0;
		for( l = 0; l < n; l++ ) {
			a[icol][l] *= pivinv;
		}
		for( l = 0; l < m; l++ ) {
			b[icol][l] *= pivinv;
		}
		for( ll = 0; ll < n; ll++ ) {
			if( ll != icol ) {
				dum = a[ll][icol];
				a[ll][icol] = 0.0;
				for( l = 0; l < n; l++ ) {
					a[ll][l] -= a[icol][l]*dum;
				}
				for( l = 0; l < m; l++ ) {
					b[ll][l] -= b[icol][l]*dum;
				}
			}
		}
	}
	for( l = n-1; l >= 0; l-- ) {
		if( indxr[l] != indxc[l] ) {
			for( k = 0; k < n; k++ ) {
				SWAP( a[k][indxr[l]], a[k][indxc[l]] );
			}
		}
	}
	return TRUE;    //Ok
}
// 3.8.98 aus NR


void covsrt( double **covar, int ma, int ia[], int mfit )
{
	int i, j, k;
	double temp;

	for( i = mfit+1; i <= ma; i++ ) {
		for( j = 1; j <= i; j++ ) {
			covar[i][j] = covar[j][i] = 0.0;
		}
	}
	k = mfit;
	for( j = ma; j >= 1; j-- ) {
		if( ia[j] ) {
			for( i = 1; i <= ma; i++ ) {
				SWAP( covar[i][k], covar[i][j] )
			}
			for( i = 1; i <= ma; i++ ) {
				SWAP( covar[k][i], covar[j][i] )
			}
			k--;
		}
	}
}


#undef SWAP


#ifdef	SIGMAS
/* (C) Copr. 1986-92 Numerical Recipes Software ]2+530ks16n+. */
BOOL lfit( double x[], double y[], double sig[], int ndat, double a[], int ia[],
           int ma, double **covar, double *chisq, void ( *funcs )( double, double [], int ) )
{
	int i, j, k, l, m, mfit = 0;
	double ym, wt, sum, sig2i, *( beta[10] ), afunc[20];

	if( ma >= 10 ) {
		nrerror( "lfit: max 9 Parameter!" );
		return ( FALSE );
	}
//	beta = matrix(0,ma-1,0,0);
//  Statt dessen: Matrix vom Haus ...
	for( j = 0; j < ma; j++ ) {
		if( ia[j] ) {
			mfit++;
		}
		beta[j] = afunc+10+j;
	}
	if( mfit == 0 )	{
		nrerror( "lfit: Nix zu fitten?" );
		return ( FALSE );
	}
	for( j = 0; j < mfit; j++ ) {
		for( k = 0; k < mfit; k++ ) {
			covar[j][k] = 0.0;
		}
		beta[j][0] = 0.0;
	}
	for( i = 0; i < ndat; i++ ) {
		( *funcs )( x[i], afunc, ma );
		ym = y[i];
		if( mfit < ma ) {
			for( j = 0; j < ma; j++ ) {
				if( !ia[j] ) {
					ym -= a[j]*afunc[j];
				}
			}
		}
		sig2i = 1.0/SQR( sig[i] );
		for( j = -1, l = 0; l < ma; l++ ) {
			if( ia[l] ) {
				wt = afunc[l]*sig2i;
				for( j++, k = -1, m = 0; m <= l; m++ ) {
					if( ia[m] ) {
						covar[j][++k] += wt*afunc[m];
					}
				}
				beta[j][0] += ym*wt;
			}
		}
	}
	for( j = 1; j < mfit; j++ ) {
		for( k = 0; k < j; k++ ) {
			covar[k][j] = covar[j][k];
		}
	}
	gaussj( covar, mfit, beta, 1 );
	for( j = -1, l = 0; l < ma; l++ ) {
		if( ia[l] ) {
			a[l] = beta[++j][0];
		}
	}
	*chisq = 0.0;
	for( i = 0; i < ndat; i++ ) {
		( *funcs )( x[i], afunc, ma );
		for( sum = 0.0, j = 0; j < ma; j++ ) {
			sum += a[j]*afunc[j];
		}
		*chisq += SQR( ( y[i]-sum )/sig[i] );
	}
//	covsrt(covar,ma,ia,mfit);
	return ( TRUE );
}


/* (C) Copr. 1986-92 Numerical Recipes Software ]2+530ks16n+. */
#else
BOOL lfit( int HUGE *x, LPUWORD y, int ndat, LPDOUBLE a, int ma,
           double HUGE* HUGE *covar, LPDOUBLE chisq, void ( HUGE *funcs )( double, LPDOUBLE, int ) )
{
	int i, j, k, l, m;
	double ym;
//	double sum;
	double wt, *( beta[10] ), afunc[20];

	if( ma > 10 ) {
		nrerror( "lfit: max 9 Parameter!" );
		return ( FALSE );
	}
//	beta = matrix(0,ma-1,0,0);
//  Statt dessen: Matrix vom Haus ...
	for( j = 0; j < ma; j++ ) {
		beta[j] = afunc+10+j;
	}
	if( ma <= 0 ) {
		nrerror( "lfit: Nix zu fitten?" );
		return ( FALSE );
	}

	for( j = 0; j < ma; j++ ) {
		for( k = 0; k < ma; k++ ) {
			covar[j][k] = 0.0;
		}
		beta[j][0] = 0.0;
	}
	for( i = 0; i < ndat; i++ ) {
		( *funcs )( x[i], afunc, ma );
		ym = y[i];
		for( j = -1, l = 0; l < ma; l++ ) {
			wt = afunc[l];
			for( j++, k = -1, m = 0; m <= l; m++ ) {
				covar[j][++k] += wt*afunc[m];
			}
			beta[j][0] += ym*wt;
		}
	}
	for( j = 1; j < ma; j++ ) {
		for( k = 0; k < j; k++ ) {
			covar[k][j] = covar[j][k];
		}
	}
	gaussj( covar, ma, beta, 1 );
	for( j = -1, l = 0; l < ma; l++ ) {
		a[l] = beta[++j][0];
	}
#ifdef	CHISQR
	*chisq = 0.0;
	for( i = 0; i < ndat; i++ ) {
		( *funcs )( x[i], afunc, ma );
		for( sum = 0.0, j = 0; j < ma; j++ ) {
			sum += a[j]*afunc[j];
		}
		*chisq += SQR( y[i]-sum );
	}
//	covsrt(covar,ma,ia,mfit);
#endif
	return ( TRUE );
}


#endif



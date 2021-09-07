/**************************************************************************************
****	Mathematische und verwandte Routinen
**************************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "snomputz.h"
#include "snom-typ.h"
#include "snom-fit.h"
#include "snom-wrk.h"
#include "snom-mem.h"
#include "snom-win.h"
#include "snom-mat.h"


#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif


/**************************************************************************************
 * Ab hier eine 1-dim FFT auf der Basis der Numerical Recipies
 * DAS könnte der DSP VIEL schneller ... */


#define SWAP( a, b ) tempr = ( a ); ( a ) = ( b ); ( b ) = tempr

/* Achtung: NR => Basis der Array ist a[1]!!! */
void four1( LPFLOAT data, ULONG nn, int isign )
{
	long n, m, mmax, j;
	long int istep, i;
	float wtemp, wr, wpr, wpi, wi, theta;
	float tempr, tempi;

	n = nn << 1;
	j = 1;
	for( i = 1; i < n; i += 2 ) {
		if( j > i ) {
			SWAP( data[j], data[i] );
			SWAP( data[j+1], data[i+1] );
		}
		m = n >> 1;
		while( m >= 2 && j > m ) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}
	mmax = 2;
	while( n > mmax ) {
		istep = 2*mmax;
		theta = ( 2.0*M_PI )/( isign*mmax );
		wtemp = sin( 0.5*theta );
		wpr = -2.0*wtemp*wtemp;
		wpi = sin( theta );
		wr = 1.0;
		wi = 0.0;
		for( m = 1; m < mmax; m += 2 ) {
			for( i = m; i <= n; i += istep ) {
				j = i+mmax;
				tempr = wr*data[j]-wi*data[j+1];
				tempi = wr*data[j+1]+wi*data[j];
				data[j] = data[i]-tempr;
				data[j+1] = data[i+1]-tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}
			wr = ( wtemp = wr )*wpr-wi*wpi+wr;
			wi = wi*wpr+wtemp*wpi+wi;
		}
		mmax = istep;
	}
}


#undef SWAP

/* Achtung: NR => Basis der Array ist a[1]!!! */
void realft( LPFLOAT data, LONG n, int isign )
{
	long int i, i1, i2, i3, i4, n2p3;
	float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
	float wr, wi, wpr, wpi, wtemp, theta;

	theta = M_PI/(float) n;
	if( isign == 1 ) {
		c2 = -0.5;
		four1( data, n, 1 );
	}
	else {
		c2 = 0.5;
		theta = -theta;
	}
	wtemp = sin( 0.5*theta );
	wpr = -2.0*wtemp*wtemp;
	wpi = sin( theta );
	wr = 1.0+wpr;
	wi = wpi;
	n2p3 = 2*n+3;
	for( i = 2; i <= n/2; i++ ) {
		i4 = 1+( i3 = n2p3-( i2 = 1+( i1 = i+i-1 ) ) );
		h1r = c1*( data[i1]+data[i3] );
		h1i = c1*( data[i2]-data[i4] );
		h2r = -c2*( data[i2]+data[i4] );
		h2i = c2*( data[i1]-data[i3] );
		data[i1] = h1r+wr*h2r-wi*h2i;
		data[i2] = h1i+wr*h2i+wi*h2r;
		data[i3] = h1r-wr*h2r+wi*h2i;
		data[i4] = -h1i+wr*h2i+wi*h2r;
		wr = ( wtemp = wr )*wpr-wi*wpi+wr;
		wi = wi*wpr+wtemp*wpi+wi;
	}
	if( isign == 1 ) {
		data[1] = ( h1r = data[1] )+data[2];
		data[2] = h1r-data[2];
	}
	else {
		data[1] = c1*( ( h1r = data[1] )+data[2] );
		data[2] = c1*( h1r-data[2] );
		four1( data, n, -1 );
	}
}


// Macht eine FFT-Filterung; die Frequenzwerte werden mit den Werten
// aus pfFilter multipliziert. ACHTUNG pfFilter muss iSize gross sein!
BOOLEAN	BildFFTFilter( LPBILD pBild, LONG w, LONG h, LPFLOAT pfFilter, int iSize )
{
	LPFLOAT	pfData;
	double zm, mo2;
	LONG x, y;
	// und nur für das zurückkopieren ...
	LONG l, lMin, lMax;

	ASSERT( pBild != NULL  &&  pBild->puDaten != NULL  &&  w <= iSize  &&  pfFilter != NULL );

	pfData = (LPFLOAT)pMalloc( sizeof( float )*iSize );
	if( pfData == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	lMin = 65535l;
	lMax = -65535l;
	// Zeilenweise filtern ...
	for( y = 0;  y < h;  y++ ) {
		// Werte kopieren und Mittelwert abziehen
		zm = 0;
		for( x = 0;  x < w;  x++ ) {
			zm += pBild->puDaten[y*w+x];
		}
		zm /= (double)w;
		for( x = 0;  x < w;  x++ ) {
			pfData[x] = (float)( pBild->puDaten[y*w+x]-zm );
		}
		for(  ;  x < iSize;  x++ ) {
			pfData[x] = 0.0;
		}
		realft( pfData-1, iSize/2, 1 );

		// Mit Filter multiplizieren
		mo2 = iSize/2.0;
		for( x = 0;  x < iSize;  x++ ) {
			pfData[x] = (float)( pfData[x]*pfFilter[x]/mo2 );
		}

		// Und wieder herstellen
		realft( pfData-1, iSize/2, -1 );
		// Daten kopieren
		for( x = 0;  x < w;  x++ ) {
			l = (LONG)( pfData[x]+0.5+zm );
			if( l < lMin ) {
				lMin = l;
			}
			if( l > lMax ) {
				lMax = l;
			}
			pBild->puDaten[y*w+x] = l;
		}
	}

	MemFree( pfData );
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}


// 9.2.99


#if 0
// bilateral filer from https://www.cnblogs.com/wangguchangqing/p/6416401.html?tdsourcetag=s_pcqq_aiomsg

void myBilateralFilter(const Mat& src, Mat& dst, int ksize, double space_sigma, double color_sigma)
{
	int channels = src.channels();
	CV_Assert(channels == 1 || channels == 3);
	double space_coeff = -0.5 / (space_sigma * space_sigma);
	double color_coeff = -0.5 / (color_sigma * color_sigma);
	int radius = ksize / 2;
	Mat temp;
	copyMakeBorder(src, temp, radius, radius, radius, radius, BorderTypes::BORDER_REFLECT);
	vector<double> _color_weight(channels * 256); // 存放差值的平方
	vector<double> _space_weight(ksize * ksize); // 空间模板系数
	vector<int> _space_ofs(ksize * ksize); // 模板窗口的坐标
	double* color_weight = &_color_weight[0];
	double* space_weight = &_space_weight[0];
	int* space_ofs = &_space_ofs[0];
	for (int i = 0; i < channels * 256; i++)
		color_weight[i] = exp(i * i * color_coeff);
	// 生成空间模板
	int maxk = 0;
	for (int i = -radius; i <= radius; i++)
	{
		for (int j = -radius; j <= radius; j++)
		{
			double r = sqrt(i * i + j * j);
			if (r > radius)
				continue;
			space_weight[maxk] = exp(r * r * space_coeff); // 存放模板系数
			space_ofs[maxk++] = i * temp.step + j * channels; // 存放模板的位置，和模板系数相对应
		}
	}
	// 滤波过程
	for (int i = 0; i < src.rows; i++)
	{
		const uchar* sptr = temp.data + (i + radius) * temp.step + radius * channels;
		uchar* dptr = dst.data + i * dst.step;
		for (int j = 0; j < src.cols; j++)
		{
			double sum = 0, wsum = 0;
			int val0 = sptr[j]; // 模板中心位置的像素
			for (int k = 0; k < maxk; k++)
			{
				int val = sptr[j + space_ofs[k]];
				double w = space_weight[k] * color_weight[abs(val - val0)]; // 模板系数 = 空间系数 * 灰度值系数
				sum += val * w;
				wsum += w;
			}
			dptr[j] = (uchar)cvRound(sum / wsum);
		}
	}
}
#endif

/**************************************************************************************
 * Autokorrelation (Autokovarianz) berechnen
 */

BOOLEAN	Autokorrelation( LPFLOAT pfZiel, LPFLOAT pfDaten, int iMaxPts, BOOLEAN UseZeroMittel )
{
	LPFLOAT	pfTemp = NULL;
	double fTemp, fMittel;
	int iDelta, i;

	ASSERT( pfZiel != NULL  &&  pfDaten != NULL  &&  iMaxPts > 0  );

	// Falls Start und Ziel identisch ...
	if( pfZiel == pfDaten )	{
		pfTemp = pfZiel;
		pfZiel = (LPFLOAT)pMalloc( iMaxPts*sizeof( float ) );
		if( pfZiel == NULL ) {
			return ( FALSE );
		}
	}


	/*** Die Autokorrelation ist folgendermaßen definiert:
	 *** G(l) := 1/N \sum_{i=1}^{N-l} z_i*z_{l-i}
	 *** ACHTUNG: Die Einheit ist Länge^2!
	 *** ACHTUNG: <z_i> := 0, sonst abziehen!
	 ***/

	if( !UseZeroMittel ) {
		fMittel = 0.0;
		// Zuerst z_i-Mittel (<z_i>) berechnen
		for( i = 0;  i < iMaxPts;  i++ ) {
			fMittel += pfDaten[i];
		}
		fMittel /= (double)iMaxPts;
#if 1
		// Dann Autokorrelation G(l) wie oben berechnen
		for( iDelta = 0;  iDelta < iMaxPts;  iDelta++ )	{
			fTemp = 0.0;
			for( i = 0;  i < iMaxPts-iDelta;  i++ ) {
				fTemp += ( pfDaten[i]-fMittel )*( pfDaten[i+iDelta]-fMittel );
			}
			fTemp /= (double)i;
			pfZiel[iDelta] = (float)fTemp;
		}
#else
		// Dann Autokorrelation G(l) nicht wie oben aber mit allen Pkte
		// => Autokorr symmetrisch
		for( iDelta = 0;  iDelta < iMaxPts;  iDelta++ )	{
			fTemp = 0.0;
			for( i = 0;  i < iMaxPts-iDelta;  i++ ) {
				fTemp += ( pfDaten[i]-fMittel )*( pfDaten[i+iDelta]-fMittel );
			}
			for(  ;  i < iMaxPts;  i++ ) {
				fTemp += ( pfDaten[i]-fMittel )*( pfDaten[i+iDelta-iMaxPts]-fMittel );
			}
			fTemp /= (double)iMaxPts;
			pfZiel[iDelta] = (float)fTemp;
		}
#endif
	}
	else {  // without substracting a mean value
		// Dann Autokorrelation G(l) wie oben berechnen
		for( iDelta = 0;  iDelta < iMaxPts;  iDelta++ )	{
			fTemp = 0.0;
			for( i = 0;  i < iMaxPts-iDelta;  i++ ) {
				fTemp += pfDaten[i]*pfDaten[i+iDelta];
			}
			fTemp /= (double)i;
			pfZiel[iDelta] = (float)fTemp;
		}
	}

	// Start und Ziel waren identisch? => Zurückkopieren
	if( pfTemp == pfDaten )	{
		//MemMove( pfDaten, pfZiel, sizeof(float)*iMaxPts );
		for( i = 0;  i < iMaxPts;  i++ ) {
			pfDaten[i] = pfZiel[i];
		}
		MemFree( pfZiel );
	}

	return TRUE;
}


/**********************************************************************************
 * Numerik
 */
BOOLEAN	Differential( LPFLOAT pfDaten, int iPunkte )
{
	float fTemp;
	int x;
	float fLastDaten;

	ASSERT(  pfDaten == NULL );

	for( x = 0;  x < iPunkte;  x++ ) {
		if( x < iPunkte-3 ) {
			fTemp = ( -11.0*pfDaten[x] + 18.0*pfDaten[x+1]  - 9.0*pfDaten[x+2]  - 2.0*pfDaten[x+3] )/6.0;
		}
		else {
			// ulLastDaten == puDaten[x-1] unverändert
			if( x < iPunkte-1 ) {
				fTemp = ( 2.0*pfDaten[x+1] - fLastDaten - pfDaten[x] )/3.0;
			}
		}
		fLastDaten = pfDaten[x];        // wird nur für x>=w benötigt
		pfDaten[x] = fTemp;
	}
	return ( TRUE );
}


// add the point if it is inside
void AddPoint( float *pfDaten, long rx, long ry, long w, long h, double ave, double *f, LONG *iPts )
{
	long y = 0, yy;
	long iOffset = ry*w+rx;

	// draw the circle
	if( ry < 0 ) {
		y = -ry;
	}
	else {
		h = h-ry;
	}
	for(  ;  y < h;  y++ ) {
		long x = 0, wx = w;

		if( rx < 0 ) {
			x = -rx;
		}
		else {
			wx = wx-rx;
		}
		yy = y*w;
		for(  ;  x < wx;  x++ )	{
			( *iPts )++;
			( *f ) += pfDaten[x+yy]*pfDaten[x+yy+iOffset];
		}
	}
}


/************************************************************************
 * und ab hier Berechnung der Korrelationsfunction
 */
void CorrelationFunction( HFILE hFile, LPUWORD puDaten, long w, long h, double dSkal )
{
	char str[256];
	long y, radius, i;
	long rx, ry, d, rmax;
	LONG *iPts;
	double *fC;
	double dAverage = 0.0;
	float *pfDaten;

	rmax = sqrt( (double)w*(double)w+(double)h*(double)h );
	iPts = (LONG*)pMalloc( rmax*sizeof( LONG ) );
	fC = (double*)pMalloc( rmax*sizeof( double ) );
	pfDaten = (float*)pMalloc( w*h*sizeof( float ) );
	if( pfDaten == NULL ) {
		StatusLineRsc( E_MEMORY );
		return;
	}
	// calculate mean
	i = 0;
	for( y = 0;  y < h*w;  y++ ) {
		i += puDaten[y];
	}
	dAverage = (double)i/(double)( h*w );
	for( y = 0;  y < h*w;  y++ ) {
		pfDaten[y] = (float)puDaten[y]-dAverage;
	}

	// some other init
	for( radius = 1;  radius < rmax;  radius++ ) {
		fC[radius] = 0.0;
		iPts[radius] = 0;

		sprintf( str, "radius %i", radius );
		StatusLine( str );
		// use the bresenham algorithm for the circle points
		rx = 0;
		ry = radius;
		d = 3-2*radius;
		while( rx < ry ) {
			// now here is a coordinate, we have to add each eight points
			AddPoint( pfDaten, +rx, +ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, +ry, +rx, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -rx, +ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -ry, +rx, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, +rx, -ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, +ry, -rx, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -rx, -ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -ry, -rx, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
//			YieldApp( FALSE );
			if( d < 0 ) {
				d = d + 4*rx + 6;
			}
			else {
				d = d + 4*( rx-ry ) + 10;
				ry--;
			}
			rx++;
		}
		// probably the last point must be added too
		if( rx == ry ) {
			// now here is a coordinate, we have to add now only four points
			AddPoint( pfDaten, +rx, +ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -rx, +ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, +rx, -ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
			AddPoint( pfDaten, -rx, -ry, w, h, dAverage, &( fC[radius] ), &( iPts[radius] ) );
		}
		// end circle
		// write to file
		fC[radius] = fC[radius]/(double)iPts[radius];
		sprintf( str, "%lf\t%li\n", fC[radius], iPts[radius] );
		_lwrite( hFile, str, lstrlen( str ) );
	}
	// clean up
	_lclose( hFile );
	MemFree( iPts );
	MemFree( fC );
	MemFree( pfDaten );
}


/************************************************************************
 * und ab hier RMS-Berechnung
 *	Berechnet die mittlere Rauhigkeit und die mittlere Höhe im Bild
 */
void RMSArea( LPUWORD puDaten, LONG ww, LONG x, LONG y, LONG w, LONG h,
              double dSkal, LPDOUBLE pfMeanH, LPDOUBLE pfRMS, LPFLOAT pfQuadrate )
{
	double fMean = 0.0, fSD = 0.0;
	LONG zMean, i, j;
	LPUWORD	puPtr;

	// Mittelwert und Stardardabweichung nach folgender Formel berechnen
	// M = M'+1/n*((x1-M')+(x2-M')+...+(xn-M')
	// D = (x1-M)^2+..+(xn-M)^2 = (x1-M')^2+...+(xn-M')^2 - n(M'-M)^2
	// sd = sqrt(1/(n-1)*D)
	// Dabei ist M':=0 der Einfachheit halber
	// somit D = x1^2 + ... + xn^2 - n*(M^2)

	if( w > ww ) {
		*pfMeanH = 0.0;
		*pfRMS = 0.0;
		return;
	}

	if( pfQuadrate != NULL ) { // Es gibt bereits ein Arry mit fertigen Quadraten (ist schneller ...)
		LPFLOAT	pF;

		puPtr = puDaten+ww*y+x;
		pF = pfQuadrate+ww*y+x;
		ww -= w;
		for( j = 0;  j < h;  j++ ) {
			zMean = 0;
			for( i = 0;  i < w;  i++ ) {
				// Für Mittelwert
				zMean += *puPtr++;
				// Für Standardabweichung
				fSD += *pF++;
			}
			fMean += zMean;
			puPtr += ww;
			pF += ww;
		}
	}
	else {
		unsigned long k;

		puPtr = puDaten+ww*y+x;
		ww -= w;
		for( j = 0;  j < h;  j++ ) {
			zMean = 0;
			for( i = 0;  i < w;  i++ ) {
				k = *puPtr++;
				// Für Mittelwert
				zMean += k;
				// Für Standardabweichung
				fSD += ( k*k );
			}
			fMean += zMean;
			puPtr += ww;
		}
	}

	*pfMeanH = (double)fMean / (double)( w*h );
	*pfRMS = sqrt( ( (double)fSD-( *pfMeanH*fMean ) )/( w*h-1l ) )*dSkal;
	*pfMeanH *= dSkal;

#if 0
	// Konventioneller Algorithmus zur Ermittlung der Standardabweichung
	// langsamer!
	fSD = 0.0;
	for( j = 0;  j < h;  j++ ) {
		puPtr = pBild->puDaten+ww*( y+j )+x;
		for( i = 0;  i < w;  i++ ) {
			d = (double)*puPtr++ - fMean;
			fSD += d*d;
		}
	}

	// Und die Werte zurück ...
	*pfMeanH = fMean*pBild->fSkal;
	d = w*h;
	d = sqrt( fSD/( d-1.0 ) )*pBild->fSkal;
	*pfRMS = d;
#endif
}


typedef struct {
	UWORD hgt;
	ULONG count;
} HgtRecordT;

int CompareHgtRecordT( const void *a, const void *b )
{
	return ( ( *(HgtRecordT*)b ).count - ( *(HgtRecordT*)a ).count );
}


/************************************************************************
 *	Berechnet die mittlere Höhe (häufigster Wert)
 */
void MeadianArea( LPUWORD puDaten, LONG ww, LONG x, LONG y, LONG w, LONG h,
                  double dSkal, UWORD maxhgt, LPDOUBLE pfMedian, LPDOUBLE pfRMS )
{
	HgtRecordT max[65536];
	LONG i, j;
	LPUWORD	puPtr;

	double fMean = 0.0, fSD = 0.0;

	assert(  w <= ww  );

	for( j = 0;  j < maxhgt;  j++ ) {
		max[j].hgt = j;
		max[j].count = 0;
	}

	puPtr = puDaten+ww*y+x;
	ww -= w;
	for( j = 0;  j < h;  j++ ) {
		for( i = 0;  i < w;  i++ ) {
			max[*puPtr++].count++;
		}
		puPtr += ww;
	}
	// now we have an array of counts, find the ten largest ones
	qsort( max, maxhgt, sizeof( *max ), CompareHgtRecordT );

	// an now get the mean Median of the most often 5% data
	for( i = 0, j = ( ( w-x )*( h-y ) )/20;  j >= 0;  i++ ) {
		long k = max[i].hgt;
		// Für Mittelwert
		fMean += k;
		// Für Standardabweichung
		fSD += ( k*k );
		j -= max[i].count;
	}

	*pfRMS = 0.0;
	if( i > 0 ) {
		*pfRMS = sqrt( ( (double)fSD-( fMean*fMean )/i )/( i-1l ) )*dSkal;
	}
	*pfMedian = fMean*dSkal / (double)i;
}
// 18.11.11


/****	Berechnet die mittlere Rauhigkeit und die mittlere Höhe auf einer Scanline ****/
void RMSLine( LPUWORD puDaten, LONG x, LONG y, LONG w, LONG h, LONG ww, double m, double fSkal, LPDOUBLE pfMeanH, LPDOUBLE pfRMS )
{
	double fMean = 0.0, fSD = 0.0, fDelta = 0.0;
	LONG zMean, inc = -1, i;
	unsigned long k;

	if( m < 0.0 ) {
		inc = 1;
		m *= -1.0;
	}
	if( m <= 1.0 ) {
		// m ist jetzt die Steigung der Geraden mit dem gewünschten Winkel
		// also praktisch der Winkel der Linie
		if( m < 1e-9 ) {
			m = 1e9;        // Gerade sollte senkrecht sein ...
		}
		else {
			m = 1.0/m;
		}

		// Also steigen jetzt die x Werte immer um eins, während y nur bei allen
		// m Pixeln um eins erhöht wird
		zMean = (long)( inc*( x/m ) );
		y = ( y+zMean+h )%h;
		fDelta += m*( zMean+1 );

		// Mittelwert und Stardardabweichung nach folgender Formel berechnen
		// M = M'+1/n*((x1-M')+(x2-M')+...+(xn-M')
		// D = (x1-M)^2+..+(xn-M)^2 = (x1-M')^2+...+(xn-M')^2 - n(M'-M)^2
		// sd = sqrt(1/(n-1)*D)
		// Dabei ist M':=0 der Einfachheit halber
		// somit D = x1^2 + ... + xn^2 - n*(M^2)
		zMean = 0;
		puDaten += x;
		for( i = 0;  i < ww;  x++, i++ ) {
			if( x > fDelta ) {
				// Unten raus: Oben wieder rein ...
				y += inc;
				fDelta += m;
				if( y < 0 ) {
					y += h;
				}
				else if( y >= h ) {
					y -= h;
				}
			}

			k = puDaten[w*y];
			puDaten++;
			// Für Mittelwert
			zMean += k;
			// Für Standardabweichung
			fSD += ( k*k );
		}
		fMean = zMean;
	}
	else {
		// Also steigen jetzt die y Werte immer um eine, während x nur bei allen
		// m Pixeln um eins erhöht wird
		zMean = (long)( inc*( y/m ) );
		x = ( x+zMean+w )%w;
		fDelta += m*( zMean+1 );

		// Mittelwert und Stardardabweichung nach folgender Formel berechnen
		// M = M'+1/n*((x1-M')+(x2-M')+...+(xn-M')
		// D = (x1-M)^2+..+(xn-M)^2 = (x1-M')^2+...+(xn-M')^2 - n(M'-M)^2
		// sd = sqrt(1/(n-1)*D)
		// Dabei ist M':=0 der Einfachheit halber
		zMean = 0;
		puDaten += w*y;
		for( i = 0;  i < ww;  y++, i++ ) {
			if( y > fDelta ) {
				// Unten raus: Oben wieder rein ...
				x += inc;
				fDelta += m;
				if( x < 0 ) {
					x += w;
				}
				else if( x >= w ) {
					x -= w;
				}
			}

			k = puDaten[x];
			puDaten += w;
			// Für Mittelwert
			zMean += k;
			// Für Standardabweichung
			fSD += ( k*k );
		}
		fMean = zMean;
	}
	fMean /= (double)ww;
	*pfRMS = sqrt( ( fSD-( fMean*fMean*ww ) )/( ww-1l ) )*fSkal;
	*pfMeanH = fMean*fSkal;
}
// 18.2.98


/****	Rechnet mit Dib und Wert ****/
BOOLEAN	BildCalcConst( LPBILD pDestBild, LONG w, LONG h, UCHAR cOperand, double wert, BOOL bOverflow )
{
	LPUWORD	puZeile, puDestData = pDestBild->puDaten;
	LONG x, y;
	UWORD uWert;
	BOOL bIsOver = FALSE;

	// Wert 0.0 macht *NIE* Sinn!
	if( wert == 0.0 ) {
		return ( FALSE );
	}

	switch( cOperand ) {
		// außerdem: Z-Skalierung verändern!
		case '*':
			pDestBild->fSkal /= wert;
			break;

		case '/':
			pDestBild->fSkal *= wert;
			break;

		case 'l':
		{
			CHAR str[32];

			pDestBild->fSkal = log( pDestBild->fSkal )*wert;
			pDestBild->bSpecialZUnit = TRUE;
			lstrcpy( (LPSTR)str, (LPSTR)pDestBild->strZUnit );
			wsprintf( pDestBild->strZUnit, "ln(%s)", (LPSTR)str );
			break;
		}

		// Für alle anderen braucht man eh gerundete Werte
		default:
			uWert = (UWORD)(long)wert;
	}

	if( (LONG)puDestData <= 256 ) {
		return ( FALSE );   // Sonst geht keine UNDO!
	}
	if( !bOverflow ) {
		// Bei bOverflow muss das lZwischenergebnis noch gespeichert werden!
		for( y = 0;  y < h;  y++ ) {
			puZeile = pDestBild->puDaten+( w*y );
			switch( cOperand ) {
				case '+':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ += uWert;
					}
					break;

				case '-':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ -= uWert;
					}
					break;

				case '*':
					for( x = 0;  x < w;  x++ ) {
						*puZeile = (UWORD)( *puZeile*wert+0.5 );
						puZeile++;
					}
					break;

				case '/':
					for( x = 0;  x < w;  x++ ) {
						*puZeile = (UWORD)( *puZeile/wert+0.5 );
						puZeile++;
					}
					break;

				case '&':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ &= uWert;
					}
					break;

				case '|':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ |= uWert;
					}
					break;

				case '^':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ ^= uWert;
					}
					break;

				case '~':
					for( x = 0;  x < w;  x++ ) {
						*puZeile = ~*puZeile;
						puZeile++;
					}
					break;

				case '%':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ %= uWert;
					}
					break;

				case 'l':
					for( x = 0;  x < w;  x++, puZeile++ ) {
						*puZeile = (UWORD)( log( ( *puZeile )+1.0 )*wert+0.5 );
					}
					break;
			}
		}
	}
	else {
		LONG lZwischen;

		// Bei bOverflow muss das lZwischenergebnis noch gespeichert werden!
		for( y = 0;  y < h;  y++ ) {
			puZeile = pDestBild->puDaten+( y*w );
			switch( cOperand ) {
				case '+':
					for( x = 0;  x < w;  x++ ) {
						lZwischen = *puZeile;
						lZwischen += uWert;
						if( lZwischen >= 0x10000l ) {
							bIsOver = TRUE;
							lZwischen = 0x0FFFFl;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;

				case '-':
					for( x = 0;  x < w;  x++ ) {
						lZwischen = *puZeile;
						if( lZwischen < wert ) {
							bIsOver = TRUE;
							lZwischen = 0;
						}
						else {
							lZwischen -= uWert;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;

				case '*':
					for( x = 0;  x < w;  x++ ) {
						lZwischen = *puZeile;
						lZwischen = (LONG)( lZwischen*wert+0.5 );
						if( lZwischen < 0 ) {
							bIsOver = TRUE;
							lZwischen = 0;
						}
						else if( lZwischen >= 0x10000l ) {
							bIsOver = TRUE;
							lZwischen = 0x0FFFFl;
						}
						*puZeile++ = (WORD)lZwischen;
					}
					break;

				case '/':
					for( x = 0;  x < w;  x++ ) {
						lZwischen = *puZeile;
						lZwischen = (LONG)( lZwischen/wert+0.5 );
						if( lZwischen < 0 ) {
							bIsOver = TRUE;
							lZwischen = 0;
						}
						else if( lZwischen >= 0x10000l ) {
							bIsOver = TRUE;
							lZwischen = 0x0FFFFl;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;

				case '&':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ &= uWert;
					}
					break;

				case '|':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ |= uWert;
					}
					break;

				case '^':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ ^= uWert;
					}
					break;

				case '~':
					for( x = 0;  x < w;  x++ ) {
						*puZeile = ~*puZeile;
						puZeile++;
					}
					break;

				case '%':
					for( x = 0;  x < w;  x++ ) {
						*puZeile++ %= uWert;
					}
					break;

				case 'l':
					for( x = 0;  x < w;  x++, puZeile++ ) {
						*puZeile = (UWORD)( log( *puZeile+1.0 )*wert+0.5 );
					}
					break;
			}
		}
	}
	BildMax( pDestBild, w, h );
	return ( !bIsOver );
}


/****	Rechnet mit 2 Bildern ****/
BOOL BildCalcBild( LPBMPDATA pDest, WORKMODE DestMode, WORD cOperand, LPBMPDATA puWerte, WORKMODE WerteMode, BOOL bOverflow )
{
	LPBILD pDestBild;
	LPUWORD	puZeile, puDestData;
	LPUWORD	puWert, puWerteData;
	BOOLEAN	bIsOver = FALSE;
	LONG w = pDest->pSnom[pDest->iAktuell].w, h = pDest->pSnom[pDest->iAktuell].h;
	LONG ww = puWerte->pSnom[puWerte->iAktuell].w, hh = puWerte->pSnom[puWerte->iAktuell].h;
	LONG i, j;

	ASSERT( ( DestMode&7 ) != 0 ); // Nix ausgewählt!

	// Gültigen Pointer erstellen
	if( DestMode == TOPO ) {
		puDestData = pDest->pSnom[pDest->iAktuell].Topo.puDaten;
		pDestBild = &pDest->pSnom[pDest->iAktuell].Topo;
	}
	else if( DestMode == ERRO ) {
		puDestData = pDest->pSnom[pDest->iAktuell].Error.puDaten;
		pDestBild = &pDest->pSnom[pDest->iAktuell].Error;
	}
	else if( DestMode == LUMI ) {
		puDestData = pDest->pSnom[pDest->iAktuell].Lumi.puDaten;
		pDestBild = &pDest->pSnom[pDest->iAktuell].Lumi;
	}

	if( (LONG)puDestData <= 256  ||  w > ww  ||  h > hh ) {
		return ( FALSE );   // Sonst geht keine UNDO, bzw. die Werte gehen vorzeitig aus ...!
	}
	// Gültigen Pointer erstellen
	if( WerteMode == TOPO )	{
		puWerteData = puWerte->pSnom[puWerte->iAktuell].Topo.puDaten;
		if( (LONG)puWerteData <= 256 ) {
			puWerteData = puWerte->pSnom[(LONG)puWerteData-1].Topo.puDaten;
		}
	}
	if( WerteMode == ERROR ) {
		puWerteData = puWerte->pSnom[puWerte->iAktuell].Error.puDaten;
		if( (LONG)puWerteData <= 256 ) {
			puWerteData = puWerte->pSnom[(LONG)puWerteData-1].Error.puDaten;
		}
	}
	if( WerteMode == LUMI )	{
		puWerteData = puWerte->pSnom[puWerte->iAktuell].Lumi.puDaten;
		if( (LONG)puWerteData <= 256 ) {
			puWerteData = puWerte->pSnom[(LONG)puWerteData-1].Lumi.puDaten;
		}
	}

	if( !bOverflow  ||  ( cOperand != '+'  &&  cOperand != '-'  &&  cOperand != '*' ) ) {
		// Bei bOverflow muss das lZwischenergebnis noch gespeichert werden!
		for( i = 0;  i < h;  i++ ) {
			puZeile = puDestData;
			puDestData += w;
			puWert = puWerteData+( i*ww );
			switch( cOperand ) {
				case '+':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ += *puWert++;
					}
					break;

				case '-':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ -= *puWert++;
					}
					break;

				case '*':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ *= *puWert++;
					}
					break;

				case '/':
					for( j = 0;  j < w;  j++ ) {
						if( *puWert == 0 ) {
							*puZeile++ = 0;
							bIsOver = TRUE;
						}
						else {
							*puZeile++ /= *puWert;
						}
						puWert++;
					}
					break;

				case '&':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ &= *puWert++;
					}
					break;

				case '|':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ |= *puWert++;
					}
					break;

				case '^':
					for( j = 0;  j < w;  j++ ) {
						*puZeile++ ^= *puWert++;
					}
					break;

				case '~':
					for( j = 0;  j < w;  j++ ) {
						*puZeile = ~*puZeile;
						puZeile++;
					}
					break;

				case '%':
					for( j = 0;  j < w;  j++ ) {
						if( *puWert == 0 ) {
							*puZeile++ = 0;
							bIsOver = TRUE;
						}
						else {
							*puZeile++ %= *puWert;
						}
						puWert++;
					}
					break;
			}
		}
	}
	else {
		LONG lZwischen;

		// Bei bOverflow muss das lZwischenergebnis noch gespeichert werden!
		for( i = 0;  i < h;  i++ ) {
			puZeile = (LPUWORD)puDestData;
			puDestData += w;
			puWert = puWerteData+( i*ww );
			switch( cOperand ) {
				case '+':
					for( j = 0;  j < w;  j++ ) {
						lZwischen = *puZeile;
						lZwischen += *puWert++;
						if( lZwischen >= 0x10000l ) {
							bIsOver = TRUE;
							lZwischen = 0x0FFFFl;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;

				case '-':
					for( j = 0;  j < w;  j++ ) {
						lZwischen = *puZeile;
						if( lZwischen < *puWert ) {
							bIsOver = TRUE;
							lZwischen = 0;
						}
						else {
							lZwischen -= *puWert++;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;

				case '*':
					for( j = 0;  j < w;  j++ ) {
						lZwischen = *puZeile;
						lZwischen *= *puWert++;
						if( lZwischen < 0 ) {
							bIsOver = TRUE;
							lZwischen = 0;
						}
						else if( lZwischen >= 0x10000l ) {
							bIsOver = TRUE;
							lZwischen = 0x0FFFFl;
						}
						*puZeile++ = (UWORD)lZwischen;
					}
					break;
			}
		}
	}
	BildMax( pDestBild, w, h );
	return ( !bIsOver );
}


/**************************************************************************************
 * Ab hier Bildmittelung
 *
 * Mittelt eine Zeile; N ist die gewünschte Anzahl der Mittelwerte, (nur intern)
 * d.h. n=3 ergibt 3 Mittel: 0-.333 .333-.666 und .666-1
 * Sind fuer einen Wert nicht genügend Punkte vorhanden,
 * ist das Ergenis 0, sonst Punktezahl
 */
BOOLEAN	MittelZeile( LPUWORD puZeile, LPUCHAR puMaske, LONG w, LPDOUBLE pfMittel, ULONG n )
{
	ULONG i, j, ulPunkte, ulMittel;

	for( i = j = 0;  j < n;  j++ ) {
		ulPunkte = 0;
		ulMittel = 0;

		// Alle Punkte in dem entsprechenden Bereich aufsummieren ...
		while( i < ( ( j+1 )*w )/n ) {
			// ... so sie nicht maskiert sind!
			if( puMaske  &&  ( puMaske[i/8]&( 0x0080>>( i%8 ) ) ) != 0 ) {
				i++;
			}
			else {
				ulMittel += (ULONG)puZeile[i++];
				ulPunkte++;
			}
		}
		// Alles ausmaskiert!?
		if( ulPunkte == 0 ) {
			return ( FALSE );
		}
		// ansonsten einfach lMitteln ...
		pfMittel[j] = (double)ulMittel/(double)ulPunkte;
	}
	// wenn hier, dann kein vorzeitiger Abbruch
	return ( TRUE );
}


/****	Mittelung eines Bildes analytisch zeilenweise ****
 * n ist die Ordnung der Mittelung;
 * benutzt als Hilfsfunktion MittelZeile
 */
BOOLEAN	MittelBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n )
{
	LPUWORD puZeile;
	LPBYTE pMaskeLinie;
	double pfKoeff[3];
	LONG x, y, lMin, lMax;

	if( pBmp == NULL  ||  pBild == NULL  ||  n > 4 ) {  // Nur bis max. 2. Ordnung
		return ( FALSE );
	}

	puZeile = pBild->puDaten;
	lMin = 32767l;  // Größte Differenz zum Mittelwert, deshalb mit 0 init.
	lMax = -32768l;
	pMaskeLinie = NULL;     // Keine Maske

	for( y = 0; y < h;  y++ ) {
		// Maske berücktsichtigen
		if( pBmp->pMaske ) {
			pMaskeLinie = pBmp->pMaske + ( h-y-1 )*pBmp->wMaskeW;
		}
		if( !MittelZeile( puZeile, pMaskeLinie, w, pfKoeff, n ) ) {
			return ( FALSE );
		}

		switch( n ) {
			case 1: // Nullter Ordnung; Mittelwert wird einfach abgezogen
			{
				LONG lMW = (LONG)pfKoeff[0], lWert;

				for( x = 0;  x < w;  x++ ) {
					lWert = (LONG)(ULONG)puZeile[x] - lMW;
					if( lWert < lMin ) {
						lMin = lWert;
					}
					else if( lWert > lMax ) {
						lMax = lWert;
					}
					puZeile[x] = (UWORD)lWert;
				}
				break;
			}


			case 2: // Erster Ordnung, Gerade
				// Die mittlere Steigung wird nach der Formel
				//**  {(x[1]-x[n/2])+(x[2]-x[n/2+1])+...}/(w/2)^2 =
				//**  (2/w)*{ (x[w/2]+x[w/2+1]+...+x[w-1])/(w/2-1) - (x[1]+x[2]+...+x[xw/2-1])(w/2-1)}
				//**  also ( Mittelwert rechts - Mittelwert links ) geteilt durch die halbe Weite
			{
				LONG lWert, lMW = (LONG)( ( pfKoeff[0]+pfKoeff[1] )/2.0+0.5 );
				double fSteigung = 2.0*( pfKoeff[1]-pfKoeff[0] )/(double)w;

				for( x = 0;  x < w;  x++ ) {
					lWert = (LONG)(ULONG)puZeile[x] - (LONG)( x*fSteigung ) - lMW;
					if( lWert < lMin ) {
						lMin = lWert;
					}
					else if( lWert > lMax ) {
						lMax = lWert;
					}
					puZeile[x] = (UWORD)lWert;
				}
				break;
			}

			case 3:
				//** Die hohe Kunst: 2. Ordnung; hier sind die Grenzen analytischer Methoden erreicht
				//** Trotzdem kurz die Methode
				//**
				//** y(x) = a*x^2 + b*x + c
				//**
				//** Wenn x von -1,5 bis 1,5, dann Annahme MittellWert=wahre lWerte bei -1,0,1
				//** also
				//** a = (m1+m3)/2-m2
				//** b = (m3-m1)/2
				//** c = m2;
			{
				LONG lWert;
				double a = ( pfKoeff[0]+pfKoeff[2] )/2.0-pfKoeff[1];
				double b = ( pfKoeff[2]-pfKoeff[0] )/2.0;
				double c = pfKoeff[1];
				// damit fx -1,5 ... 1,5, fx = mx*(x-dx);
				double dx = (double)w/2.0;
				double mx = 1.5/dx;
				double fx;

				for( x = 0;  x < w;  x++ ) {
					fx = mx*( x-dx );
					lWert = (LONG)(ULONG)puZeile[x] - (LONG)( a*fx*fx+b*fx+c );
					if( lWert < lMin ) {
						lMin = lWert;
					}
					else if( lWert > lMax ) {
						lMax = lWert;
					}
					puZeile[x] = (UWORD)lWert;
				}
				break;
			}
		}
		puZeile += w;
	}

	return ( BildMinMax( pBild, lMin, lMax, w, h ) );
}


/* nur intern! */
// Polynom n-ter Ordnung für MittelFitBild ...
void fpoly( double x, LPDOUBLE pf, int n )
{
	int i;

	pf[0] = 1.0;
	for( i = 1;  i < n;  i++ ) {
		pf[i] = pf[i-1]*x;
	}
}


/* nur intern */
// Polynom n-ter Ordnung für MittelFitBild ...
double fPolyWert( double x, LPDOUBLE pf, int n )
{
	int i;
	double y = pf[n-1];

	for( i = n-2;  i >= 0;  i-- ) {
		y = y*x+pf[i];
	}
	return ( y );
}


/* Interpoliert die maskierte Region horizontal durch ein Polynom n. Grades */
BOOLEAN	InterpolateVertikalBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int grad )
{
	LONG n, x, y, lWert;
	WORD wBitMaske;
	BOOL bThisLine;
	// für's fitten
	int HUGE *pX;
	LPUWORD	pY;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], chi, a[10];

	if( ( pX = (int HUGE*)pMalloc( sizeof( int )*h ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( WORD )*h ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// matrix erstellen
	for( n = 0;  n < 10;  n++ ) {
		pCovar[n] = (LPDOUBLE)CoMem+10*n;
	}

	for( x = 0;  x < w; ) {
		bThisLine = FALSE;
		wBitMaske = 0x80>>( x%8 );
		// Testen, ob etwas zu fitten und die Felder für den Fit füllen
		for( n = y = 0;  y < h;  y++ ) {
			if( pBmp->pMaske[( x/8 )+( h-y-1 )*pBmp->wMaskeW]&wBitMaske ) {
				bThisLine = TRUE;
			}
			else {
				pX[n] = y;
				pY[n] = pBild->puDaten[x+w*y];
				n++;
			}
		}
		if( bThisLine )	{
			if( !lfit( pX, pY, n, a, grad, pCovar, &chi, fpoly ) ) {
				;       // Fitfehler!
			}
			for( y = 0;  y < h;  y++ ) {
				if( pBmp->pMaske[( x/8 )+( h-y-1 )*pBmp->wMaskeW]&wBitMaske ) {
					lWert = fPolyWert( y, a, grad );
					// Überlauf => ignorieren!
					if( lWert > 65535 ) {
						lWert = 65535;
					}
					if( lWert < 0 ) {
						lWert = 0;
					}
					pBild->puDaten[x+y*w] = (UWORD)lWert;
				}
			}
			x++;
		}
		else {
			// Nur solange die Bitmasken noch 8 Bit weit!
			x += 8;
		}
	}

	MemFree( pX );
	MemFree( pY );
	return TRUE;
}


/* Interpoliert die maskierte Region horizontal durch ein Polynom n. Grades */
BOOLEAN	InterpolateHorizontalBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int grad )
{
	LONG n, x, y, lWert;
	WORD wBitMaske;
	BOOL bThisLine;
	// für's fitten
	int HUGE *pX;
	LPUWORD	pY;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], chi, a[10];

	if( ( pX = (int HUGE*)pMalloc( sizeof( int )*w ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( WORD )*w ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// matrix erstellen
	for( n = 0;  n < 10;  n++ ) {
		pCovar[n] = (LPDOUBLE)CoMem+10*n;
	}

	for( y = 0;  y < h;  y++ ) {
		bThisLine = FALSE;
		// Testen, ob etwas zu fitten und die Felder für den Fit füllen
		for( n = x = 0;  x < w;  x++ ) {
			wBitMaske = 0x0080>>( x%8 );
			if( pBmp->pMaske[( x/8 )+( h-y-1 )*pBmp->wMaskeW]&wBitMaske ) {
				bThisLine = TRUE;
			}
			else {
				pX[n] = x;
				pY[n] = pBild->puDaten[x+w*y];
				n++;
			}
		}
		if( bThisLine )	{
			if( !lfit( pX, pY, n, a, grad, pCovar, &chi, fpoly ) ) {
				;       // Fitfehler!
			}
			for( x = 0;  x < w; x++ ) {
				wBitMaske = 0x0080>>( x%8 );
				if( pBmp->pMaske[( x/8 )+( h-y-1 )*pBmp->wMaskeW]&wBitMaske ) {
					lWert = fPolyWert( y, a, grad );
					if( lWert > 65535 ) {
						lWert = 65534;
					}
					if( lWert >= pBild->uMaxDaten ) {
						pBild->uMaxDaten = (UWORD)lWert+1;
					}
					pBild->puDaten[x+y*w] = (UWORD)lWert;
				}
			}
		}
	}

	MemFree( pX );
	MemFree( pY );
	return ( TRUE );
}
// 20.12.98


/****	Mittelung eines Bildes fitting zeilenweise ****/
/**** n ist die Ordnung der Mittelung;
 **** benutzt als Hilfsfunktion lfit ****/
static BOOLEAN	MittelFitBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n )
{
	LPUWORD pZeile;
	LPBYTE pMaske;
	LONG x, y, i, lWert, lMin, lMax;
	LPLONG pX;
	LPUWORD	pY;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], chi, a[10];

	n++;
	if( pBmp == NULL  ||  pBild == NULL  ||  n > 10 ) { // Nur bis max. 9. Ordnung
		return ( FALSE );
	}

	if( ( pX = (LPLONG)pMalloc( sizeof( int )*w ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( long )*w ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	for( x = 0;  x < 10;  x++ ) {
		pCovar[x] = CoMem+10*x;
	}

	lMin = 32767l;  // Größte Differenz zum MittellWert, deshalb mit 0 init.
	lMax = -32768l;
	pMaske = NULL;  // Keine Maske
	pZeile = pBild->puDaten;

	for( y = 0; y < h;  y++ ) {
		// Maske berücktsichtigen
		if( pBmp->pMaske ) {
			pMaske = pBmp->pMaske + ( h-y-1 )*pBmp->wMaskeW;
		}

		// Punkte zusammentragen ...
		for( i = x = 0;  x < w;  x++ ) {
			// ... so sie nicht maskiert sind!
			if( pMaske == 0  ||  ( pMaske[x/8]&( 0x0080>>( x%8 ) ) ) == 0 )	{
				pX[i] = x;
				pY[i] = pZeile[x];
				i++;
			}
		}

		// Hoffentlich bleib noch etwas von der Zeile zum Fitten übrig ...
		// ... Fitten ...
		if( i == 0  ||  !lfit( pX, pY, i, a, n, pCovar, &chi, fpoly ) )	{
			MemFree( pX );
			MemFree( pY );
			return ( FALSE );
		}

		for( x = 0;  x < w;  x++ ) {
			// ... und Polynom n-ter Ordnung abziehen ...
			lWert = (LONG)(ULONG)pZeile[x] - (LONG)fPolyWert( x, a, n );
			if( lWert < lMin ) {
				lMin = lWert;
			}
			else if( lWert > lMax ) {
				lMax = lWert;
			}
			pZeile[x] = (UWORD)lWert;
		}
		pZeile += w;
	}

	MemFree( pX );
	MemFree( pY );
	return ( BildMinMax( pBild, lMin, lMax, w, h ) );
}

typedef struct {
	LONG w, h, MaskeW;
	LPBYTE pMaske, pBmpMaske;
	LPUWORD pSource;
	LPUWORD pX;
	LPUWORD pY;
	LPLONG pDist;
	LPUWORD pData;
	LPULONG length;
	LONG lMin, lMax;
	double	ratio_square;
	int n;
	double *a;
} ADD_DDA_DATA;

void CALLBACK LineAddData( int x, int y, ADD_DDA_DATA HUGE *pDDA )
{
	if(
		x < pDDA->w  &&  y < pDDA->h  &&
		( pDDA->pBmpMaske ==  NULL  ||  ( pDDA->pBmpMaske[( pDDA->h-y-1 )*pDDA->MaskeW + (x/8)]&( 0x0080>>( x%8 ) ) ) == 0  )  &&
		pDDA->pMaske[( y*pDDA->w ) + x] == 0
	  ) {
		// untreated data point
		  long len = pDDA->length[0];
		pDDA->pX[len] = x;
		pDDA->pX[len] = y;
		pDDA->pDist[len] = (long)(sqrt( (x+x)+(y*y*pDDA->ratio_square) )*1000.0 + 0.5);
		pDDA->pData[len] = pDDA->pSource[x+y*pDDA->w];
		pDDA->length[0] ++;
	}
}

void CALLBACK LineApplyToData( int x, int y, ADD_DDA_DATA HUGE *pDDA )
{
	if(  x < pDDA->w  &&  y < pDDA->h  &&  pDDA->pMaske[( y*pDDA->w ) + x] == 0   ) {
		// untreated data point
		long xw = (long)(sqrt( (x+x)+(y*y*pDDA->ratio_square) )*1000.0 + 0.5);
		long lWert = pDDA->pSource[x+y*pDDA->w] - (LONG)fPolyWert( xw, pDDA->a, pDDA->n );
		pDDA->pSource[x+y*pDDA->w] = lWert;
		if(  lWert < pDDA->lMin  ) {
			pDDA->lMin = lWert;
		}
		if(  lWert > pDDA->lMax  ) {
			pDDA->lMax = lWert;
		}
		pDDA->pMaske[ ( y*pDDA->w ) + x ] = 1;
	}
}

/**** fitting an image with order n by fittig linewise under an angle ****/
BOOLEAN	MittelFitBildRotate( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n, double angle )
{
	LPUWORD pData;
	LPBYTE pMaske;
	LONG x, y, i, lData;
	LPUWORD pX, pY;
	LPLONG pDist;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], chi, a[10], tan_angle;
	ADD_DDA_DATA fit_dda;

	if(  fabs(angle)<0.001  ) {
		// much faster without angle
		return MittelFitBild( pBmp, pBild, w, h, n );
	}

	n++;
	if( pBmp == NULL  ||  pBild == NULL  ||  n > 10 ) { // Nur bis max. 9. Ordnung
		return ( FALSE );
	}

	if( ( pX = (LPUWORD)pMalloc( sizeof( int )*(w+h) ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( long )*(w+h) ) ) == NULL  ||
	    ( pDist = (LPLONG)pMalloc( sizeof( long )*(w+h) ) ) == NULL  ||
	    ( pData = (LPUWORD)pMalloc( sizeof( long )*(w+h) ) ) == NULL  ||
	    ( pMaske = (LPBYTE)pMalloc( sizeof( byte )*(w*h) ) ) == NULL
		) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	MemSet( pMaske, 0, w*h );

	for( x = 0;  x < 10;  x++ ) {
		pCovar[x] = CoMem+10*x;
	}

	fit_dda.w = w;
	fit_dda.h = h;
	fit_dda.MaskeW = pBmp->wMaskeW;
	fit_dda.pMaske = pMaske;
	fit_dda.pBmpMaske = pBmp->pMaske;
	fit_dda.pSource = pBild->puDaten;
	fit_dda.pX = pX;
	fit_dda.pY = pY;
	fit_dda.pDist = pDist;
	fit_dda.length = &lData;
	fit_dda.ratio_square = (pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].fY)/(pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].fX);
	fit_dda.a = a;
	fit_dda.n = n;
	fit_dda.lMin = 1111111;
	fit_dda.lMax = -1111111;

	// ensure we have always a positive gradient
	tan_angle = tan(angle);
	if(  tan_angle < 0.0  ) {
		tan_angle = -tan_angle;
	}

	for( y = 0; y < h;  y++ ) {
		// Now for each point draw a line under the angle to the border of the image
		lData = 0;
		fit_dda.pData = pData;
		LineDDA( 0, y, w, y+(int)(w*tan_angle+0.5), (LINEDDAPROC)LineAddData, (LPARAM)&fit_dda );

		if(  lData<n  ) {
			if(  lData>0  ) {
				// just substratct mean value
				LONG mean = 0;
				for(  i=0;  i<lData;  i++  ) {
					mean += pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ];
				}
				mean /= lData;
				for(  i=0;  i<lData;  i++  ) {
					if(  pMaske[( y*w ) + x] == 0  ) {
						long lWert = pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ] - mean;
						pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ] = lWert;
						if(  lWert < fit_dda.lMin  ) {
							fit_dda.lMin = lWert;
						}
						if(  lWert > fit_dda.lMax  ) {
							fit_dda.lMax = lWert;
						}
						pMaske[( y*w ) + x] = 1;
					}
				}
			}

		}
		else {
			// else full fit
			if(  !lfit( pDist, pData, lData, a, n, pCovar, &chi, fpoly ) )	{
				MemFree( pX );
				MemFree( pY );
				MemFree( pMaske );
				MemFree( pData );
				return ( FALSE );
			}

			for(  i=0;  i<lData;  i++  ) {
				fit_dda.pData = pBild->puDaten;
				LineDDA( 0, y, w, y+(int)(w*tan_angle+0.5), (LINEDDAPROC)LineApplyToData, (LPARAM)&fit_dda );
			}
		}
	}

	for( x = 0; x < w;  x++ ) {
		// Now for each point draw a line under the angle to the border of the image
		lData = 0;
		fit_dda.pData = pData;
		fit_dda.n = n;
		LineDDA( x, 0, x+w, (int)(w*tan_angle+0.5), (LINEDDAPROC)LineAddData, (LPARAM)&fit_dda );

		if(  lData<n  ) {
			if(  lData>0  ) {
				// just substratct mean value
				LONG mean = 0;
				for(  i=0;  i<lData;  i++  ) {
					mean += pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ];
				}
				mean /= lData;
				for(  i=0;  i<lData;  i++  ) {
					if(  pMaske[( y*w ) + x] == 0  ) {
						long lWert = pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ] - mean;
						pBild->puDaten[ fit_dda.pX[i] + fit_dda.pY[i]*w ] = lWert;
						if(  lWert < fit_dda.lMin  ) {
							fit_dda.lMin = lWert;
						}
						if(  lWert > fit_dda.lMax  ) {
							fit_dda.lMax = lWert;
						}
						pMaske[( y*w ) + x] = 1;
					}
				}
			}

		}
		else {
			// else full fit
			if(  !lfit( pDist, pData, lData, a, n, pCovar, &chi, fpoly ) )	{
				MemFree( pX );
				MemFree( pY );
				MemFree( pMaske );
				MemFree( pData );
				return ( FALSE );
			}

			for(  i=0;  i<lData;  i++  ) {
				fit_dda.pData = pBild->puDaten;
				LineDDA( x, 0, x+w, (int)(w*tan_angle+0.5), (LINEDDAPROC)LineApplyToData, (LPARAM)&fit_dda );
			}
		}
	}

	MemFree( pX );
	MemFree( pY );
	MemFree( pDist );
	MemFree( pMaske );
	MemFree( pData );
	return (1+ BildMinMax( pBild, fit_dda.lMin, fit_dda.lMax, w, h ) );
}



// Berechnet ein Polygon einer best-fit Ebenen furch die nicht maskierten Teile des Bildes
BOOLEAN	Fit3DBild( LPBMPDATA pBmp, LPUWORD puDaten, LONG w, LONG h, int n, double x_poly[], double y_poly[] )
{
	LPUWORD pZeile;
	LPBYTE pMaske;
	LONG x, y, i;
	LPLONG pMittel;
	LPLONG pAnzahl;
	LPUWORD pY;
	int *pX;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], chi;

	if( ( pAnzahl = (LPLONG)pMalloc( sizeof( long )*max( w, h ) ) ) == NULL  ||
	    ( pMittel = (LPLONG)pMalloc( sizeof( long )*max( w, h ) ) ) == NULL  ||
	    ( pX = (int*)pMalloc( sizeof( int )*max( w, h ) ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( UWORD )*max( w, h ) ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// all coefficients zero
	for( x = 0;  x <= n;  x++ ) {
		x_poly[x] = y_poly[x] = 0.0;
	}

	for( x = 0;  x < 10;  x++ ) {
		pCovar[x] = CoMem+10*x;
	}

	/**** Zeilenweise Mitteln ****/
	pMaske = NULL;  // Keine Maske

	for( y = 0; y < h;  y++ ) {
		// Maske berücktsichtigen
		if( pBmp->pMaske ) {
			pMaske = pBmp->pMaske + ( h-y-1 )*pBmp->wMaskeW;
		}

		// Punkte zusammentragen ...
		pAnzahl[y] = 0;
		pMittel[y] = 0;
		pZeile = puDaten+y*w;
		for( x = 0;  x < w;  x++ ) {
			// ... so sie nicht maskiert sind!
			if( pMaske == 0  ||  ( pMaske[x/8]&( 0x0080>>( x%8 ) ) ) == 0 )	{
				pAnzahl[y]++;
				pMittel[y] += pZeile[x];
			}
		}
	}

	for( y = i = 0;  y < h;  y++ ) {
		if( pAnzahl[y] > 0 ) {  // Gar keine Punkte zum Fitten übrig?
			pY[i] = pMittel[y]/pAnzahl[y];  // Y-Wert aller Zeilen
			pX[i] = y;      // dito. X-Wert
			i++;
		}
	}

	// Hoffentlich bleib noch etwas von der Zeile zum Fitten übrig ...
	if( i == 0  ||  !lfit( pX, pY, i, y_poly, n, pCovar, &chi, fpoly ) ) {
		MemFree( pAnzahl );
		MemFree( pMittel );
		MemFree( pX );
		MemFree( pY );
		return ( FALSE );
	}


	/**** Spaltenweise Mitteln ****/
	pMaske = NULL;  // Keine Maske

	for( x = 0; x < w;  x++ ) {
		// Maske berücktsichtigen
		i = ( 0x0080>>( x%8 ) );
		if( pBmp->pMaske ) {
			pMaske = pBmp->pMaske+( x/8 );
		}

		// Punkte zusammentragen ...
		pAnzahl[x] = 0;
		pMittel[x] = 0;
		pZeile = puDaten+x;
		for( y = 0;  y < h;  y++ ) {
			// ... so sie nicht maskiert sind!
			if( pMaske == 0  ||  ( pMaske[( h-y-1 )*pBmp->wMaskeW]&i ) == 0 ) {
				pAnzahl[x]++;
				pMittel[x] += (LONG)pZeile[y*w] - (LONG)fPolyWert( y, y_poly, n );
			}
		}
	}

	for( x = i = 0;  x < w;  x++ ) {
		if( pAnzahl[x] > 0 ) {  // Gar keine Punkte zum Fitten übrig?
			pY[i] = pMittel[x]/pAnzahl[x];  // Y-Wert aller Zeilen
			pX[i] = x;      // dito. X-Wert
			i++;
		}
	}

	// Hoffentlich bleib noch etwas von der Zeile zum Fitten übrig ...
	if( i == 0  ||  !lfit( pX, pY, i, x_poly, n, pCovar, &chi, fpoly ) ) {
		MemFree( pAnzahl );
		MemFree( pMittel );
		MemFree( pX );
		MemFree( pY );
		return ( FALSE );
	}

	MemFree( pAnzahl );
	MemFree( pMittel );
	MemFree( pX );
	MemFree( pY );
	return ( TRUE );
}
// 1.7.2002


/****	Mittelung eines Bildes analytisch ****
 **** Vorher werden jedoch alle unmaskierten Zeilen/Spalten gemittelt
 **** n ist die Ordnung der Mittelung
 **** xy_what gibt an ob x(Bit 0) und/oder y (Bit 1) gefitted werden sollen ...  ****/
BOOLEAN	MittelFit3DBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n, int xy_what )
{
	LPUWORD pZeile;
	LPBYTE pMaske;
	LONG x, y, i, lWert, lPolyWert, lMin, lMax;
	LPLONG pMittel;
	LPUWORD	pAnzahl;
	int *pX;
	UWORD *pY;
	LPDOUBLE( pCovar[10] );
	double CoMem[100], a[10], chi;

	n++;
	if( pBmp == NULL  ||  pBild == NULL  ||  n > 10  ||  ( xy_what&3 ) == 0 ) {     // Nur bis max. 9. Ordnung
		return ( FALSE );
	}

	if( ( pAnzahl = (LPWORD)pMalloc( sizeof( long )*max( w, h ) ) ) == NULL  ||
	    ( pMittel = (LPLONG)pMalloc( sizeof( long )*max( w, h ) ) ) == NULL  ||
	    ( pX = (int*)pMalloc( sizeof( int )*max( w, h ) ) ) == NULL  ||
	    ( pY = (LPUWORD)pMalloc( sizeof( UWORD )*max( w, h ) ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	for( x = 0;  x < 10;  x++ ) {
		pCovar[x] = CoMem+10*x;
	}

	if( xy_what&2 )	{
		/**** Zeilenweise Mitteln ****/

		pMaske = NULL;  // Keine Maske

		for( y = 0; y < h;  y++ ) {
			// Maske berücktsichtigen
			if( pBmp->pMaske ) {
				pMaske = pBmp->pMaske + ( h-y-1 )*pBmp->wMaskeW;
			}

			// Punkte zusammentragen ...
			pAnzahl[y] = 0;
			pMittel[y] = 0;
			pZeile = pBild->puDaten+y*w;
			for( x = 0;  x < w;  x++ ) {
				// ... so sie nicht maskiert sind!
				if( pMaske == 0  ||  ( pMaske[x/8]&( 0x0080>>( x%8 ) ) ) == 0 )	{
					pAnzahl[y]++;
					pMittel[y] += pZeile[x];
				}
			}
		}

		for( y = i = 0;  y < h;  y++ ) {
			if( pAnzahl[y] > 0 ) {  // Gar keine Punkte zum Fitten übrig?
				pY[i] = pMittel[y]/pAnzahl[y];  // Y-Wert aller Zeilen
				pX[i] = y;      // dito. X-Wert
				i++;
			}
		}

		// Hoffentlich bleib noch etwas von der Zeile zum Fitten übrig ...
		if( i == 0  ||  !lfit( pX, pY, i, a, n, pCovar, &chi, fpoly ) )	{
			MemFree( pAnzahl );
			MemFree( pMittel );
			MemFree( pX );
			MemFree( pY );
			return ( FALSE );
		}

		lMin = 32767l;  // Größte Differenz zum MittelWert, deshalb mit 0 init.
		lMax = -32768l;

		// ... Fitten war ok ...
		for( y = 0; y < h;  y++ ) {
			pZeile = pBild->puDaten+y*w;
			// ... und Polynom n-ter Ordnung abziehen ...
			lPolyWert = (LONG)fPolyWert( y, a, n );
			for( x = 0;  x < w;  x++ ) {
				lWert = (LONG)(ULONG)pZeile[x] - lPolyWert;
				if( lWert < lMin ) {
					lMin = lWert;
				}
				else if( lWert > lMax ) {
					lMax = lWert;
				}
				pZeile[x] = (UWORD)lWert;
			}
		}
		// Mitteln, sonst scheitert nächster Fit
		if( !BildMinMax( pBild, lMin, lMax, w, h ) ) {
			MemFree( pAnzahl );
			MemFree( pMittel );
			return ( FALSE );
		}
		// if x-Mitteln
	}

	if( xy_what&1 )	{
		/**** Spaltenweise Mitteln ****/

		pMaske = NULL;  // Keine Maske
		for( x = 0; x < w;  x++ ) {
			// Maske berücktsichtigen
			i = ( 0x0080>>( x%8 ) );
			if( pBmp->pMaske ) {
				pMaske = pBmp->pMaske+( x/8 );
			}

			// Punkte zusammentragen ...
			pAnzahl[x] = 0;
			pMittel[x] = 0;
			pZeile = pBild->puDaten+x;
			for( y = 0;  y < h;  y++ ) {
				// ... so sie nicht maskiert sind!
				if( pMaske == 0  ||  ( pMaske[( h-y-1 )*pBmp->wMaskeW]&i ) == 0 ) {
					pAnzahl[x]++;
					pMittel[x] += pZeile[y*w];
				}
			}
		}

		for( x = i = 0;  x < w;  x++ ) {
			if( pAnzahl[x] > 0 ) {  // Gar keine Punkte zum Fitten übrig?
				pY[i] = pMittel[x]/pAnzahl[x];  // Y-Wert aller Zeilen
				pX[i] = x;      // dito. X-Wert
				i++;
			}
		}

		// Hoffentlich bleib noch etwas von der Zeile zum Fitten übrig ...
		if( i == 0  ||  !lfit( pX, pY, i, a, n, pCovar, &chi, fpoly ) )	{
			MemFree( pAnzahl );
			MemFree( pMittel );
			MemFree( pX );
			MemFree( pY );
			return ( FALSE );
		}

		MemFree( pAnzahl );
		MemFree( pMittel );
		MemFree( pX );
		MemFree( pY );

		lMin = 32767l;  // Größte Differenz zum MittelWert, deshalb mit 0 init.
		lMax = -32768l;

		// ... Fitten war ok ...
		for( x = 0; x < w;  x++ ) {
			pZeile = pBild->puDaten+x;
			// ... und Polynom n-ter Ordnung abziehen ...
			lPolyWert = (LONG)fPolyWert( x, a, n );
			for( y = 0;  y < h;  y++ ) {
				lWert = (LONG)(ULONG)pZeile[y*w] - lPolyWert;
				if( lWert < lMin ) {
					lMin = lWert;
				}
				else if( lWert > lMax ) {
					lMax = lWert;
				}
				pZeile[y*w] = (UWORD)lWert;
			}
		}
		// Und mitteln ...
		if( !BildMinMax( pBild, lMin, lMax, w, h ) ) {
			return ( FALSE );
		}
		// if y-Mitteln
	}
	return ( TRUE );
}
// 24.2.00


// Fill routines
// Attention, dirty coding here
// to save stack size, we are using lots of global and inlines!!!

LPBYTE pFillArea;
LONG iFillAreaWW, iFillW, iFillH;
LPUWORD	pFillData;
UWORD iFillLevel, FillHigher;

int GetFillPixel( LONG x, LONG y )
{
	return ( ( pFillArea[( iFillH-1-y )*iFillAreaWW+( x>>3 )] & ( 0x080>>( x&7 ) ) ) != 0 );
}


void SetFillPixel( LONG x, LONG y )
{
	pFillArea[( iFillH-1-y )*iFillAreaWW+( x>>3 )] |= (BYTE)( 0x080>>( x&7 ) );
}


// Check, if we want to fill this point
BOOLEAN	CheckFillPixel( WORD xt, WORD yt )
{
	if( GetFillPixel( xt, yt ) ) {
		return ( FALSE );
	}
	if( FillHigher  &&  iFillLevel >= pFillData[iFillW*yt+xt] ) {
		return ( TRUE );
	}
	if( !FillHigher  &&  iFillLevel <= pFillData[iFillW*yt+xt] ) {
		return ( TRUE );
	}
	return ( FALSE );
}


/* A diamond flood-fill using a circular queue system.
   Each pixel surrounding the current pixel is added to
   the queue if it meets the criteria, then is retrieved in
   its turn. */
VOID AreaFill( WORD xt, WORD yt )
{
	WORD *pSave, *pStart, *pRead;    //queue save, start, read
	LONG iQueueSize = iFillW*iFillH*2;      // Number elements

	if( !CheckFillPixel( xt, yt ) ) {
		return;
	}

	pSave = (WORD*)pMalloc( iQueueSize*sizeof( WORD ) );
	//memset(pSave,qSsz*sizeof(WORD),0); //Clear the contents
	pStart = pRead = pSave;

	// Save first element
	*pStart++ = xt;
	*pStart++ = yt;

	//Main queue loop
	while( pRead != pStart ) {
		//Add new members to queue
		//Above current pixel
		if( yt > 0  &&  CheckFillPixel( xt, yt-1 ) ) {
			*pStart++ = xt;
			*pStart++ = yt-1;
			SetFillPixel( xt, yt-1 );
			if( pStart >= pSave+iQueueSize ) {
				pStart = pSave; //Loop back to beginning of queue
			}
		}
		//Below current pixel
		if( yt < iFillW-1  &&  CheckFillPixel( xt, yt+1 ) ) {
			*pStart++ = xt;
			*pStart++ = yt+1;
			SetFillPixel( xt, yt+1 );
			if( pStart >= pSave+iQueueSize ) {
				pStart = pSave; //Loop back to beginning of queue
			}
		}
		//Left of current pixel
		if( xt > 0  &&  CheckFillPixel( xt-1, yt ) ) {
			*pStart++ = xt-1;
			*pStart++ = yt;
			SetFillPixel( xt-1, yt );
			if( pStart >= pSave+iQueueSize ) {
				pStart = pSave; //Loop back to beginning of queue
			}
		}
		//Right of current pixel
		if( xt <= iFillW-1  &&  CheckFillPixel( xt+1, yt ) ) {
			*pStart++ = xt+1;
			*pStart++ = yt;
			SetFillPixel( xt+1, yt );
			if( pStart >= pSave+iQueueSize ) {
				pStart = pSave; //Loop back to beginning of queue
			}
		}
		//Retrieve current queue member
		xt = *pRead++;
		yt =	*pRead++;
		if( pRead >= pSave+iQueueSize ) {
			pRead = pSave; //Loop back to beginning of queue
		}
	} //Back to beginning of loop
	  //Free the memory
	MemFree( pSave );
}


// Modified from source of DrawIt-Palm drawing program


// Mark all neighbouring points higher (lower) as the startpoint in the bitmap pArea (iAreaWW wide)
void MarkUpperArea( LPUWORD puData, LONG w, LONG h, LPBYTE pArea, LONG iAreaWW, LONG iStartX, LONG iStartY, UWORD iLevel )
{
	ASSERT( puData != NULL  &&  pArea != NULL  &&  iStartX <= w  &&  iStartY <= h  );

	pFillData = puData;
	iFillLevel = iLevel;
	FillHigher = iFillLevel > puData[w*iStartY+iStartX];
	iFillW = w;
	iFillH = h;
	pFillArea = pArea;
	iFillAreaWW = iAreaWW;
	AreaFill( iStartX, iStartY );
}


// End of dirty coding alert


// returns the triangle surface area, the differenz for the three points is (dx,dy): (1,0), (0,1), (1,1)
double TriangleArea( UWORD z00, UWORD z01, UWORD z11, double x2, double y2, double z2 )
{
	double a = sqrt( ( z00-z01 )*( z00-z01 )*z2+x2 ), b = sqrt( ( z01-z11 )*( z01-z11 )*z2+y2 ), c = sqrt( ( z00-z11 )*( z00-z11 )*z2+x2+y2 );
	double s = 0.5*( a+b+c );
	return ( sqrt( s*( s-a )*( s-b )*( s-c ) ) );
}
// 14.6.02



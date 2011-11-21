/**************************************************************************************
****	Mathematische und verwandte Routinen
**************************************************************************************/


/* Achtung: NR => Basis der Array ist a[1]!!! */
void realft(LPFLOAT data, LONG n, int isign);

// Polynom n-ter Ordnung fuer MittelFitBild und andere ...
double	fPolyWert( double x, LPDOUBLE pf, int n );

// Macht eine FFT-Filterung; die Frequenzwerte werden mit den Werten 
// aus pfFilter multipliziert. ACHTUNG pfFilter muss iSize gross sein!
BOOLEAN	BildFFTFilter( LPBILD pBild, LONG w, LONG h, LPFLOAT pfFilter, int iSize );
// 9.2.99

// Autokorrelation berechnen
BOOLEAN	Autokorrelation( LPFLOAT pfZiel, LPFLOAT pfDaten, int iMaxPts, BOOLEAN UseZeroMean );
// 4/98

// Numerisch Differenzieren
BOOLEAN	Differential( LPFLOAT pfDaten, int iPunkte );
// 10.11.98

// Correlation function (berechnet Korrelationslänge)
void	CorrelationFunction( HFILE hFile, LPUWORD puDaten, LONG w, LONG h, double dSkal );

/****	Berechnet die mittlere Rauhigkeit und die mittlere Höhe im Bild ****/
void	RMSArea( LPUWORD puDaten, LONG ww, LONG x, LONG y, LONG w, LONG h,
							 double dSkal, LPDOUBLE pfMeanH, LPDOUBLE pfRMS, LPFLOAT pfQuadrate );
// 18.2.98

void	MeadianArea( LPUWORD puDaten, LONG ww, LONG x, LONG y, LONG w, LONG h, double dSkal, UWORD maxhgt, LPDOUBLE pfMedian, LPDOUBLE pfRMS );
// 18.11.11

/****	Berechnet die mittlere Rauhigkeit und die mittlere Höhe auf einer Scanline ****/
void	RMSLine( LPUWORD puDaten, LONG x, LONG y, LONG w, LONG h, LONG ww, double m, double fSkal, LPDOUBLE pfMeanH, LPDOUBLE pfRMS );
// 18.2.98

/****	Rechnet mit Bild und Wert ****/
BOOLEAN	BildCalcConst( LPBILD pDestBild, LONG w, LONG h, UCHAR cOperand, double wert, BOOL bOverflow );
// 9.6.97, 9.8.98

/****	Rechnet mit 2 Bildern ****/
BOOL	BildCalcBild( LPBMPDATA pDest, WORKMODE DestMode, WORD cOperand, LPBMPDATA puWerte, WORKMODE WerteMode, BOOL bOverflow );
// 9.6.97, 9.8.98

// Bestimmt die Mittelwerte in einer Zeile (für MittelBild und den Ausgleich beim Messen)
BOOLEAN	MittelZeile( LPUWORD puZeile, LPUCHAR puMaske, LONG w, LPDOUBLE pfMittel, ULONG n );
// 26.11.97, 31.7.98

/****	Mittelung eines Bildes analytisch zeilenweise ****/
BOOLEAN	MittelBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n );
// 26.11.97, 31.7.98

/****	Mittelung eines Bildes analytisch zeilenweise ****/
BOOLEAN	MittelFitBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n );
// 26.11.97, 31.7.98

/* Interpoliert die maskierte Region horizontal durch ein Polynom n. Grades */
BOOLEAN	InterpolateVertikalBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int grad );
// 20.12.98

/* Interpoliert die maskierte Region horizontal durch ein Polynom n. Grades */
BOOLEAN	InterpolateHorizontalBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int grad );
// 20.12.98

// Berechnet ein Polygon einer best-fit Ebenen furch die nicht maskierten Teile des Bildes
BOOLEAN	Fit3DBild( LPBMPDATA pBmp, LPUWORD puData, LONG w, LONG h, int n, double *x_poly, double *y_poly );
// 1.7.02

/****	Mittelung eines Bildes analytisch ****/
/**** Vorher werden jedoch alle unmaskierten Zeilen/Spalten gemittelt ****/
/**** n ist die Ordnung der Mittelung; ****/
/**** xy_what gibt an ob x(Bit 0) und/oder y (Bit 1) gefitted werden sollen ...  ****/
BOOLEAN	MittelFit3DBild( LPBMPDATA pBmp, LPBILD pBild, LONG w, LONG h, int n, int xy_what );
// 24.2.00

// Mark all neighbouring points higher (lower) as the startpoint iLevel in the bitmap pArea (iAreaWW wide)
void MarkUpperArea( LPUWORD puData, LONG w, LONG h, LPBYTE pArea, LONG iAreaWW, LONG iStartX, LONG iStartY, UWORD iLevel );
// 22.4.02

// Calc triangle surface area (not trivial!)
double	TriangleArea( UWORD z00, UWORD z01, UWORD z11, double x2, double y2, double z2 );
// 14.6.02


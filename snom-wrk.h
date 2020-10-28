/**************************************************************************************
****	SNOM-WRK:	Bildbe- und Verarbeitungsroutinen
**************************************************************************************/


// Kümmert sich um Überlauf etc.
// Wenn die Differenz zu groß (= Überlauf aufgetreten), wird FALSE zurückgeliefert
BOOLEAN	BildMinMax( LPBILD pBild, const LONG lMin, const LONG lMax, const LONG w, const LONG h );
// 24.10.98


/****	Setzt minimalen Wert auf Null  ****/
BOOLEAN	BildMax( LPBILD pBild, LONG w, LONG h );
// 26.11.97

/* Berechnet die häufigste Steigung von links nach rechts und zieht diese wieder ab
 * TRUE, wenn erfolgreich
 */
BOOLEAN	BildSteigungX( LPBILD pBild, LONG lTeiler, const LONG w, const LONG h );
// 24.10.98


/* Berechnet die häufigste Differenz zweier Zeilen und zeiht diese ab
 * TRUE, wenn erfolgreich
 */
BOOLEAN	BildSteigungY( LPBILD pBild, const LONG lTeiler, const LONG w, const LONG h );
// 24.10.98


// Berechnet ein gleitendes Mittel
// TRUE, wenn erfolgreich
BOOLEAN	BildGleitendesMittel( LPBILD pBild, LONG lPunkte, const LONG w, const LONG h );
// 24.10.98


// Arera average, right bottom border bocme constant
// TRUE, wenn erfolgreich
BOOLEAN	BildGleitendesMittel2D( LPBILD pBild, LONG lPunkte, const LONG w, const LONG h );
// 24.10.98


// Negativ berechnen
// TRUE (immer erfolgreich)
BOOLEAN	BildNegieren( LPBILD pBild, const LONG w, const LONG h );
// 23.10.96


// Zeilenweise differenzieren
BOOLEAN	BildDifferential( LPBILD pBild, LONG w, LONG h );
// 11.11.98


/* Simpelste numerische Integration */
BOOLEAN	BildIntegral( LPBILD pBild, LONG w, LONG h );
// 11.11.98


// Einfaches Despiken
BOOLEAN	BildDespike( LPBILD pBild, LONG w, LONG h, LONG lSpikeX, LONG lSpikeY, UWORD uLowLim, UWORD uHighLim, 
										 BOOLEAN bX, BOOLEAN bY, BOOLEAN bLowLim, BOOLEAN bUpLim, BOOLEAN bInterpolate );
// 3.1.99

// Berechnet spaltengemittelte Medianmittelung
void	BildMedianSpalten( LPUWORD puDaten, LONG w, LONG h, WORD iAnzahl );
// 20.9.98


// Berechnet Medianmittelung
BOOLEAN	BildMedian( LPUWORD puDaten, LONG w, LONG h, WORD iAnzahl );
// 14.10.98


/**************************************************************************************
****	SNOM-WRK:	Bildbe- und Verarbeitungsroutinen
**************************************************************************************/

#include "snomputz.h"
#include "snom-typ.h"
#include "snom-mem.h"
#include "snom-win.h"
#include "snom-wrk.h"


/*************************************************************************************/
/* Zuerst Hilfsfunktionen */


// Kümmert sich um Überlauf etc.
// Wenn die Differenz zu groß (= Überlauf aufgetreten), wird FALSE zurückgeliefert
BOOLEAN	BildMinMax( LPBILD pBild, const LONG lMin, const LONG lMax, const LONG w, const LONG h )
{
	BOOLEAN	bKeinUeberlauf = TRUE;
	LONG i;

	ASSERT(  lMin <= lMax  &&  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256  );

	// vor evt. Überlauf warnen
	if( lMax-lMin >= 65535l ) {
		CHAR str[256];

		// Windows-spezifisch!
		StatusLineRsc( W_OVERFLOW );
		wsprintf( (LPSTR)str, (LPSTR)GetStringRsc( DO_NO_OVERFLOW ), ( lMax-lMin+65536l )/65536l );
		Warnung( str );
		pBild->uMaxDaten = 0xFFFFu;
		bKeinUeberlauf = FALSE;
	}
	else {
		pBild->uMaxDaten = (WORD)( lMax-lMin )+1;
	}

	// Minimum abziehen, wenn nötig
	if( lMin != 0 )	{
		for( i = 0;  i < w*h;  i++ ) {
			pBild->puDaten[i] -= lMin;
		}
	}

	return ( bKeinUeberlauf );
}
// 24.10.98


/****	Setzt minimalen Wert auf Null  ****/
/* benutzt ebenfalss BildMinMax */
BOOLEAN	BildMax( LPBILD pBild, LONG w, LONG h )
{
	LPUWORD	pSrc;
	LONG i;
	WORD uMin, uMax;

	ASSERT(  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256  );

	uMin = 0xffffu;
	uMax = 0;
	pSrc = pBild->puDaten;
	for( i = 0;  i < w*h;  i++ ) {
		if( pSrc[i] > uMax ) {
			uMax = pSrc[i];
		}
		else if( pSrc[i] < uMin ) {
			uMin = pSrc[i];
		}
	}
	return ( BildMinMax( pBild, (ULONG)uMin, (ULONG)uMax, w, h ) );
}
// 26.11.97


/*************************************************************************************/
/* Und nun die "echten" Routinen */


/* Berechnet die häufigste Steigung von links nach rechts und zieht diese wieder ab
 * TRUE, wenn erfolgreich
 */
BOOLEAN	BildSteigungX( LPBILD pBild, LONG lTeiler, const LONG w, const LONG h )
{
	LPUWORD	puDaten = pBild->puDaten;
	LPLONG plAnzahl, plZeile;
	LONG lDelta, lMax, lMin, lMaxPos;
	LONG lM, lMDiv;
	LONG x, y, i;

	ASSERT(  lTeiler > 0  &&  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	// Array für temporäre Daten
	i = 512;
	if( w > 512 ) {
		i = w;
	}
	plAnzahl = (LPLONG)pMalloc( sizeof( LONG )*i );
	if( plAnzahl == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// Zuerst Häufigkeiten der Steigung im Bild bestimmen
	// Es werden nur "kleine" Steigungn um +- 512 Unterschied pro Pixel berücktsichtigt
	for( y = 0;  y < h;  y++ ) {
		for( x = 1;  x < w;  x++ ) {
			lDelta = 256l + ( (LONG)puDaten[x]-(LONG)puDaten[x-1] )/lTeiler;
			if( lDelta >= 0  &&  lDelta < 512 ) {
				plAnzahl[lDelta]++;
			}
		}
	}

	// Und nun häufigste Steigung bestimmen
	lMaxPos = lMax = 0;
	for( x = 1;  x < 512;  x++ ) {
		if( plAnzahl[x] > lMax ) {
			lMax = plAnzahl[x];
			lMaxPos = x;
		}
	}

	// Kein echtes Maxima gefunden?
	if( lMaxPos == 0  ||	 lMax <= 15 ) {
		WarnungRsc( W_STATISTIK );
		MemFree( plAnzahl );
		return ( FALSE );
	}

	// Nun die Steigung berechnen
	// m = dm/dmdiv für Ganzkommaarithmetrik
	lM = lMax*( lMaxPos-256 ) + plAnzahl[lMaxPos-1]*( lMaxPos-257 ) + plAnzahl[lMaxPos+1]*( lMaxPos-255 );
	lMDiv = lMax + plAnzahl[lMaxPos-1] + plAnzahl[lMaxPos+1];

	// Differenz aufaddieren
	// Zuerst berechnen
	lMin = 100000l;
	lMax = -100000l;
	plZeile = plAnzahl;
	for( x = 0;  x < w;  x++ ) {
		plZeile[x] = ( ( x*lM*lTeiler )/lMDiv );
	}

	// Und nun von jeder Zeile abziehen
	for( y = 0;  y < h;  y++ ) {
		puDaten = pBild->puDaten+y*w;
		for( x = 0;  x < w;  x++ ) {
			i = (LONG)(ULONG)puDaten[x];
			i -= plZeile[x];
			if( i < lMin ) {
				lMin = i;
			}
			if( i > lMax ) {
				lMax = i;
			}
			puDaten[x] = (UWORD)i;
		}
	}
	MemFree( plAnzahl );

	// Minimum korrigieren
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}
// 24.10.98


/* Berechnet die häufigste Differenz zweier Zeilen und zieht diese ab
 * TRUE, wenn erfolgreich
 */
BOOLEAN	BildSteigungY( LPBILD pBild, const LONG lTeiler, const LONG w, const LONG h )
{
	LPSWORD	psDelta;
	LPUWORD	puDaten = pBild->puDaten;
	LPUWORD	puZeile, puZeileDavor;
	double fYDiff;
	WORD plAnzahl[2048];
	LONG lDelta, lMax, lMin, lMaxPos;
	LONG lM, lMDiv;
	LONG x, y, i;
	BOOLEAN	bStatistikWarnung = FALSE;

	ASSERT(  lTeiler > 0  &&  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	// Array für temporär Daten
	psDelta = (LPSWORD)pMalloc( sizeof( SWORD )*h );
	if( psDelta == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// Zuerst Häufigkeiten der Steigung im Bild bestimmen
	// Es werden nur "kleine" Steigungn um +- 512 Unterschied pro Pixel berücktsichtigt
	puZeile = pBild->puDaten;
	fYDiff = 0.0;
	psDelta[0] = 0;

	// Differenz aufaddieren
	// Zuerst berechnen
	lMin = 100000l;
	lMax = -100000l;

	for( y = 1;  y < h;  y++ ) {
		MemSet( plAnzahl, 0, sizeof( WORD )*2048 );
		puZeileDavor = puZeile;
		puZeile += w;

		// Differenzen zählen
		for( x = 0;  x < w;  x++ ) {
			lDelta = 1024l + ( (LONG)(ULONG)puZeileDavor[x]-(LONG)(ULONG)puZeile[x] )/lTeiler;
			if( lDelta > 0  &&  lDelta < 2048l ) {
				plAnzahl[lDelta]++;
			}
		}

		// Und nun häufigste Steigung bestimmen
		lMaxPos = lMax = 0;
		for( x = 0;  x < 2048;  x++ ) {
			if( plAnzahl[x] > lMax ) {
				lMax = plAnzahl[x];
				lMaxPos = x;
			}
		}

		// Kein echtes Maxima gefunden?
		if( lMaxPos == 0  ||  lMaxPos == 2048  ||  lMax <= 5 ) {
			if( !bStatistikWarnung ) {
				WarnungRsc( W_STATISTIK );
			}
			bStatistikWarnung = TRUE;
			psDelta[y] = (SWORD)fYDiff;
		}
		else {
			// Nun die Steigung berechnen
			// m = dm/dmdiv für Ganzkommaarithmetrik
			lM = lMax*( lMaxPos-1024l ) + plAnzahl[lMaxPos-1]*( lMaxPos-1025l ) + plAnzahl[lMaxPos+1]*( lMaxPos-1023l );
			lMDiv = lMax + plAnzahl[lMaxPos-1] + plAnzahl[lMaxPos+1];
			fYDiff += (double)( lM*lTeiler )/(double)lMDiv;   // Korrekturwert
			psDelta[y] = (SWORD)fYDiff;
		}
	}

	for( y = 0; y < h;  y++ ) {
		puDaten = pBild->puDaten+y*w;
		lDelta = psDelta[y];
		for( x = 0;  x < w;  x++ ) {
			i = (LONG)(ULONG)puDaten[x];
			i += lDelta;
			if( i < lMin ) {
				lMin = i;
			}
			if( i > lMax ) {
				lMax = i;
			}
			puDaten[x] = (UWORD)i;
		}
	}
	MemFree( psDelta );

	// Minimum korrigieren
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}
// 24.10.98


// Berechnet ein gleitendes Mittel
// TRUE, wenn erfolgreich
BOOLEAN	BildGleitendesMittel( LPBILD pBild, LONG lPunkte, const LONG w, const LONG h )
{
	LPLONG plMittel;
	LPUWORD	puZeile;
	LONG lMax, lMin;
	LONG x, y, i;

	ASSERT(  lPunkte > 1  &&  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	// Zwischenspeicher anlegen
	plMittel = (LPLONG)pMalloc( w*sizeof( LONG ) );
	if( plMittel == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// Es is immer eine ungerade Anzahl von Punkten, so oder so ...
	lPunkte /= 2;

	// Nun mitteln
	puZeile = pBild->puDaten;
	lMin = 100000l;
	lMax = -100000l;
	for( y = 0; y < h;  y++ ) {
		// Mittelwerte berechnen
		for( x = 0;  x < w;  x++ ) {
			plMittel[x] = 0;
			for( i = x-lPunkte; i <= x+lPunkte; i++ ) {
				if( i < 0 ) {
					plMittel[x] += puZeile[0];
				}
				else if( i >= w ) {
					plMittel[x] += puZeile[w-1];
				}
				else {
					plMittel[x] += puZeile[i];
				}
			}
			plMittel[x] /= 2*lPunkte+1;
		}

		// Und Werte zuweisen
		for( x = 0;  x < w;  x++ ) {
			i = (ULONG)plMittel[x];
			if( lMax < i ) {
				lMax = i;
			}
			if( lMin > i ) {
				lMin = i;
			}
			puZeile[x] = (UWORD)i;
		}

		puZeile += w;
	}

	// Minimum korrigieren
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}
// 24.10.98


// Negativ berechnen
// TRUE (immer erfolgreich)
BOOLEAN	BildNegieren( LPBILD pBild, const LONG w, const LONG h )
{
	LONG l = w*h;
	LPUWORD	puPtr = pBild->puDaten;
	UWORD uMaxDaten = pBild->uMaxDaten;

	ASSERT(  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	while( l-- > 0 ) {
		// Da alles unsigned geht das klar ...
		*puPtr = uMaxDaten-*puPtr-1;
		puPtr++;
	}
	return ( TRUE );
}
// 23.10.96


// Berechnet einen Konturplot
void KonturenBerechen( LPUWORD puDaten, LONG w, LONG h, UWORD uDifferenz )
{
	LPUWORD puZeile;
	LPUWORD	puDest, puDestZeile;
	UWORD uLastKontur;
	LONG x, y;

	ASSERT(  w*h > 0   &&  (LONG)puDaten > 256  &&  uDifferenz != 0 );

	// Konturen herausfinden; dabei auf Grenzen nach OBEN und UNTEN zusätzlich achten!
	puDest = (LPUWORD)pMalloc( sizeof( UWORD )*h*w );
	if( puDest == NULL ) {
		StatusLineRsc( E_MEMORY );
		return;
	}

	// Zuerst Zeilenweise
	for( y = 0;  y < h;  y++ ) {
		puZeile = puDaten+y*w;
		puDestZeile = puDest+( y*w );
		uLastKontur = ( *puZeile++ )/uDifferenz;
		*puDestZeile++ = 1;
		for( x = 1;  x < w-1;  x++ ) {
			if( *puZeile/uDifferenz != uLastKontur ) {
				uLastKontur = *puZeile/uDifferenz;
				*puDestZeile++ = 0;
			}
			else {
				*puDestZeile++ = 1;
			}
			puZeile++;
		}
		*puDestZeile = 1;
	}

	// Dann Spaltenweise
	for( x = 0;  x < w;  x++ ) {
		puZeile = puDaten+x;
		puDestZeile = puDest+x+w;
		uLastKontur = *puZeile/uDifferenz;
		puZeile += w;
		for( y = 1;  y < h-1;  y++ ) {
			if( *puZeile/uDifferenz != uLastKontur ) {
				uLastKontur = *puZeile/uDifferenz;
				*puDestZeile = 0;
			}
			puZeile += w;
			puDestZeile += w;
		}
	}

	MemMove( puDaten, puDest, sizeof( WORD )*w*h );
	MemFree( puDest );
}
// 24.2.98 (Routine uralt ...)


/**************************************************************************************
 * Numerik
 */
BOOLEAN	BildDifferential( LPBILD pBild, LONG w, LONG h )
{
	LPUWORD	puDaten = pBild->puDaten;
	LONG lTemp, x, y;
	LONG lMax = -65536l, lMin = +65536l;
	UWORD uLastDaten;

	ASSERT(  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	for( y = 0;  y < h;  y++ ) {
		uLastDaten = 0;
		for( x = 0;  x < w;  x++ ) {
			// ulLastDaten == puDaten[x-1] unverändert, bzw == 0 für x=0
			if( x < w-3 ) {
				lTemp = ( 0l - 11l*(LONG)(ULONG)puDaten[x] + 18l*(LONG)(ULONG)puDaten[x+1] - 9l*(LONG)(ULONG)puDaten[x+2] + 2*(LONG)(ULONG)puDaten[x+3] )/6l;
			}
			else if( x < w-1 ) {
				lTemp = ( 2l*(LONG)(ULONG)puDaten[x+1] - (LONG)(ULONG)uLastDaten - (LONG)(ULONG)puDaten[x] )/3l;
			}
			if( lTemp < lMin ) {
				lMin = lTemp;
			}
			if( lTemp > lMax ) {
				lMax = lTemp;
			}
			uLastDaten = puDaten[x];        // wird nur für x>=w benötigt
			puDaten[x] = (UWORD)lTemp;
		}
		puDaten += w;
	}

	// Minimum korrigieren
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}
/* 10.11.98 */


/* Simpelste numerische Integration */
BOOLEAN	BildIntegral( LPBILD pBild, LONG w, LONG h )
{
	LPUWORD	puDaten;
	LPLONG plAnzahl;
	LONG lTemp, x, y, lMittel;
	LONG lMax = -65536l, lMin = +65536l;
	int i;

	ASSERT(  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	puDaten = pBild->puDaten;
	plAnzahl = (LPLONG)pMalloc( sizeof( LONG )*( pBild->uMaxDaten+1l ) );
	if( plAnzahl == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}


	// Zuerst den häufigsten Wert als Steigung Null setzten
	for( x = 0;  x < h*w;  x++ ) {
		plAnzahl[puDaten[x]]++;
	}
	lTemp = 0;
	lMittel = 0;
	for( i = 0;  i < pBild->uMaxDaten;  i++ ) {
		if( plAnzahl[i] > lTemp ) {
			lTemp = plAnzahl[i];
			lMittel = i;
		}
	}
	MemFree( plAnzahl );
	lMittel *= 2;   // Mittelwert mal 2

	for( y = 0;  y < h;  y++ ) {
		lTemp = 0;
		for( x = 0;  x < w;  x++ ) {
			if( x < w-1 ) {
				lTemp += ( (LONG)(ULONG)puDaten[x+1] + (LONG)(ULONG)puDaten[x] - lMittel )/2l;
			}
			if( lTemp < lMin ) {
				lMin = lTemp;
			}
			if( lTemp > lMax ) {
				lMax = lTemp;
			}
			puDaten[x] = (UWORD)lTemp;
		}
		puDaten += w;
	}

	// Minimum korrigieren
	BildMinMax( pBild, lMin, lMax, w, h );
	return ( TRUE );
}
/* 10.11.98 */


/* Einfaches Despiken */
/* bX: X-Richtung
         lSpikeX: Anzahl Punkte in X (0 = bel.)
         bY: Y-Richtung
         lSpikeY: Anzahl Punkte in Y (0 = bel.)
         bLowLim: Unten abschneiden
         uLowLim: Untere Grenze
         bUpLim: Oben Abschneiden
         uUpLim: Obere Grenze
         bInterpolate: Zwischenwerte interpolieren
 */
BOOLEAN	BildDespike( LPBILD pBild, LONG w, LONG h, LONG lSpikeX, LONG lSpikeY, UWORD uLowLim, UWORD uUpLim,
                     BOOLEAN bX, BOOLEAN bY, BOOLEAN bLowLim, BOOLEAN bUpLim, BOOLEAN bInterpolate )
{
	LONG x, y, i, lDelta, lBreite;
	LPUWORD	puDaten = pBild->puDaten;

	ASSERT(  w*h > 0   &&  pBild != NULL  &&  (LONG)pBild->puDaten > 256 );

	// "Despiken" in x-Rtg. ... (bis lSpikeY Pixel Differenz)
	if( bX ) {
		for( y = 0;  y < w*h;  y += w )	{
			for( x = y+1;  x < y+w;  x++ ) {

				// Zuerst unteres Limit beachten
				if( ( bLowLim  &&  puDaten[x] <= uLowLim )  &&  puDaten[x-1] > uLowLim ) {
					// lSpikeY fehlende Punkte ergänzen ...
					for( i = x+1;  i < y+w  &&  ( lSpikeX == 0  ||  i < x+lSpikeX )  &&  puDaten[i] <= uLowLim;  i++ ) {
						;
					}
					if( i < y+w  &&  ( lSpikeX == 0  ||  i < x+lSpikeX ) ) {
						lBreite = i-x+1;
						lDelta = (long)puDaten[i]-(long)puDaten[x-1];
						while( --i >= x ) {
							if( bInterpolate ) {
								// Linear Interpolieren
								puDaten[i] = (WORD)( puDaten[x-1] + ( lDelta*( i-x+1 ) )/lBreite );
							}
							else {
								puDaten[i] = (WORD)( puDaten[x-1] + lDelta/2 );
							}
						}
					}
				}

				// Dann oberes Limit beachten
				if( ( bUpLim  &&  puDaten[x] >= uUpLim )  &&  puDaten[x-1] < uUpLim ) {
					// Maximal 3 fehlenden Punkte ergänzen ...
					for( i = x+1;  i < y+w  &&  ( lSpikeX == 0  ||  i < x+lSpikeX )  &&  puDaten[i] >= uUpLim;  i++ ) {
						;
					}
					if( i < y+w  &&  ( lSpikeX == 0  ||  i < x+lSpikeX ) ) {
						lBreite = i-x+1;
						lDelta = (long)puDaten[i]-(long)puDaten[x-1];
						while( --i >= x ) {
							if( bInterpolate ) {
								// Linear Interpolieren
								puDaten[i] = (WORD)( puDaten[x-1] + ( lDelta*( i-x+1 ) )/lBreite );
							}
							else {
								puDaten[i] = (WORD)( puDaten[x-1] + lDelta/2 );
							}
						}
					}
				}
			} // for x
		} // for y
	}

	// ... und "Despiken" y-Rtg.
	if( bY ) {
		for( x = 0;  x < w;  x++ ) {
			for( y = w;  y < w*h;  y += w )	{

				// Zuerst unteres Limit beachten
				if( ( bLowLim  &&  puDaten[x+y] <= uLowLim )  &&  puDaten[x+y-w] > uLowLim ) {
					// lSpikeY fehlende Punkte ergänzen ...
					for( i = y+w;  i < w*h  &&  ( lSpikeY == 0  ||  i < y+lSpikeY*w )  &&  puDaten[x+i] <= uLowLim;  i += w ) {
						;
					}
					if( i < w*h  &&  ( lSpikeY == 0  ||  i < y+lSpikeY*w ) ) {
						lBreite = ( i-y )/w+1;
						lDelta = (long)puDaten[x+i]-(long)puDaten[x+y-w];
						for( i -= w;  i >= y;  i -= w )	{
							if( bInterpolate ) {
								// Linear Interpolieren
								puDaten[x+i] = (WORD)( puDaten[x+y-w] + ( lDelta*( ( i-y )/w+1 ) )/lBreite );
							}
							else {
								puDaten[x+i] = (WORD)( puDaten[x+y-w] + lDelta/2 );
							}
						}
					}
				}

				// Dann oberes Limit beachten
				if( ( bUpLim  &&  puDaten[x+y] >= uUpLim )  &&  puDaten[x+y-w] < uUpLim ) {
					// lSpikeY fehlende Punkte ergänzen ...
					for( i = y+w;  i < w*h  &&  ( lSpikeY == 0  ||  i < y+lSpikeY*w )  &&  puDaten[x+i] >= uUpLim;  i += w ) {
						;
					}
					if( i < w*h  &&  ( lSpikeY == 0  ||  i < y+lSpikeY*w ) ) {
						lBreite = ( i-y )/w+1;
						lDelta = (long)puDaten[x+i]-(long)puDaten[x+y-w];
						for( i -= w;  i >= y;  i -= w )	{
							if( bInterpolate ) {
								// Linear Interpolieren
								puDaten[x+i] = (WORD)( puDaten[x+y-w] + ( lDelta*( ( i-y )/w+1 ) )/lBreite );
							}
							else {
								puDaten[x+i] = (WORD)( puDaten[x+y-w] + lDelta/2 );
							}
						}
					}
				}
			} // for x
		} // for y
	}

	return ( bX||bY );
}
// 3.1.99


/**************************************************************************
 *	Medianmittelung ab hier ...
 *
 *	Geeignet um Rauschen und Spikes zu entfernen, aber recht aufwendig
 *	Es wird eine geeignete ungerade Anzahl von Punkten um den zu testenden
 *	Punkte bestimmt. Diese werden sortiert und der Punkt wird durch den
 *	mittleren Wert ersetzt.
 *	So erfindet die Medianmittelung auch keine neuen Werte dazu!
 *
 *	(Bei 25 und mehr Punkten wäre aber ein besserer Suchalgorithmus als nun
 *	 ausgerechnet ein Bubblesort angesagt ...)
 *
 *	Damit SnomPutz danach nicht absemmelt, muss allerdings der Rand durch
 *	Wiederholung gleicher Werte oben und unten dazugeschummelt werden ...
 *	(Bild darf nicht schrumpfen!)
 */

// Hilfsfunktion für Median
void swap( LPUWORD a, LPUWORD b )
{
	UWORD temp = ( *a );
	*a = *b;
	*b = temp;
}


// Bestimmt den mittleren Wert
UWORD Median( LPUWORD puWerte, int iAnzahl )
{
	BOOLEAN	bNoChange;
	int i, j;

	// Für drei Punkte einfach per if/else
	if( iAnzahl == 3 ) {
		if( puWerte[0] > puWerte[2] ) {
			if( puWerte[0] > puWerte[1] ) {
				// 0>2 && 0>1
				if( puWerte[2] > puWerte[1] ) {
					// 0>2>1
					return ( puWerte[2] );
				}
				else {
					// 0>1>2
					return ( puWerte[1] );
				}
			}
			else {
				// 1>0>2
				return ( puWerte[0] );
			}
		}
		else {
			if( puWerte[0] < puWerte[1] ) {
				// 0<=2 && 0<=1
				if( puWerte[2] <= puWerte[1] ) {
					// 0<=2<=1
					return ( puWerte[2] );
				}
				else {
					// 0<=1<=2
					return ( puWerte[1] );
				}
			}
			else {
				// 1<=0<=2
				return ( puWerte[0] );
			}
		}
	}
	// Für 3 Pkte.

	// Für x Punkte ein einfacher Vertauschungssort
	// in Fachkreisen auch Arrg! Bubblesort genannt!
	for( j = 1;  j < iAnzahl/2+2;  j++ ) {
		bNoChange = TRUE;
		for( i = 0;  i < iAnzahl-j;  i++ ) {
			if( puWerte[i] < puWerte[i+1] )	{
				swap( puWerte+i, puWerte+i+1 );
				bNoChange = FALSE;
			}
		}
		// Nicht vertauscht? Dann fertig!
		if( bNoChange ) {
			break;
		}
	}
	return ( puWerte[iAnzahl/2] );
}


// Berechnet spaltengemittelte Medianmittelung
void BildMedianSpalten( LPUWORD puDaten, LONG w, LONG h, WORD iAnzahl )
{
	LPUWORD	puSrc;
	long x, y, i;
	WORD puWerte[9];                // Maximal über 9 Punkte

	puSrc = puDaten;
	h -= iAnzahl;
	for( y = 0;  y <= h;  y++ ) {
		for( x = 0;  x < w;  x++ ) {
			for( i = 0;	 i < iAnzahl;  i++ ) {
				puWerte[i] = puDaten[i*w];
			}
			// Da die Werte in der nullten Zeile eh nicht mehr verwertet werden,
			// werden sie gleich zurückgeschrieben
			*puDaten++ = Median( puWerte, iAnzahl );
		}
	}

	// Achtung: Die resultierende Bitmap ist natürlich um "iAnzahl" Zeile niedriger.
	puDaten = puSrc;
	for( i = h+iAnzahl-1;  i > 0;  i-- ) {
		if( i > h+iAnzahl/2l ) {
			// letzte Zeile wiederholen
			MemMove( puDaten+i*w, puSrc+h*w, w*sizeof( WORD ) );
		}
		else if( i < iAnzahl/2l ) {
			// erste Zeile wiederholen
			MemMove( puDaten+i*w, puSrc, w*sizeof( WORD ) );
		}
		else {
			// Daten nach unten setzen
			MemMove( puDaten+i*w, puSrc+( i-iAnzahl/2l )*w, w*sizeof( WORD ) );
		}
	}
}
// 20.9.98


// Berechnet Medianmittelung
BOOLEAN	BildMedian( LPUWORD puDaten, LONG w, LONG h, WORD iAnzahl )
{
	LPUWORD	puZeile;
	WORD puWerte[25];               // maximal 5x5
	LONG x, y, i, j;

	switch( iAnzahl ) {
		case 1: // l-förmige Verteilung
			puZeile = puDaten;
			for( y = 0;  y < h-1;  y++ ) {
				for( x = 0;  x < w-1;  x++ ) {
					puWerte[0] = puDaten[0];
					puWerte[1] = puDaten[1];
					puWerte[2] = puDaten[w];
					*puZeile++ = Median( puWerte, 3 );
					puDaten++;
				}
				*puZeile++ = *puDaten++;        // Letzten Wert doppeln und überspringen
			}
			// Letzte Zeile doppeln
			MemMove( puDaten, puDaten-w, w*sizeof( WORD ) );
			return ( TRUE );

		case 3:
			// Zwischenspeicher für eine Zeile beschaffen
			if( ( puZeile = pMalloc( w*sizeof( WORD ) ) ) == NULL )	{
				FehlerRsc( E_MEMORY );
				return ( FALSE );
			}

			for( y = 0;  y < h-2;  y++ ) {
				for( x = 0;  x < w-2;  x++ ) {
					for( i = 0;  i < 3;  i++ ) {
						for( j = 0;  j < 3;  j++ ) {
							puWerte[i*3+j] = puDaten[x+y*w+i*w+j];
						}
					}
					puZeile[x+1] = Median( puWerte, 9 );
				}
				// Erstes und letztes Element auffüllen
				puZeile[0] = puZeile[1];
				puZeile[w-1] = puZeile[w-2];
				// Zeile kopieren
				MemMove( puDaten+y*w, puZeile, sizeof( WORD )*w );
			}
			MemFree( puZeile );
			break;

		case 5:
			// Zwischenspeicher für eine Zeile beschaffen
			if( ( puZeile = pMalloc( w*sizeof( WORD ) ) ) == NULL )	{
				FehlerRsc( E_MEMORY );
				return ( FALSE );
			}

			for( y = 0;  y < h-4;  y++ ) {
				for( x = 0;  x < w-4;  x++ ) {
					for( i = 0;  i < 5;  i++ ) {
						for( j = 0;  j < 5;  j++ ) {
							puWerte[i*5+j] = puDaten[x+y*w+i*w+j];
						}
					}
					puZeile[x+2] = Median( puWerte, 25 );
				}
				puZeile[0] = puZeile[1] = puZeile[2];
				puZeile[w-1] = puZeile[w-2] = puZeile[w-3];
				MemMove( puDaten+y*w, puZeile, sizeof( WORD )*w );
			}
			MemFree( puZeile );
			break;
	}

	// Achtung: Die resultierende Bitmap ist natürlich um "iAnzahl" Zeile niedriger.
	for( i = h-1;  i > 0;  i-- ) {
		if( i > h-iAnzahl/2l ) {
			// letzte Zeile wiederholen
			MemMove( puDaten+i*w, puDaten+( h-iAnzahl-1 )*w, w*sizeof( WORD ) );
		}
		else if( i < iAnzahl/2l ) {
			// erste Zeile wiederholen
			MemMove( puDaten+i*w, puDaten, w*sizeof( WORD ) );
		}
		else {
			// Daten nach unten setzen
			MemMove( puDaten+i*w, puDaten+( i-iAnzahl/2 )*w, w*sizeof( WORD ) );
		}
	}

	return ( TRUE );
}
// 14.10.98


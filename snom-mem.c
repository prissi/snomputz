/**************************************************************************************
****	Konvertierung-, Speicher- und verwandte Routinen
**************************************************************************************/

#include <stdlib.h>     //für _rotr() und _rotl()

#include "snomputz.h"
#include "snom-typ.h"
#include "snom-win.h"
#include "snom-mem.h"

#define _USE_LIB

/********************************************************************************************/

// Aus Motorola mach Intel und zurück (Langwort)
LONG LongBig2Little( LONG i )
{
	LONG j = i;
	UCHAR *pc = (UCHAR*)&j;

	return ( ( (ULONG)pc[0]<<24 )|( (ULONG)pc[1]<<16 )|( (ULONG)pc[2]<<8 )|(ULONG)pc[3] );
}


// Aus Motorola mach Intel und zurück (Wort)
UWORD Big2Little( UWORD i )
{
	UWORD j = i;
	UCHAR *pc = (UCHAR*)&j;

	return ( ( (UWORD)pc[0]<<8 )|(UWORD)pc[1] );
}


// Macht aus einer Tabelle einen Texttabelle;
// Trennzeichen ist Tabulator
// Rückgabewert ist die neue Länge (negativ, wenn zu klein!)
LONG CopyFloat2Text( LPFLOAT( pfWerte[] ), LONG lSpalten, LONG lZeilen, LPSTR strText, LONG lMaxLaenge )
{
	LONG lPos = 0, i, j;
	CHAR str[20];

	ASSERT( pfWerte != NULL  &&  strText != NULL  &&  lZeilen > 0  &&  lSpalten > 0  &&  lMaxLaenge > 0  );

	// Text vorbereiten
	for( i = 0;  i < lZeilen;  i++ ) {
		for( j = 0;  j < lSpalten;  j++ ) {
			gcvt( pfWerte[j][i], 10, str );
			strcpy( (LPSTR)( strText+lPos ), str );
			lPos += strlen( str );
			strText[lPos] = '\t';
			lPos++;
		}

		// Neue Zeile
		lstrcpy( (LPSTR)( strText+lPos-1 ), "\xD\xA" );
		lPos += 1;

		// Maximale Länge überschritten?
		if( lMaxLaenge < lPos+12 ) {
			return ( -lPos );
		}
	}
	return ( lPos );
}
// 10.10.97


/********************************************************************************************/
/* Elementare Speicherverwaltung: Unter DOS alles andere als trivial ... */

LPVOID pMalloc( ULONG ulLaenge )
{
	ASSERT(  ulLaenge > 0  );
#ifdef _USE_LIB
	return ( (LPVOID)calloc( ulLaenge, 1l ) );
#else
#ifndef BIT32
	return ( (LPVOID)GlobalLock( GlobalAlloc( GMEM_FIXED|GMEM_ZEROINIT, ulLaenge ) ) );
#else
	return ( (LPVOID)GlobalAlloc( GMEM_FIXED|GMEM_ZEROINIT, ulLaenge ) );
#endif
#endif
}


void MemFree( LPVOID pvPtr )
{
#ifdef _USE_LIB
	free( pvPtr );
#else
#ifndef BIT32
	HGLOBAL	h = GlobalPtrHandle( pvPtr );
	ASSERT(  pvPtr != NULL  );
	GlobalUnlock( h );
	GlobalFree( h );
#else
	ASSERT(  pvPtr != NULL  );
	GlobalFree( (HGLOBAL)pvPtr );
#endif
#endif
}


void MemMove( LPVOID pvTarget, LPVOID pvSrc, LONG lLen )
{
#ifndef	BIT32
	// Klimmzüge für 16 Bit
	if( (LPCHAR)pvTarget+lLen < (LPCHAR)pvSrc  ||  (LPCHAR)pvTarget > (LPCHAR)pvSrc+lLen ) {
		// Wenn nicht überlappend: Einfach
		hmemcpy( pvTarget, pvSrc, lLen );
	}
	else {
		// Ansonsten mit krepligen Bibliotheksroutinen ...
		void far* far _fmemmove( void far *dest, const void far *src, size_t n );


		LONG lOffset = 0;

		while( lOffset+32768l < lLen ) {
			_fmemmove( (char huge*)pvTarget+lOffset, (char huge*)pvSrc+lOffset, 32768ul );
			lOffset += 32768l;
		}
		if( lLen-lOffset > 0 ) {
			_fmemmove( (char huge*)pvTarget+lOffset, (char huge*)pvSrc+lOffset, lLen-lOffset );
		}
	}
#else
	void* memmove( void *dest, const void *src, size_t n );

	memmove( pvTarget, pvSrc, lLen );
#endif
}


void MemSet( LPVOID pvTarget, UCHAR cConst, LONG lLen )
{
#ifndef	BIT32
	// Klimmzüge für 16 Bit
	// Ansonsten mit krepligen Bibliotheksroutinen ...
	void far * far _fmemset( void far *dest, unsigned const, size_t n );


	LONG lOffset = 0;
	while( lOffset+32768l < lLen ) {
		_fmemset( (char huge*)pvTarget+lOffset, cConst, 32768ul );
		lOffset += 32768l;
	}
	if( lLen-lOffset > 0 ) {
		_fmemset( (char huge*)pvTarget+lOffset, cConst, lLen-lOffset );
	}
#else
	void* memset( void *dest, int i, size_t n );

	memset( pvTarget, cConst, lLen );
#endif
}


/********************************************************************************************/


/****	Gibt TRUE zurück, wenn diese Position maskiert ist ****/
/**** ACHTUNG: Keine Fehlerprüfung! ****/
BOOL IsMaske( LPBMPDATA pBmp, int x, int y )
{
	int w = pBmp->wMaskeW, i;

	if( pBmp->pMaske == NULL ) {
		return ( FALSE );
	}
	ASSERT(  w > 0  &&  x/8 < w  );
	i = pBmp->pMaske[y*w+( x/8 )];
	if( i == 0  ||  ( i&( 0x0080>>( x%8 ) ) ) == 0 ) {
		return ( FALSE );
	}
	return ( TRUE );
}
// 26.1.99


/****	Vergrössert Bild (ist hier, da extensiver Einsatz von Kopierroutinen) ****/
BOOL BildResize( LPBILD pBild, LONG w, LONG h, LONG NeuW, LONG NeuH )
{
	LPUWORD	puNeu;
	LONG x, y, old_x, old_y, i, delta;
	double fw = (double)w/(double)NeuW;
	double fh = (double)h/(double)NeuH;

	if( ( puNeu = (LPUWORD)pMalloc( NeuW*h*sizeof( WORD ) ) ) == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	// Vergrössern in x-Rtg
	if( fw < 1.0 ) {
		for( y = 0;  y < h;  y++ ) {
			x = old_x = 0;
			do {
				i = delta = (long)( ( old_x++ )/fw+0.5 ) - x;
				while( i > 0  &&  x < NeuW ) {
					if( old_x < w ) {
						puNeu[y*NeuW + x] = (UWORD)( ( pBild->puDaten[y*w+old_x]*( delta-i ) + pBild->puDaten[y*w+old_x-1]*i )/delta );
					}
					else {
						puNeu[y*NeuW + x] = pBild->puDaten[y*w+w-1];
					}
					i--;
					x++;
				}
			} while( x < NeuW );
		}
	}
	else {
		// Verkleinern in X-Rtg.
		for( y = 0;  y < h;  y++ ) {
			for( x = 0;  x < NeuW;  x++ ) {
				i = (long)( x*fw );
				if( i >= w ) {
					i = w-1;
				}
				puNeu[y*NeuW+x] = pBild->puDaten[y*w+i];
			}
		}
	}

	// Y-Rtg.
	MemFree( pBild->puDaten );
	pBild->puDaten = puNeu;
	if( ( puNeu = (LPUWORD)pMalloc( NeuW*NeuH*sizeof( WORD ) ) ) == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	// Vergroessern in Y-Rtg.
	if( fh < 1.0 ) {
		y = old_y = 0;
		do {
			i = delta = (long)( ( old_y++ )/fh+0.5 ) - y;
			while( i > 0  &&  y < NeuH ) {
				for( x = 0;  x < NeuW;  x++ ) {
					if( old_y < h ) {
						puNeu[y*NeuW + x] = (UWORD)( ( pBild->puDaten[( old_y-1 )*NeuW+x]*i + pBild->puDaten[old_y*NeuW+x]*( delta-i ) )/delta );
					}
					else {
						puNeu[y*NeuW + x] = pBild->puDaten[( h-1 )*NeuW+x];
					}
				}
				i--;
				y++;
			}
		} while( y < NeuH );
	}
	else {
		// Verkleinern in y-Rtg.
		for( y = 0;  y < NeuH;  y++ ) {
			i = (LONG)( y*fh );
			if( i >= h ) {
				i = h-1;
			}
			MemMove( puNeu+y*NeuW, pBild->puDaten+i*NeuW, NeuW*sizeof( WORD ) );
		}
	}

	MemFree( pBild->puDaten );
	pBild->puDaten = puNeu;
	return ( TRUE );
}
// 4.7.98


// Gibt die i-te Bitmap frei
BOOLEAN	FreeBmp( LPBMPDATA pBmp, WORD i )
{
/* Ungültiger Parameterbereich */
	ASSERT( ( i > 0  &&  pBmp->iMax > 1 )  &&  i < pBmp->iMax  );

	// Zuerst Daten freigeben
	if( (LONG)pBmp->pSnom[i].Topo.puDaten > 256 ) {
		MemFree( pBmp->pSnom[i].Topo.puDaten );
	}
	if( (LONG)pBmp->pSnom[i].Error.puDaten > 256 ) {
		MemFree( pBmp->pSnom[i].Error.puDaten );
	}
	if( (LONG)pBmp->pSnom[i].Lumi.puDaten > 256 ) {
		MemFree( pBmp->pSnom[i].Lumi.puDaten );
	}
	pBmp->iMax--;

	// Evt. die aktuelle Bitmap verändern
	if( pBmp->iAktuell >= i ) {
		pBmp->iAktuell--;
	}
	if( pBmp->iSaved == i ) {
		pBmp->iSaved = -1;
	}
	if( pBmp->iSaved > i ) {
		pBmp->iSaved++;
	}
	// Wenn aus der Mitte gelöscht, muss verschoben werden
	MemMove( &( pBmp->pSnom[i] ), &( pBmp->pSnom[i+1] ), sizeof( SNOMDATA )*(ULONG)( pBmp->iMax-i ) );

	// Dann müssen die Pointer angepasst werden
	while( i < pBmp->iMax )	{
		if( (LONG)pBmp->pSnom[i].Topo.puDaten <= 256ul  &&  pBmp->pSnom[i].Topo.puDaten != NULL ) {
			pBmp->pSnom[i].Topo.puDaten = (LPWORD)( (LONG)pBmp->pSnom[i].Topo.puDaten-1 );
		}
		if( (LONG)pBmp->pSnom[i].Error.puDaten <= 256ul  &&  (LONG)pBmp->pSnom[i].Error.puDaten > 256 ) {
			pBmp->pSnom[i].Error.puDaten = (LPWORD)( (LONG)pBmp->pSnom[i].Error.puDaten-1 );
		}
		if( (LONG)pBmp->pSnom[i].Lumi.puDaten <= 256ul  &&  pBmp->pSnom[i].Lumi.puDaten != NULL ) {
			pBmp->pSnom[i].Lumi.puDaten = (LPWORD)( (LONG)pBmp->pSnom[i].Lumi.puDaten-1 );
		}
		i++;
	}
	return ( TRUE );
}
// 6.8.97


/* Testet, ob und was überhaupt verändert werden soll
 * Sollte vor pAllocNewSnom aufgerufen werden!
 * Rückgabewert: Die tatsächlich zu ändernden Teile
 */
WORKMODE WhatToDo( LPBMPDATA pBmp, WORKMODE modus )
{
	WORKMODE Was = NONE;
	ASSERT( pBmp != NULL  &&  pBmp->iAktuell >= 0  &&  pBmp->iAktuell < pBmp->iMax  );

	if( modus&TOPO  &&  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten ) {
		Was = TOPO;
	}
	if( modus&ERRO  &&  pBmp->pSnom[pBmp->iAktuell].Error.puDaten ) {
		Was |= ERRO;
	}
	if( modus&LUMI  &&  pBmp->pSnom[pBmp->iAktuell].Lumi.puDaten ) {
		Was |= LUMI;
	}
	return ( Was );
}
// 28.10.98


/* Gibt Pointer auf Bild oder Null zurück */
LPBILD GetBildPointer( LPBMPDATA pBmp, WORKMODE modus )
{
	if( modus&TOPO ) {
		return ( &( pBmp->pSnom[pBmp->iAktuell].Topo ) );
	}
	if( modus&ERRO ) {
		return ( &( pBmp->pSnom[pBmp->iAktuell].Error ) );
	}
	if( modus&LUMI ) {
		return ( &( pBmp->pSnom[pBmp->iAktuell].Lumi ) );
	}
	return ( NULL );
}
// 26.1.99


/* Gibt Pointer auf Daten oder Null zurück */
LPUWORD	GetDataPointer( LPBMPDATA pBmp, WORKMODE modus )
{
	LPUWORD	pu = NULL;

	if( modus&TOPO ) {
		pu = pBmp->pSnom[pBmp->iAktuell].Topo.puDaten;
		if( 256 >= (long)pu ) {
			pu = pBmp->pSnom[(long)pu-1].Topo.puDaten;
		}
	}
	if( modus&ERRO ) {
		pu = pBmp->pSnom[pBmp->iAktuell].Error.puDaten;
		if( 256 >= (LONG)pu ) {
			pu = pBmp->pSnom[(long)pu-1].Error.puDaten;
		}
	}
	if( modus&LUMI ) {
		pu = pBmp->pSnom[pBmp->iAktuell].Lumi.puDaten;
		if( 256 >= (long)pu ) {
			pu = pBmp->pSnom[(long)pu-1].Lumi.puDaten;
		}
	}
	return ( pu );
}
// 11.11.98


/* Bereitet neue Topografie/Error/Luminzensdaten-Bitmap vor
 * Dabei ist zu beachten, dass Werte für den Pointer <= 256
 * Indizes für die Originaldaten sind!
 */
LPSNOMDATA pAllocNewSnom( LPBMPDATA pBmp, WORKMODE modus )
{
	LPUWORD puSrc, puAlloc;
	LONG lLaenge;
	WORD iAkt = pBmp->iAktuell, i;

	// Was ist überhaupt nötig?
	modus = WhatToDo( pBmp, modus );

	ASSERT( pBmp != NULL  &&  pBmp->iAktuell >= 0  &&  pBmp->iAktuell < pBmp->iMax  );

	// weitergehende Aktionen freigeben
	// (kein Redo danach, da dies letzte Aktion war)
	for( i = pBmp->iMax-1;  i > iAkt;  i-- ) {
		FreeBmp( pBmp, i );
	}

	ASSERT(  pBmp->iAktuell == iAkt  );

	// Falls UNDO-Puffer voll, die ersten Aktionen freigeben (kommt praktisch nie vor ...)
	if( iAkt >= MAX_SNOM-2 ) {
		FreeBmp( pBmp, 0 );
	}
	iAkt = pBmp->iMax-1;

	// Neu Bitmap mit alten Wert sinnvoll initialisieren
	// ACHTUNG: Pointer sind noch falsch!
	MemMove( &( pBmp->pSnom[iAkt+1] ), &( pBmp->pSnom[iAkt] ), sizeof( SNOMDATA ) );

	// Neuen Speicher anlegen, das gleiche wie oben
	lLaenge = pBmp->pSnom[iAkt].w*pBmp->pSnom[iAkt].h*sizeof( UWORD );
	if( pBmp->pSnom[iAkt].Topo.puDaten ) {
		if( modus&TOPO ) {
			while( ( puAlloc = (LPUWORD)pMalloc( lLaenge ) ) == NULL ) {
				StatusLineRsc( I_FREE_MEMORY );
				// Solange Speicher freigeben, bis es wieder klappt
				if( pBmp->iAktuell <= 1 ) {
					FehlerRsc( E_MEMORY );
					return ( NULL );
				}
				// Denn 0 enthält die Originaldaten und einen auf *JEDEN* Fall gültigen Pointer
				FreeBmp( pBmp, 1 );
			}
			iAkt = pBmp->iMax-1;
			pBmp->pSnom[iAkt+1].Topo.puDaten = puAlloc;
			puSrc = pBmp->pSnom[iAkt].Topo.puDaten;
			if( (LONG)puSrc <= 256 ) {
				puSrc = pBmp->pSnom[(LONG)puSrc-1].Topo.puDaten;
			}
			if( pBmp->pSnom[iAkt+1].Topo.puDaten != NULL  &&  puSrc != NULL ) {
				MemMove( pBmp->pSnom[iAkt+1].Topo.puDaten, puSrc, lLaenge );
			}
		}
		else {
			// Nur Pointer auf letze anlegen
			if( (LONG)pBmp->pSnom[iAkt].Topo.puDaten < 256 ) {
				pBmp->pSnom[iAkt+1].Topo.puDaten = pBmp->pSnom[iAkt].Topo.puDaten;
			}
			else {
				pBmp->pSnom[iAkt+1].Topo.puDaten = (LPWORD)( iAkt+1 );
			}
		}
	}

	// Neuen Pointer für Error-Signal anlegen
	// analog wie Topografie
	if( pBmp->pSnom[iAkt].Error.puDaten ) {
		if( modus&ERRO ) {
			while( ( puAlloc = (LPUWORD)pMalloc( lLaenge ) ) == NULL ) {
				StatusLineRsc( I_FREE_MEMORY );
				if( pBmp->iAktuell <= 1 ) {
					FehlerRsc( E_MEMORY );
					return ( NULL );
				}
				FreeBmp( pBmp, 1 );
			}
			iAkt = pBmp->iMax-1;
			pBmp->pSnom[iAkt+1].Error.puDaten = puAlloc;
			puSrc = pBmp->pSnom[iAkt].Error.puDaten;
			if( (LONG)puSrc <= 256 ) {
				puSrc = pBmp->pSnom[(LONG)puSrc-1].Error.puDaten;
			}
			if( pBmp->pSnom[iAkt+1].Error.puDaten != NULL  &&  puSrc != NULL ) {
				MemMove( pBmp->pSnom[iAkt+1].Error.puDaten, puSrc, lLaenge );
			}
		}
		else {
			// Nur Pointer auf letze anlegen
			if( (LONG)pBmp->pSnom[iAkt].Error.puDaten < 256 ) {
				pBmp->pSnom[iAkt+1].Error.puDaten = pBmp->pSnom[iAkt].Error.puDaten;
			}
			else {
				pBmp->pSnom[iAkt+1].Error.puDaten = (LPWORD)( iAkt+1 );
			}
		}
	}

	// Neuen Speicher anlegen
	// Ein Wert von kleiner als 256 heißt, dass die Daten in Record mit dieser Nummer stehen
	if( pBmp->pSnom[iAkt].Lumi.puDaten ) {
		if( modus&LUMI ) {
			while( ( puAlloc = (LPUWORD)pMalloc( lLaenge ) ) == NULL ) {
				StatusLineRsc( I_FREE_MEMORY );
				if( pBmp->iAktuell <= 1 ) {
					FehlerRsc( E_MEMORY );
					return ( NULL );
				}
				FreeBmp( pBmp, 1 );
			}
			iAkt = pBmp->iMax-1;
			pBmp->pSnom[iAkt+1].Lumi.puDaten = puAlloc;
			puSrc = pBmp->pSnom[iAkt].Lumi.puDaten;
			if( (LONG)puSrc <= 256 ) {
				puSrc = pBmp->pSnom[(LONG)puSrc-1].Lumi.puDaten;
			}
			if( pBmp->pSnom[iAkt+1].Lumi.puDaten != NULL  &&  puSrc != NULL ) {
				MemMove( pBmp->pSnom[iAkt+1].Lumi.puDaten, puSrc, lLaenge );
			}
		}
		else {
			// Nur Pointer auf letze anlegen
			if( (LONG)pBmp->pSnom[iAkt].Lumi.puDaten < 256 ) {
				pBmp->pSnom[iAkt+1].Lumi.puDaten = pBmp->pSnom[iAkt].Lumi.puDaten;
			}
			else {
				pBmp->pSnom[iAkt+1].Lumi.puDaten = (LPWORD)( iAkt+1 );
			}
		}
	}

	ASSERT(	!( ( modus&TOPO && pBmp->pSnom[iAkt+1].Topo.puDaten == NULL )  &&
	           ( modus&ERRO && pBmp->pSnom[iAkt+1].Error.puDaten == NULL )  &&
	           ( modus&LUMI && pBmp->pSnom[iAkt+1].Lumi.puDaten == NULL ) ) );

	iAkt++;
	pBmp->iAktuell = iAkt;
	pBmp->iMax = iAkt+1;
	return ( &( pBmp->pSnom[iAkt] ) );
}
// 28.7.97



/**************************************************************************************
****	SNOM-MEM:	Konvertierung-, Speicher- und verwandte Routinen
**************************************************************************************/

// Aus Motorola mach Intel und zurück (Langwort)
LONG LongBig2Little( LONG i );

// Aus Motorola mach Intel und zurück (Wort)
UWORD Big2Little( UWORD i );


/* Macht aus einer Tabelle einen Texttabelle
 * Trennzeichen ist Tabulator
 * Rückgabewert ist die neue Länge (negativ, wenn zu klein!)
 */
LONG	CopyFloat2Text( LPFLOAT pfWerte[], LONG lSpalten, LONG lZeilen, LPSTR strText, LONG lMaxLaenge );
// 10.10.97

/* Elementare Speicherverwaltung: Unter DOS alles andere als trivial ... */
LPVOID	pMalloc( ULONG	ulLaenge );
void	MemFree( LPVOID pvPtr );
void	MemMove( LPVOID pvTarget, LPVOID pvSrc, LONG lLen );
void	MemSet( LPVOID pvTarget, UCHAR cConst, LONG lLen );

#ifndef	BIT32
// Routinen für 16-Bit kompatibilität: Windows-Routinen
#define strcpy( i, j ) lstrcpy( (i), (j) )
#define strlen( i ) lstrlen( (i) )
#define strcat( i, j ) lstrcat( (i), (j) )
#endif


/****	Gibt TRUE zurück, wenn diese Position maskiert ist ****/
/**** ACHTUNG: Keine Fehlerprüfung! ****/
BOOL	IsMaske( LPBMPDATA pBmp, int x, int y );
// 26.1.99


/****	Vergrössert Bild (ist hier, da extensiver Einsatz von Kopierroutinen) ****/
BOOL	BildResize( LPBILD pBild, LONG w, LONG h, LONG NeuW, LONG NeuH );
// 4.7.98


// Gibt die i-te Bitmap frei
BOOLEAN	FreeBmp( LPBMPDATA pBmp, WORD i );
// 6.8.97


/* Gibt Pointer auf Daten oder Null zurück */
LPUWORD	GetDataPointer( LPBMPDATA pBmp, WORKMODE modus );
// 11.11.98


/* Gibt Pointer auf Bild oder Null zurück */
LPBILD	GetBildPointer( LPBMPDATA pBmp, WORKMODE modus );
// 26.1.99



/* Testet, ob und was überhaupt verändert werden soll
 * Sollte vor pAllocNewSnom aufgerufen werden!
 * Rückgabewert: Die tatsächlich zu ändernden Teile
 */
WORKMODE	WhatToDo( LPBMPDATA pBmp, WORKMODE modus );
// 28.10.98



/* Bereitet neue Topografie/Error/Luminzensdaten-Bitmap vor
 * Dabei ist zu beachten, dass Werte für den Pointer <= 256
 * Indizes für die Originaldaten sind!
 */
LPSNOMDATA pAllocNewSnom( LPBMPDATA pBmp, WORKMODE modus );
// 28.7.97




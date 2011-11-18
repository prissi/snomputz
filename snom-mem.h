/**************************************************************************************
****	SNOM-MEM:	Konvertierung-, Speicher- und verwandte Routinen
**************************************************************************************/

// Aus Motorola mach Intel und zur�ck (Langwort)
LONG LongBig2Little( LONG i );

// Aus Motorola mach Intel und zur�ck (Wort)
UWORD Big2Little( UWORD i );


/* Macht aus einer Tabelle einen Texttabelle
 * Trennzeichen ist Tabulator
 * R�ckgabewert ist die neue L�nge (negativ, wenn zu klein!)
 */
LONG	CopyFloat2Text( LPFLOAT pfWerte[], LONG lSpalten, LONG lZeilen, LPSTR strText, LONG lMaxLaenge );
// 10.10.97

/* Elementare Speicherverwaltung: Unter DOS alles andere als trivial ... */
LPVOID	pMalloc( ULONG	ulLaenge );
void	MemFree( LPVOID pvPtr );
void	MemMove( LPVOID pvTarget, LPVOID pvSrc, LONG lLen );
void	MemSet( LPVOID pvTarget, UCHAR cConst, LONG lLen );

#ifndef	BIT32
// Routinen f�r 16-Bit kompatibilit�t: Windows-Routinen
#define strcpy( i, j ) lstrcpy( (i), (j) )
#define strlen( i ) lstrlen( (i) )
#define strcat( i, j ) lstrcat( (i), (j) )
#endif


/****	Gibt TRUE zur�ck, wenn diese Position maskiert ist ****/
/**** ACHTUNG: Keine Fehlerpr�fung! ****/
BOOL	IsMaske( LPBMPDATA pBmp, int x, int y );
// 26.1.99


/****	Vergr�ssert Bild (ist hier, da extensiver Einsatz von Kopierroutinen) ****/
BOOL	BildResize( LPBILD pBild, LONG w, LONG h, LONG NeuW, LONG NeuH );
// 4.7.98


// Gibt die i-te Bitmap frei
BOOLEAN	FreeBmp( LPBMPDATA pBmp, WORD i );
// 6.8.97


/* Gibt Pointer auf Daten oder Null zur�ck */
LPUWORD	GetDataPointer( LPBMPDATA pBmp, WORKMODE modus );
// 11.11.98


/* Gibt Pointer auf Bild oder Null zur�ck */
LPBILD	GetBildPointer( LPBMPDATA pBmp, WORKMODE modus );
// 26.1.99



/* Testet, ob und was �berhaupt ver�ndert werden soll
 * Sollte vor pAllocNewSnom aufgerufen werden!
 * R�ckgabewert: Die tats�chlich zu �ndernden Teile
 */
WORKMODE	WhatToDo( LPBMPDATA pBmp, WORKMODE modus );
// 28.10.98



/* Bereitet neue Topografie/Error/Luminzensdaten-Bitmap vor
 * Dabei ist zu beachten, dass Werte f�r den Pointer <= 256
 * Indizes f�r die Originaldaten sind!
 */
LPSNOMDATA pAllocNewSnom( LPBMPDATA pBmp, WORKMODE modus );
// 28.7.97




/*************************************
   Hier passiert die Arbeit in SNOM-Putz
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wingdi.h>
#include <math.h>

#include "myportab.h"
#include "snom-typ.h"
#include "snomputz.h"
#include "snom-var.h"
#include "snomlang.h"

#include "snom-dsp.h"
#include "snom-wrk.h"
#include "snom-mem.h"
#include "snom-mat.h"
#include "snom-win.h"


/***************************************************************************/
/* Ein paar Hilffunktionen zur verwaltung von Bitmaps */


BOOL PointInRect( CONST RECT *lprc /* address of structure with rectangle	*/, POINT pt /* structure with point	*/)
{
	return ( pt.x >= lprc->left  &&  pt.x <= lprc->right  &&  pt.y >= lprc->top  &&  pt.y <= lprc->bottom );
}


// Funktionen zum Ermitteln der Daten aus DIB-Speicherblocks
#define IsCore( lpDib ) ( ( ( (LPBITMAPINFOHEADER)lpDib )->biSize ) == sizeof( BITMAPCOREHEADER ) )
#define AlignDWord( i ) ( ( (DWORD)( i )+3ul )&~3ul )

// Geht schief bei OS/2-BMPs!
#define	GetDibWidth( i ) ( ( (LPBITMAPINFOHEADER)i )->biWidth )
#define	GetDibHeight( i ) ( ( (LPBITMAPINFOHEADER)i )->biHeight )

// Wieviel Farben sind in der Farbepalette?
LONG GetDibNumColors( LPBITMAPINFO lpDib )
{
	if( IsCore( lpDib ) ) {
		return ( 1l<<( ( (LPBITMAPCOREHEADER) lpDib )->bcBitCount ) );
	}
	else {
		if( lpDib->bmiHeader.biClrUsed > 0 ) {
			return ( (WORD)lpDib->bmiHeader.biClrUsed );
		}
		return ( 1l<<lpDib->bmiHeader.biBitCount );
	}
}


// Ermittelt Offset der Daten vom Start
LONG GetDibBitsAddr( LPBITMAPINFO lpDib )
{
	DWORD dwNumColors;

	dwNumColors = GetDibNumColors( lpDib );
	if( IsCore( lpDib ) ) {
		return ( sizeof( BITMAPCOREHEADER )+dwNumColors*sizeof( RGBTRIPLE ) );
	}
	else {
		return ( sizeof( BITMAPINFOHEADER )+dwNumColors*sizeof( RGBQUAD ) );
	}
}


// Kopiert die Farbewerte aus der Palette
WORD GetDibPalette( LPLOGPALETTE palette, LPBITMAPINFO lpDib )
{
	WORD col;
	RGBTRIPLE HUGE *pTriple;
	RGBQUAD HUGE *pQuad;
	DWORD dwNumColors;

	dwNumColors = GetDibNumColors( lpDib );
	if( IsCore( lpDib ) ) {
		// OS2 zuerst
		if( dwNumColors <= 256 ) {
			pTriple = (RGBTRIPLE HUGE*)( (LPBYTE)lpDib + ( (LPBITMAPCOREHEADER)lpDib )->bcSize );
			palette->palVersion = 0x300;
			palette->palNumEntries = (WORD)dwNumColors;
			for( col = 0;  col < dwNumColors;  col++ ) {
				//	So will es das Lehrbuch
				palette->palPalEntry[col].peBlue = pTriple[col].rgbtBlue;
				palette->palPalEntry[col].peGreen = pTriple[col].rgbtGreen;
				palette->palPalEntry[col].peRed = pTriple[col].rgbtRed;
				palette->palPalEntry[col].peFlags = 0;
			}
		}
	}
	else {
		// Windows Dib
		if( dwNumColors <= 256 ) {
			pQuad = (RGBQUAD HUGE*)( (LPBYTE)lpDib + sizeof( BITMAPINFOHEADER ) );
			palette->palVersion = 0x300;
			if( ( (LPBITMAPINFOHEADER) lpDib )->biClrImportant > 0 ) {
				palette->palNumEntries = (WORD)( ( (LPBITMAPINFOHEADER) lpDib )->biClrImportant );
			}
			else {
				palette->palNumEntries = (WORD)dwNumColors;
			}
			for( col = 0;  col < dwNumColors;  col++ ) {
				//	So will es da Lehrbuch
				palette->palPalEntry[col].peBlue = (BYTE)pQuad[col].rgbBlue;
				palette->palPalEntry[col].peGreen = (BYTE)pQuad[col].rgbGreen;
				palette->palPalEntry[col].peRed = (BYTE)pQuad[col].rgbRed;
				palette->palPalEntry[col].peFlags = 0;
			}
		}
	}
	return ( TRUE );
}


//**** Erzeugt neue Farbpalette, inklusive 2-Farbeverlauf und "Totfarben" ****
BOOL SetDibPaletteColors( LPBITMAPINFO pDib, COLORREF HUGE *Farbe, LPBILD pBild, LONG StartFarbe, LONG EndFarbe, WORD ClipStart, WORD ClipEnde )
{
	LONG sRed, eRed, sGreen, eGreen, sBlue, eBlue;
	LONG i, j;
	LONG ende, maxuse;

	ASSERT(  StartFarbe < 256  &&  ( EndFarbe < 256  ||  ClipEnde < 256 ) );
	//**** Zuerst den Anfang in der Startfarbe ...
	sRed = GetRValue( Farbe[0] );
	sGreen = GetGValue( Farbe[0] );
	sBlue = GetBValue( Farbe[0] );

	maxuse = ende = EndFarbe - StartFarbe;
	j = StartFarbe;

	// nothing => use only two colors
	if( pBild != NULL ) {
		if( (LONG)pBild > 1  &&  !pBild->bNoLUT  &&  pBild->iNumColors > 0 ) {
			// Using Vaules from LUT
			for( j = StartFarbe;  j <= EndFarbe;  j++ ) {
				if( j < ClipStart  ||  j >= ClipEnde ) {
					continue;
				}
				i = ( ( j-StartFarbe )*pBild->iNumColors )/maxuse;
				pDib->bmiColors[j].rgbRed		= GetRValue( pBild->LUT[i] );
				pDib->bmiColors[j].rgbGreen = GetGValue( pBild->LUT[i] );
				pDib->bmiColors[j].rgbBlue	= GetBValue( pBild->LUT[i] );
			}
			return ( TRUE );
		}
		else {
			// Normal 3 Color change
			eRed = GetRValue( Farbe[1] );
			eGreen = GetGValue( Farbe[1] );
			eBlue = GetBValue( Farbe[1] );
			ende /= 2;
			for( i = 0;  i < ende;  i++, j++ ) {
				if( j >= ClipStart  &&  j <= ClipEnde )	{
					pDib->bmiColors[j].rgbRed = (BYTE)( ( ( ende-i )*sRed+i*eRed )/ende );
					pDib->bmiColors[j].rgbGreen = (BYTE)( ( ( ende-i )*sGreen+i*eGreen )/ende );
					pDib->bmiColors[j].rgbBlue = (BYTE)( ( ( ende-i )*sBlue+i*eBlue )/ende );
				}
			}
			sRed = eRed;
			sGreen = eGreen;
			sBlue = eBlue;
			ende = maxuse-ende;
		}
	}

	// und der naechste Farbverlauf
	ende = max(1,maxuse-( j-StartFarbe ));
	eRed = GetRValue( Farbe[2] );
	eGreen = GetGValue( Farbe[2] );
	eBlue = GetBValue( Farbe[2] );
	for( i = 0;  i <= ende;  i++, j++ ) {
		if( j >= ClipStart  &&  j <= ClipEnde )	{
			pDib->bmiColors[j].rgbRed = (BYTE)( ( ( ende-i )*sRed+i*eRed )/ende );
			pDib->bmiColors[j].rgbGreen = (BYTE)( ( ( ende-i )*sGreen+i*eGreen )/ende );
			pDib->bmiColors[j].rgbBlue = (BYTE)( ( ( ende-i )*sBlue+i*eBlue )/ende );
		}
	}
	return ( TRUE );
}
// 26.5.97


// Schreibt Bitmap (lpDib ist Globales Handle auf Bitmap!)
BOOLEAN	WriteDib( LPSTR szFileName, LPBITMAPINFO lpDib, LPBYTE lpDaten )
{
	OFSTRUCT of;
	BITMAPFILEHEADER bmfh ;
	DWORD dwHeaderSize, dwW;
	LONG lDibSize;
	int hFile ;

	if( HFILE_ERROR == ( hFile = OpenFile( szFileName, &of, OF_WRITE | OF_CREATE ) ) ) {
		return ( FALSE );
	}

	if( IsCore( lpDib ) ) {
		dwW = ( ( (LPBITMAPCOREHEADER)lpDib )->bcBitCount*(DWORD)( (LPBITMAPCOREHEADER)lpDib )->bcWidth+7ul )/8ul;
		lDibSize = AlignDWord( dwW ) * (DWORD)( (LPBITMAPCOREHEADER)lpDib )->bcHeight;
		dwHeaderSize = sizeof( BITMAPCOREHEADER ) + ( sizeof( RGBTRIPLE )<<( (LPBITMAPCOREHEADER)lpDib )->bcBitCount );
	}
	else {
		dwW = ( lpDib->bmiHeader.biBitCount*lpDib->bmiHeader.biWidth+7ul )/8ul;
		lDibSize = AlignDWord( dwW ) * lpDib->bmiHeader.biHeight;
		dwHeaderSize = sizeof( BITMAPINFOHEADER ) + sizeof( RGBQUAD )*GetDibNumColors( lpDib );
	}

	// Fileheader schreiben
	bmfh.bfType = *(LPUWORD)"BM";
	bmfh.bfSize = AlignDWord( lDibSize+dwHeaderSize );
	bmfh.bfOffBits = dwHeaderSize + sizeof( BITMAPFILEHEADER );
	_hwrite( hFile, (LPSTR) &bmfh, sizeof( BITMAPFILEHEADER ) );

	// Dib-Header schreiben
	_hwrite( hFile, (LPSTR)lpDib, dwHeaderSize );

#if 0
	{
		// Run length encoded
		LONG x = 0, y = 0, w = lpDib->bmiHeader.biWidth;
		h = lpDib->bmiHeader.biHeight;

		for( y = 0;  y < h;  y++ ) {
			for( x = 0;  x < w;  x++ ) {
				if( lpDaten[x] == 0 ) {
					pPtr;
				}
			}
		}
	}
#else
	if( _hwrite( hFile, (LPSTR)lpDaten, lDibSize ) != lDibSize ) {
		_lclose( hFile );
		FehlerRsc( E_HARDDISK );
		return ( FALSE );
	}
#endif

	_lclose( hFile ) ;
	return ( TRUE );
}


/***************************************************************************/
/* Ab hier nur Neuzeichen ... */

//**** Callback fï¿½r das Neuzeichnen der Fenster
BOOL CALLBACK EnumRedrawAll( HWND hwnd, LPARAM flags )
{
	LPBMPDATA pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0l );
	if( flags&CLEAR3D ) {
		if( !IsBadWritePtr( pBmp, sizeof( BMPDATA ) ) ) {
			RecalcCache( pBmp, TRUE, TRUE );
		}
	}
	if( flags&SLIDER ) {
		RECT ClientRect;

		if( pBmp->bIsScanLine )	{
			pBmp->bPlotUnten = PlotsUnten;
			// Platz fï¿½r Scanline dazurechnen
			pBmp->rectFenster.right = 64+pBmp->rectPlot.right;      // Rechts Platz lassen
			if( pBmp->bPlotUnten ) {
				if( pBmp->rectLinks.top > 0 ) {
					pBmp->rectFenster.bottom = pBmp->rectPlot.bottom+pBmp->rectLinks.top;
				}
				else {
					pBmp->rectFenster.bottom = pBmp->rectPlot.bottom+pBmp->rectRechts.top;
				}
			}
		}
		GetClientRect( hwnd, &ClientRect );
		SendMessage( hwnd, WM_RECALC_SLIDER, 0, (LPARAM)( (DWORD)ClientRect.right | ( ( (DWORD)ClientRect.bottom )<<16ul ) ) );
	}
	if( flags&UPDATE ) {
		InvalidateRect( hwnd, NULL, FALSE );
	}
	return ( TRUE );
}
// 3.5.97


// Sendet an alle Childs ein Redraw
BOOL RedrawAll( WORD flags )
{
	BOOL b;

	StatusLineRsc( I_REDRAW );
	b = EnumChildWindows( hwndClient, EnumRedrawAll, flags  );
	ClearStatusLine();
	return ( b );
}
// 3.5.97

/******************** Ab hier: Zeichenfunktionen *************************************/

// Skalierung zeichnen
#define		SKALA	1

#define MAX_TABLE	4
#define	MIN_SCALE	1e-10   // Darunter waren es bestimmt sinnlose Eingabewerte
#define	MAX_SCALE	1e10    // oder auch hierï¿½ber
double table[MAX_TABLE] = { 1.0, 2.0, 2.5, 5.0 };

// Berechnet die "zweckmaessigste Einheit, d.h die nachste durch 1, 2, 2,5, 5 teilbare Zahl,
// die am ehesten zwischen min und max Anzahl von Strichen erzeugt ...
double CalcIncrement( int min, int max, double len )
{
	double prefactor = 1.0, signum = 1.0;
	unsigned t_index = 0;
	int significant = -1;
	BOOL dec = FALSE;

	if( len < 0.0 )	{
		signum = -1.0;
		len *= -1.0;
	}
	if( min > max ) {
		max = min;
	}
	if( len <= MIN_SCALE ) {
		return ( signum*MIN_SCALE );
	}
	if( len >= MAX_SCALE ) {
		return ( signum*MAX_SCALE );
	}
	while( 1 ) {
		// prefactor ist zu groï¿½!
		if( len/( prefactor*table[t_index] ) < min ) {
			dec = TRUE;
			t_index = ( t_index+3 ) % 4;
			if( t_index == 3 ) {
				prefactor /= 10.0;
			}
		}
		// prefactor ist zu klein
		// vorsicht, wenn vorher prefactor verkleinert wurde, kann es einen Endlosschleife geben;
		// deswegen Abbruch, wenn dec==TRUE!
		if( len/( prefactor*table[t_index] ) > max ) {
			if( dec ) {
				return ( prefactor*table[t_index] );
			}
			t_index++;
			if( t_index > 3 ) {
				prefactor *= 10.0;
				t_index = 0;
			}
		}
		else if( len/( prefactor*table[t_index] ) >= min ) {
			return ( prefactor*table[t_index]*signum ) ;
		}

		dec = FALSE;
	}
	// Endet nie hier!
}
// 18.8.97


// Berechnet die Anzahl signifikanter Stellen nach dem Komma fï¿½r Achsenskalierung
int CalcDigits( double inc )
{
	int digit = 0;

	if( inc < 0.0 ) {
		inc *= -1.0;
	}
	if( inc > 10.0 ) {
		return ( 0 );
	}
	while( inc < 1.0 ) {
		digit++;
		inc *= 10.0;
	}
	if( fabs( inc-2.5 ) < 0.1 ) {
		digit++;
	}
	return ( digit );
}
// 29.11.98


// Berechnet Ausmaï¿½e einer Bitmap mit Legende, 3D ...
void CalcDibSize( HDC hdc, LONG cxDib, LONG cyDib, LPRECT pCoord, BOOL Show3D, BOOL ShowScanline )
{
	LONG x = 0, y = 0, temp;

	// Ausmaï¿½e der 3D-Bitmap berechnen, ansonsten ist Hoehe und Breite cxDib/cyDib
	if( Show3D ) {
		temp = cxDib*( 1.0-f3DXYWinkel )+cyDib*f3DXYWinkel + 2;
		cyDib = ( cxDib*f3DXYWinkel+cyDib*( 1.0-f3DXYWinkel ) )*f3DZWinkel + MAX_3D_HOEHE*f3DZSkal*( 1.0-f3DZWinkel );
		cxDib = temp;
	}
	pCoord->left = (WORD)x;
	pCoord->top = (WORD)y;
	x += cxDib;
	y += cyDib;
	pCoord->right = (WORD)x;
	pCoord->bottom = (WORD)y;
	// eine evtuelle Skanlinie wird noch ignoriert
}
// 6.5.97


// Malt eine 3D Ansicht in den Hintergrund
//	pDest: Ziel (ab Position xoff,yoff) hat die Ausmasse ww, hh
//	pSrc: Quelle, dw Breit, dh Hoch
//	Farben kleiner min oder grï¿½ï¿½er max werden abgeschnitten
//	Ansonsten mit Faktor ColorFaktor multipliziert und StartColor aufaddiert ...
void Draw3D( LPUCHAR pBits, LONG xoff, LONG yoff, LONG ww, LONG hh,
             LPUWORD pSrc, LONG dw, LONG dh,
             WORD min, WORD max, float fColorFaktor, WORD StartColor, LPBILD pBild )
{
	// 3D-Bitmap erstellen
	long xw = 1024l*pBild->fXYWinkel3D, yw = 1024l*( 1.0-pBild->fXYWinkel3D );
	// Z-Weite nur mal 256, da sonst evt. ï¿½berlaufprobleme!
	long zw = 256l*pBild->fZSkal3D*( 1.0-pBild->fZWinkel3D )*MAX_3D_HOEHE;
	long angle = 1024l*pBild->fZWinkel3D;
	long lColorFactor = 1024l/fColorFaktor;
	LPUWORD pLine;
	LPUCHAR	pDest;
	LPUCHAR	pMaxDest;       // Begrenzung
	LONG fh, x, dx, y, dy, offset;
	LONG col, a_col;

	fh = ( ( dw*xw+dh*yw )/1024l )*angle/1024l;
	yoff = hh-yoff-fh-zw/256l;
	max = pBild->uMaxDaten;
	pMaxDest = pBits+( hh*ww );

	// Nun kopieren
	for( y = 0;  y < dh-1;  y += w3DZoom ) {
		pLine = pSrc+y*dw;
		for( x = 0;  x < dw-1;  x += w3DZoom ) {
			col = pLine[x]-min;
			if( col > max ) {
				col = max;
			}
			else if( col < 0 ) {
				col = 0;
			}
			a_col = col*zw/( 256l*max );      // Topografie ist immer zwischen 0 und MAX_3D_HOEHE Pixel hoch!
			if( pBild->bKonturen &&  col%pBild->uKontur <= pBild->uKonturToleranz ) { // Konturen
				col = 3;
			}
			else if( pBild->bModuloKonturen ) {
				col = (BYTE)( StartColor+( ( col%pBild->uModulo )*1024l )/lColorFactor );
			}
			else {
				col = col*1024l/lColorFactor;                   // Dagegen gibt die Farbe einen Palettenindex, der erst "von Hand" berechnet wird
				col += StartColor;
			}
			dx = xoff + ( ( dh-y )*xw+x*yw )/1024l;
			dy = yoff + fh - ( ( y*yw + x*xw )/1024l )*angle/1024l;
			offset = dx+( dy*ww );
			pDest = pBits + offset;
			a_col++;
			while( a_col-- > 0  &&  pDest < pMaxDest ) {
				*pDest = (BYTE)col;
				pDest += ww;
			}
		}
	}

	// Letzte Zeile
	pLine = pSrc+( dh-1 )*dw;
	for( x = 0; x < dw;  x++ ) {
		col = pLine[x]-min;
		if( col > max ) {
			col = max;
		}
		else if( col < 0 ) {
			col = 0;
		}
		a_col = col*zw/( 256l*max );
		dx = xoff + x*yw/1024l;
		dy = yoff + fh - ( ( dh*yw + x*xw )/1024l )*angle/1024l;
		offset = (LONG)( dx+( dy*ww ) );
		pDest = pBits+offset;
		a_col++;
		while( a_col-- > 0  &&  pDest < pMaxDest ) {
			*pDest = 2;     // Begrenzungsfarbe!
			pDest += ww;
		}
	}

	// Letzte Spalte
	for( y = 0;  y < dh;  y++ ) {
		col = pSrc[( dw-1 )+y*dw]-min;
		if( col > max ) {
			col = max;
		}
		else if( col < 0 ) {
			col = 0;
		}
		a_col = col*zw/( 256l*max );
		dx = xoff + ( ( dh-y )*xw+dw*yw )/1024l;
		dy = yoff + fh - ( ( y*yw + dw*xw )/1024l )*angle/1024l;
		offset = (LONG)( dx+( dy*ww ) );
		pDest = pBits + offset;
		a_col++;
		while( a_col-- > 0  &&  pDest < pMaxDest ) {
			*pDest = 2;     // Begrenzungsfarbe!
			pDest += ww;
		}
	}
}


// Ende Draw3D
// April 97
// "Schnelle Integeroutinen": 28.12.97
// Berechnet die neue Bitmapfarben (nur fï¿½r Recalc-Cache)
WORD FarbenSetzen( LPBITMAPINFO pCacheDib, LPBILD pBild, LONG startcol, LONG max, double maxcol, BOOL StartEnde )
{
	LONG startoffset, endoffset, endcol;
	COLORREF offsetcolor[3];

	endcol = startcol + max*maxcol+0.5;
	startoffset = startcol;
	if( StartEnde ) {
		startoffset += pBild->fStart*( endcol-startcol )/100.0;
	}
	endoffset = endcol;
	if( StartEnde ) {
		endoffset = startcol+pBild->fEnde*( endcol-startcol )/100.0;
	}
	if( startoffset >= endoffset ) {
		if( endoffset < endcol ) {
			endoffset++;
		}
		else {
			startoffset--;
		}
	}
	SetDibPaletteColors( pCacheDib, (COLORREF HUGE*)( pBild->Farben ), pBild, startoffset, endoffset, startcol, endcol );

	// Cips reagions in Start-/Endcolor
	if( (LONG)pBild > 1  &&  !pBild->bNoLUT  &&  pBild->iNumColors > 0 ) {
		// Take from LUT
		if( startcol < startoffset ) {
			offsetcolor[0] = pBild->LUT[0];
			offsetcolor[2] = pBild->LUT[0];
			SetDibPaletteColors( pCacheDib, (COLORREF HUGE*)offsetcolor, NULL, startcol, startoffset, startcol, endcol );
		}
		if( endoffset < endcol ) {
			offsetcolor[0] = pBild->LUT[pBild->iNumColors-1];
			offsetcolor[2] = pBild->LUT[pBild->iNumColors-1];
			SetDibPaletteColors( pCacheDib, (COLORREF HUGE*)offsetcolor, NULL, endoffset, endcol, startcol, endcol );
		}
	}
	else {
		// Take from Memory
		if( startcol < startoffset ) {
			offsetcolor[0] = ( (COLORREF HUGE*)( pBild->Farben ) )[0];
			offsetcolor[2] = ( (COLORREF HUGE*)( pBild->Farben ) )[0];
			SetDibPaletteColors( pCacheDib, (COLORREF HUGE*)offsetcolor, NULL, startcol, startoffset, startcol, endcol );
		}
		if( endoffset < endcol ) {
			offsetcolor[0] = ( (COLORREF HUGE*)( pBild->Farben ) )[2];
			offsetcolor[2] = ( (COLORREF HUGE*)( pBild->Farben ) )[2];
			SetDibPaletteColors( pCacheDib, (COLORREF HUGE*)offsetcolor, NULL, endoffset, endcol, startcol, endcol );
		}
	}
	return ( (WORD)endcol );
}
// 22.11.97


// Skala fï¿½r 3D-Bitmap malen
void Draw3DSkala( HDC hDC, WORD x_or, WORD y_or, LPBMPDATA pBmp, LPSNOMDATA pSnom, LPBILD pBild, WORD max )
{
	// Zuerst 3D-Skalierung; ist mit Abstand schwieriger
	// Nur ein paar Abkï¿½rzungen ...
	double xw, yw, zw, angle;
	long fh;
	double inc, wert, offset;
	int iPrecis, x, y, ch, cnt;
	BYTE str[16];
	SIZE size;

	GetTextExtentPoint( hDC, "0", 1, &size );
	ch = size.cy;
	if( GetMapMode( hDC ) != MM_TEXT ) {
		ch = -size.cy;
	}

	pBild->fXYWinkel3D = f3DXYWinkel;
	pBild->fZWinkel3D = f3DZWinkel;
	pBild->fZSkal3D = f3DZSkal;
	xw = pBild->fXYWinkel3D;
	yw = ( 1.0-pBild->fXYWinkel3D );
	zw = pBild->fZSkal3D*( 1.0-pBild->fZWinkel3D )*MAX_3D_HOEHE;
	angle = pBild->fZWinkel3D;
	fh = ( pSnom->w*xw+pSnom->h*yw )*angle;

	// Skalierung Z-Achse
	if( !pBild->bShowNoZ ) {
		y = zw/( ch+2 );
		if( y < 2 ) {
			inc = CalcIncrement( 1, 1, max*pBild->fSkal );
		}
		else {
			inc = CalcIncrement( y, y+2, max*pBild->fSkal );
		}
		SetTextAlign( hDC, TA_RIGHT|TA_TOP );
		// Zuerst: Z-Achse
		y = y_or + zw + fh-pSnom->w*xw*angle;
		MoveToEx( hDC, x_or, y, NULL );
		y = y_or + fh-pSnom->w*xw*angle;
		LineTo( hDC, x_or, y );
		if( inc >= 500.0  &&  !pBild->bSpecialZUnit ) {
			iPrecis = CalcDigits( inc/1000.0 );
		}
		else {
			iPrecis = CalcDigits( inc );
		}
		// Skalierung der Z-Achse automatisch
		if( pBild->bSpecialZUnit ) {
			TextOut( hDC, x_or, y-2*ch, pBild->strZUnit, lstrlen( pBild->strZUnit ) );
		}
		else {
			// Sonst einfach Topografie ...
			if( inc > 500.0 ) {
				TextOut( hDC, x_or, y-2*ch, STR_TOPO_SUNIT, 6 );
			}
			else {
				TextOut( hDC, x_or, y-2*ch, STR_TOPO_UNIT, 6 );
			}
		}

		// Dann Achsenmarkierungen
		for( wert = 0.0;  wert < max*pBild->fSkal;  wert += inc ) {
			y = wert/pBild->fSkal;
			y = y_or + zw + fh-pSnom->w*xw*angle - ( y*zw )/max;
			MoveToEx( hDC, x_or, y, NULL );
			LineTo( hDC, x_or-6, y );
			if( !pBild->bSpecialZUnit  &&  inc > 500.0 ) {
				sprintf( str, "%.*lf", iPrecis, wert/1000.0 );
			}
			else {
				sprintf( str, "%.*lf", iPrecis, wert );
			}
			TextOut( hDC, x_or-8, y-ch/2, str, lstrlen( str ) );
		}
	}

	// Skalierung X-Achse
	inc = CalcIncrement( 3, 5, pSnom->w*pSnom->fX );
	SetTextAlign( hDC, TA_RIGHT|TA_TOP );
	offset = 0.0;
	if( pBmp->pPsi.cShowOffset ) {
		offset = pSnom->fXOff;
	}
	if( inc >= 250.0 ) {
		iPrecis = CalcDigits( inc/1000.0 );
	}
	else {
		iPrecis = CalcDigits( inc );
	}
	wert = -fmod( offset+inc*13.0, inc );
	for(  ;  wert <= pSnom->w*pSnom->fX;  wert += inc ) {
		x = wert/pSnom->fX;
		y = y_or + zw + fh - ( 0*yw + x*xw )*angle;
		x = x_or + 0*xw+( pSnom->w-x )*yw;
		if( inc >= 250.0 ) {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset )/1000.0 );
		}
		else {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset ) );
		}
		MoveToEx( hDC, x, y, NULL );
		LineTo( hDC, (int)( x-10*xw ), (int)( y+10*yw*angle ) );
		TextOut( hDC, (int)( x-10*xw ), (int)( y+10*yw*angle ), str, lstrlen( str ) );
	}
	if( inc >= 250.0 ) {
		TextOut( hDC, (int)( x_or+pSnom->w/2*yw-30*xw ), (int)( y_or+zw+fh-pSnom->w/2*xw*angle+30*yw ), STR_X_SUNIT, 6 );
	}
	else {
		TextOut( hDC, (int)( x_or+pSnom->w/2*yw-30*xw ), (int)( y_or+zw+fh-pSnom->w/2*xw*angle+30*yw ), STR_X_UNIT, 6 );
	}

	// Y-Achse
	SetTextAlign( hDC, TA_LEFT|TA_TOP );
	offset = 0.0;
	if( pBmp->pPsi.cShowOffset ) {
		offset = pSnom->fYOff;
	}
	wert = -fmod( offset+inc*13.0, inc );
	for( cnt = 0;  wert <= pSnom->h*pSnom->fY  &&  cnt < 50;  wert += inc, cnt++ ) {
		y = wert/pSnom->fY;
		x = x_or + y*xw+pSnom->w*yw;
		y = y_or + zw + fh - ( y*yw + 0*xw )*angle;
		if( inc >= 250.0 ) {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset )/1000.0 );
		}
		else {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset ) );
		}
		MoveToEx( hDC, x, y, NULL );
		LineTo( hDC, (int)( x+10*yw ), (int)( y+10*xw*angle ) );
		TextOut( hDC, (int)( x+10*yw ), (int)( y+10*xw*angle ), str, lstrlen( str ) );
	}
	if( inc >= 250.0 ) {
		TextOut( hDC, (int)( x_or+pSnom->h/2*xw+pSnom->w*yw+30*xw ), (int)( y_or+zw+fh-pSnom->h/2*yw*angle+30*yw ), STR_Y_SUNIT, 6 );
	}
	else {
		TextOut( hDC, (int)( x_or+pSnom->h/2*xw+pSnom->w*yw+30*xw ), (int)( y_or+zw+fh-pSnom->h/2*yw*angle+30*yw ), STR_Y_UNIT, 6 );
	}
}
// 22.11.97


// Malt eine waagerechte Achse
void DrawHorizontalAxis( HDC hDC, int x, int y, int weite, int min, int max, double MaxWeite, double offset, const char *label, const char *alt_label )
{
	double inc, wert;
	WORD x_pos;
	int iPrecis, ch;
	BYTE str[16];
	SIZE size;

	if( MaxWeite == 0 ) {
		return;
	}
	GetTextExtentPoint( hDC, "0", 1, &size );
	ch = size.cy;
	if( GetMapMode( hDC ) != MM_TEXT ) {
		ch = -size.cy;
	}
	SetTextAlign( hDC, TA_CENTER|TA_TOP );
	inc = CalcIncrement( min, max, MaxWeite );
	if( alt_label  &&  inc >= 250.0 ) {
		iPrecis = CalcDigits( inc/1000.0 );
	}
	else {
		iPrecis = CalcDigits( inc );
	}
	if( offset != 0.0 ) {
		wert = inc-fmod( offset+inc*13.0, inc );
	}
	else {
		wert = 0.0;
	}
	for(  ;  wert <= MaxWeite;  wert += inc ) {
		x_pos = x+( weite*wert/MaxWeite+.5 );
		if( alt_label  &&  inc >= 250.0 ) {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset )/1000.0 );
		}
		else {
			sprintf( str, "%.*lf", iPrecis, ( wert+offset ) );
		}
		MoveToEx( hDC, x_pos, y, NULL );
		LineTo( hDC, x_pos, y+ch/2 );
		TextOut( hDC, x_pos, y+ch/2, str, lstrlen( str ) );
	}
	if( alt_label  &&  inc >= 250.0 ) {
		TextOut( hDC, x+weite/2, y+ch*2, alt_label, lstrlen( alt_label ) );
	}
	else {
		TextOut( hDC, x+weite/2, y+ch*2, label, lstrlen( label ) );
	}
}
// 22.11.97


// Malt eine senkrechte Achse
// return the width of the widest entry
LONG DrawVerticalAxis( HDC hDC, int x, int y, int hoehe, int min, int max, double fHoehe, double fOffset, const char *label, const char *alt_label )
{
	double inc, wert;
	WORD y_pos;
	BOOL neg_h = FALSE;
	int iPrecis;
	BYTE str[16];
	int ch, cw, iWidth = 0;                         // width of the widest description string
	SIZE size;
	double divider = 1.0;

	if( fHoehe == 0 ) {
		return ( 0 );
	}
	// Skalierung z-Achse
	SetTextAlign( hDC, TA_LEFT|TA_TOP );
	GetTextExtentPoint( hDC, "0", 1, &size );
	cw = size.cx;
	ch = size.cy;
	if( GetMapMode( hDC ) != MM_TEXT ) {
		ch = -ch;
	}
	y_pos = labs( hoehe/( size.cy+2 ) );
	if( y_pos <= max ) {
		inc = CalcIncrement( min, labs( y_pos )-1, fHoehe );
	}
	else {
		inc = CalcIncrement( max-1, max, fHoehe );
	}
	if( inc >= 500.0  &&  alt_label != NULL ) {
		iPrecis = CalcDigits( inc/1000.0 );
		divider = 1000.0;
	}
	else {
		iPrecis = CalcDigits( inc );
	}
	if( fOffset != 0.0 ) {
		wert = inc-fmod( fOffset+inc*13.0, inc );
	}
	else {
		wert = 0.0;
	}
	for(  ;  fabs( wert ) < fabs( fHoehe );  wert += inc ) {
		y_pos = y+( hoehe*wert )/fHoehe+.5;
		MoveToEx( hDC, x, y_pos, NULL );
		LineTo( hDC, x+cw, y_pos );
		sprintf( str, "%.*lf", iPrecis, ( wert+fOffset )/divider );
		TextOut( hDC, x+( cw*3 )/2, y_pos-ch/2, str, lstrlen( str ) );
		GetTextExtentPoint( hDC, str, lstrlen( str ), &size );
		if( iWidth < size.cx ) {
			iWidth = size.cx;
		}
	}
	iWidth += ( cw*3 )/2;

	// Axis label
	if( ch > 0 ) {
		if( hoehe < 0 ) {
			y += hoehe;
		}
	}
	else if( hoehe > 0 ) {
		y += hoehe;
	}

	SetTextAlign( hDC, TA_RIGHT|TA_BOTTOM );
	if( divider > 999.0 ) {
		TextOut( hDC, x+iWidth, y-ch/2, alt_label, lstrlen( alt_label ) );
	}
	else {
		TextOut( hDC, x+iWidth, y-ch/2, label, lstrlen( label ) );
	}
	return ( iWidth );
}
// 22.11.97


// Zeichnet die Cache-Bitmap neu
// Vorsicht: Monster-Routine (trotz allen Auslagerns ...)
BOOL RecalcCache( LPBMPDATA pBmp, BOOL Bitmaps, BOOL DontEmulContrast )
{
	LPSNOMDATA pSnom;
	LPBITMAPINFO pCacheDib = (LPBITMAPINFO)pBmp->pDib;
	LPBILD pLinks;
	LPBILD pRechts;
	LPUWORD	pSrc;
	LPUCHAR	pBits, pDest;

	HDC hDC, hDC2;
	SIZE size;
	RECT xywh;
	double maxlinkscol = 1.0, maxrechtscol = 1.0;
	long lScaleFactor;      // =1024/maxlinkscol bzw. 1024/maxrechtscol
	LONG usedcol, w, ww, h, skala_w, skala_h;
	WORD startcol, endcol;
	WORD maxlinks = 0, maxrechts = 0;
	int iAkt = pBmp->iAktuell;
	char str[16];           // fï¿½r Zahlen


	// Zuerst Pointer fï¿½r rechte und linke Seite holen
	pSnom = &( pBmp->pSnom[iAkt] );
	pLinks = NULL;
	switch( pBmp->Links ) {
		case TOPO:
			pLinks = &( pBmp->pSnom[iAkt].Topo );
			break;

		case ERRO:
			pLinks = &( pBmp->pSnom[iAkt].Error );
			break;

		case LUMI:
			pLinks = &( pBmp->pSnom[iAkt].Lumi );
			break;
	}
	if( pLinks  &&  pLinks->Typ == NONE ) {
		pLinks = NULL;
	}
	pRechts = NULL;
	switch( pBmp->Rechts ) {
		case TOPO:
			pRechts = &( pBmp->pSnom[iAkt].Topo );
			break;

		case ERRO:
			pRechts = &( pBmp->pSnom[iAkt].Error );
			break;

		case LUMI:
			pRechts = &( pBmp->pSnom[iAkt].Lumi );
			break;
	}
	if( pRechts  &&  pRechts->Typ == NONE ) {
		pRechts = NULL;
	}
	// Nichts zu tun?
	if( pLinks == NULL  &&  pRechts == NULL ) {
		// Nur die ï¿½berschrift
		return ( TRUE );
	}

	hDC = GetDC( NULL );
	// Farben "gerecht" verteilen; wenn Bitmap 1 32000 und Bitmap 2 nur 15 hat, Farben im gleichen
	// Verhï¿½ltnis zuweisen; Minimum sind 15 Farben!
	// Dabei wird (noch?) nicht gecheckt, ob evt. Farben doppelt genutzt werden kï¿½nnten
	if( pLinks ) {
		if( pLinks->bModuloKonturen ) {
			maxlinks = pLinks->uModulo+1;
		}
		else {
			maxlinks = pLinks->uMaxDaten;
		}
	}
	if( pRechts ) {
		if( pRechts->bModuloKonturen ) {
			maxrechts = pRechts->uModulo+1;
		}
		else {
			maxrechts = pRechts->uMaxDaten;
		}
	}
	usedcol = (unsigned long)maxrechts + (unsigned long)maxlinks;

	// mehr als 250 Farben?, dann muss geteilt werden
	if( usedcol > 250l ) {
		if( pLinks  &&  pRechts ) {
			maxlinkscol = 124.5/( (double)maxlinks+1.0 );
			maxrechtscol = 124.5/( (double)maxrechts+1.0 );
		}
		else if( pLinks ) {
			maxlinkscol = 249.0/( (double)maxlinks+1.0 );
		}
		else {
			maxrechtscol = 249.0/( (double)maxrechts+1.0 );
		}

		// Wert >1.0 (= mehrere Farben fï¿½r einen Bitwert) sind sinnlos!
		if( maxlinkscol > 1.0 ) {
			maxlinkscol = 1.0;
		}
		if( maxrechtscol > 1.0 ) {
			maxrechtscol = 1.0;
		}
	}       // Jetzt werden maximal 252 Farben von den Bitmaps gebraucht

	// Endlich die Anzahl der nï¿½tigen Farben berechnen
	usedcol = (WORD)( maxlinks*maxlinkscol+0.5 )+(WORD)( maxrechts*maxrechtscol+0.5 );
	if( usedcol < 2 ) {
		return ( FALSE );
	}

	pCacheDib->bmiHeader.biXPelsPerMeter = GetDeviceCaps( hDC, LOGPIXELSX )*39.37;
	pCacheDib->bmiHeader.biYPelsPerMeter = GetDeviceCaps( hDC, LOGPIXELSY )*39.37;

	// Farbe 0 = Hintergrund
	pCacheDib->bmiColors[0].rgbRed = GetRValue( cHinten );
	pCacheDib->bmiColors[0].rgbGreen = GetGValue( cHinten );
	pCacheDib->bmiColors[0].rgbBlue = GetBValue( cHinten );
	// Farbe 1 = Schwarz
	pCacheDib->bmiColors[1].rgbRed = 0;
	pCacheDib->bmiColors[1].rgbGreen = 0;
	pCacheDib->bmiColors[1].rgbBlue = 0;
	// Farbe 2 = Vordergrund
	pCacheDib->bmiColors[2].rgbRed = GetRValue( cVorn );
	pCacheDib->bmiColors[2].rgbGreen = GetGValue( cVorn );
	pCacheDib->bmiColors[2].rgbBlue = GetBValue( cVorn );
	// Farbe 3 = Markierung
	pCacheDib->bmiColors[3].rgbRed = GetRValue( cMarkierungLinks );
	pCacheDib->bmiColors[3].rgbGreen = GetGValue( cMarkierungLinks );
	pCacheDib->bmiColors[3].rgbBlue = GetBValue( cMarkierungLinks );
	// Farbe 4 = !Markierung
	pCacheDib->bmiColors[4].rgbRed = GetRValue( cMarkierungRechts );
	pCacheDib->bmiColors[4].rgbGreen = GetGValue( cMarkierungRechts );
	pCacheDib->bmiColors[4].rgbBlue = GetBValue( cMarkierungRechts );

	// "Links"-Farben zuweisen
	endcol = 4;
	startcol = 5;
	if( pLinks ) {
		endcol = FarbenSetzen( pCacheDib, pLinks, startcol, maxlinks, maxlinkscol, ( !DontEmulContrast )|pLinks->bPseudo3D );
	}
	// "Rechts"-Farben zuweisen
	startcol = endcol+1;
	if( pRechts ) {
		endcol = FarbenSetzen( pCacheDib, pRechts, startcol, maxrechts, maxrechtscol, ( !DontEmulContrast )|pRechts->bPseudo3D );
	}

	//****  Ab hier werden die Ausmaï¿½e berechet
	w = h = 0;
	SelectObject( hDC, CreateFontIndirect( &lf ) );  // Fï¿½r alle Rechnungen
	// LinksX==0  (analog fï¿½r RechtsX) heiï¿½t: Nicht existent!

	pBmp->rectLinks.left = pBmp->rectLinks.right = pBmp->rectRechts.left = pBmp->rectRechts.right = 0;
	pBmp->rectLinks.top = pBmp->rectRechts.top = 0;         // Doppelte Hï¿½he der ï¿½berschriften

	if( pLinks ) {
		// Ausmaï¿½e der Skala berechnen
		gcvt( CalcIncrement( 3, 9, pLinks->uMaxDaten*pLinks->fSkal )*5.0+0.23, 4, str );
		GetTextExtentPoint( hDC, str, lstrlen( str ), &size );
		skala_w = size.cx;
		skala_h = size.cy*3+8;
		GetTextExtentPoint( hDC, "Ty", 2, &size );
		size.cy *= 2;   // Doppelte Hï¿½he der ï¿½berschriften
		CalcDibSize( hDC, pSnom->w, pSnom->h, &xywh, pLinks->bPseudo3D, FALSE );
		pBmp->rectLinks.left = (int)( 26+skala_w );
		pBmp->rectLinks.top = 10+size.cy;
		h = 20+xywh.bottom+size.cy+skala_h;
		pBmp->rectLinks.right = pBmp->rectLinks.left + xywh.right;
		pBmp->rectLinks.bottom = pBmp->rectLinks.top + xywh.bottom;
		w = xywh.right+20+skala_w*2;
	}
	if( pRechts ) {
		gcvt( CalcIncrement( 3, 9, pRechts->uMaxDaten*pRechts->fSkal )*5.0+0.23, 4, str );
		GetTextExtentPoint( hDC, str, lstrlen( str ), &size );
		skala_w = size.cx;
		skala_h = size.cy*3+8;
		GetTextExtentPoint( hDC, "Ty", 2, &size );
		size.cy *= 2;   // Doppelte Hï¿½he der ï¿½berschriften
		CalcDibSize( hDC, pSnom->w, pSnom->h, &xywh, pRechts->bPseudo3D, FALSE );
		if( h < 2l+xywh.bottom+size.cy+skala_h ) {
			h = 20+xywh.bottom+size.cy+skala_h;
		}
		pBmp->rectRechts.left = (int)( w+26+skala_w );
		pBmp->rectRechts.top = 10+size.cy;
		pBmp->rectRechts.right = pBmp->rectRechts.left + xywh.right;
		pBmp->rectRechts.bottom = pBmp->rectRechts.top + xywh.bottom;
		w += xywh.right+20+skala_w*2;
	}
	DeleteObject( SelectObject( hDC, GetStockObject( SYSTEM_FONT ) ) );     // Zeichensatz wieder lï¿½schen

	pCacheDib->bmiHeader.biWidth = w;
	pCacheDib->bmiHeader.biHeight = h;

	// Ausmaï¿½e Fenster berechnen
	pBmp->rectFenster.left = pBmp->rectFenster.top = 0;
	pBmp->rectFenster.right = w;
	pBmp->rectFenster.bottom = h;

	// Ausmaï¿½e Scanline berechnen
	{
		if( !pBmp->bPlotUnten )	{
			// Bild ist daneben
			pBmp->rectPlot.left = w+10;
			pBmp->rectPlot.right = w+30+xywh.right+skala_w*2;
			if( pBmp->rectLinks.top != 0 ) {
				pBmp->rectPlot.top = pBmp->rectLinks.top;
			}
			else {
				pBmp->rectPlot.top = pBmp->rectRechts.top;
			}
			pBmp->rectPlot.bottom = pBmp->rectPlot.top+pSnom->h;
		}
		else {
			// Bild ist darunter
			if( pBmp->rectLinks.left != 0 ) {
				pBmp->rectPlot.left = pBmp->rectLinks.left;
			}
			else {
				pBmp->rectPlot.left = pBmp->rectRechts.left;
			}
			if( pBmp->rectRechts.right != 0 ) {
				pBmp->rectPlot.right = pBmp->rectRechts.right;
			}
			else {
				pBmp->rectPlot.right = pBmp->rectLinks.right;
			}
			pBmp->rectPlot.top = h+10;
			pBmp->rectPlot.bottom = h+10+pSnom->h;
		}
	}

	// Platz fï¿½r Scanline evt. schon mal dazurechnen
	if( pBmp->bIsScanLine  &&  pBmp->lMaxScan > 0 )	{
		pBmp->rectFenster.right = 64+pBmp->rectPlot.right;      // Rechts Platz lassen
		if( pBmp->bPlotUnten ) {
			pBmp->rectFenster.bottom = pBmp->rectPlot.bottom+pBmp->rectLinks.top;
		}
	}

	// Evt. ist hier Schluss (nur Farbpaletten updaten ... !)
	if( !Bitmaps ) {
		ReleaseDC( NULL, hDC );
		return ( TRUE );
	}

	// **********************************************************
	// **	Ab hier nur, wenn der Inhalt neu gemalt werden muss! **
	// **********************************************************
	ww = ( w+3l )&( ~3l );
	pBits = (LPUCHAR)pMalloc( ww*h );
	if( pBits == NULL ) {
		return ( FALSE );
	}

	// Text und Grafik fï¿½r Skalierung werden eingefï¿½gt
	// (geht nur vor den Daten ..., Windows sei Dank!)
	// Ansonsten werden die Farben kastriert!
	{
		HGDIOBJ	hBitmap, hOld, hOldPen;
		LPBITMAPINFO pTextDib;
		LONG i;

		// Zuerst das Vorgeplï¿½nkel, anlegen von Memory-DC, usw.
		pTextDib = (LPBITMAPINFO)pMalloc( sizeof( BITMAPINFOHEADER )+256*sizeof( RGBQUAD ) );
		for( i = 0;  i < 256;  i++ ) {
			( (LPWORD)( pTextDib->bmiColors ) )[i] = i;
		}
		MemMove( pTextDib, pCacheDib, sizeof( BITMAPINFOHEADER ) ); //+256*sizeof(RGBQUAD) );
		hDC2 = CreateCompatibleDC( hDC );
		hBitmap = CreateDIBitmap( hDC, (BITMAPINFOHEADER far*)pTextDib, CBM_INIT, pBits, (BITMAPINFO far*)pTextDib, DIB_RGB_COLORS );
#if __NEVER_EVER__
		// Darf nie angestellt werden! (sonst werden im
		// 256-Farben Modus alle Farben verstellt!) Zwei Tage Fehlersuche!
		//		SetSystemPaletteUse( hDC2, SYSPAL_NOSTATIC );
#endif
		hOld = SelectObject( hDC2, hBitmap );

		// Geeignete Zeichenparameter aus dem Hut zaubern ...
		SetMapMode( hDC2, MM_TEXT );
		GetTextExtentPoint( hDC, "0", 1, &size );
		SetBkMode( hDC2, TRANSPARENT );
		SetTextColor( hDC2, PALETTEINDEX( 1 ) );
		hOldPen = SelectObject( hDC2, CreatePen( PS_SOLID, 0, PALETTEINDEX( 1 ) ) );

		// Endlich Achsen und Bilder beschriften
		if( pLinks ) {
			SetTextAlign( hDC2, TA_LEFT|TA_TOP );
			lf.lfHeight *= 2;
			SelectObject( hDC2, CreateFontIndirect( &lf ) );
			lf.lfHeight /= 2;
			TextOut( hDC2, pBmp->rectLinks.left, 5, pLinks->strTitel, lstrlen( pLinks->strTitel ) );
			DeleteObject( SelectObject( hDC2, CreateFontIndirect( &lf ) ) );
			GetTextExtentPoint( hDC2, "0", 1, &size );

			// Achsenskalierung, getrennt fï¿½r 3D und Normal
			if( pLinks->bPseudo3D ) {
				Draw3DSkala( hDC2, pBmp->rectLinks.left, pBmp->rectLinks.top, pBmp, pSnom, pLinks, pLinks->uMaxDaten );
			}
			else {
				//****	2D-Skalierung (ist zum Glï¿½ck einfacher ...)
				// Skalierung x-Achse
				DrawHorizontalAxis( hDC2, pBmp->rectLinks.left, pBmp->rectLinks.top+pSnom->h, pSnom->w, 3, 5, pSnom->w*pSnom->fX, pBmp->pPsi.cShowOffset*pSnom->fXOff, STR_X_UNIT, STR_X_SUNIT );
				// Skalierung z-Achse
				if( !pLinks->bShowNoZ )	{
					if( pLinks->bSpecialZUnit ) {
						DrawVerticalAxis( hDC2, 10, pBmp->rectLinks.top, pSnom->h, 2, 10, maxlinks*pLinks->fSkal*pLinks->fEnde/100.0, 0.0, pLinks->strZUnit, NULL );
					}
					else {
						DrawVerticalAxis( hDC2, 10, pBmp->rectLinks.top, pSnom->h, 2, 10, maxlinks*pLinks->fSkal*pLinks->fEnde/100.0, 0.0, STR_TOPO_UNIT, STR_TOPO_SUNIT );
					}
				}
			}
		}

		// Endlich Achsen und Bilder beschriften: Rechts ...
		if( pRechts ) {
			SetTextAlign( hDC2, TA_LEFT|TA_TOP );
			lf.lfHeight *= 2;
			SelectObject( hDC2, CreateFontIndirect( &lf ) );
			lf.lfHeight /= 2;
			TextOut( hDC2, pBmp->rectRechts.left, 5, pRechts->strTitel, lstrlen( pRechts->strTitel ) );
			DeleteObject( SelectObject( hDC2, CreateFontIndirect( &lf ) ) );
			GetTextExtentPoint( hDC2, "0", 1, &size );

			// Achsenskalierung, getrennt fï¿½r 3D und Normal
			if( pRechts->bPseudo3D ) {
				Draw3DSkala( hDC2, pBmp->rectRechts.left, pBmp->rectRechts.top, pBmp, pSnom, pRechts, pRechts->uMaxDaten );
			}
			else {
				//****	2D-Skalierung (ist zum Glï¿½ck einfacher ...)
				// Skalierung x-Achse
				DrawHorizontalAxis( hDC2, pBmp->rectRechts.left, pBmp->rectRechts.top+pSnom->h, pSnom->w, 3, 5, pSnom->w*pSnom->fX, pBmp->pPsi.cShowOffset*pSnom->fXOff, STR_X_UNIT, STR_X_SUNIT );
				// Skalierung z-Achse
				if( !pRechts->bShowNoZ ) {
					if( pRechts->bSpecialZUnit ) {
						DrawVerticalAxis( hDC2, pBmp->rectRechts.left-skala_w-16, pBmp->rectRechts.top, pSnom->h, 2, 10, maxrechts*pRechts->fSkal*pRechts->fEnde/100.0, 0.0, pRechts->strZUnit, NULL );
					}
					else {
						DrawVerticalAxis( hDC2, pBmp->rectRechts.left-skala_w-16, pBmp->rectRechts.top, pSnom->h, 2, 10, maxrechts*pRechts->fSkal*pRechts->fEnde/100.0, 0.0, STR_TOPO_UNIT, STR_TOPO_SUNIT );
					}
				}
			}
		}

		// und nun die mï¿½hsam gewonnen Daten wieder in ein Bild wandeln ...
		SelectObject( hDC2, hOld );
		GetDIBits( hDC2, hBitmap, 0, (WORD)( pCacheDib->bmiHeader.biHeight ), pBits, (BITMAPINFO far*)pTextDib, DIB_PAL_COLORS );
		DeleteObject( hBitmap );
		// Zeichensatz wieder lï¿½schen
		DeleteObject( SelectObject( hDC2, hOldPen ) );
		DeleteObject( SelectObject( hDC2, GetStockObject( SYSTEM_FONT ) ) );
		DeleteDC( hDC2 );
		MemFree( pTextDib );
		// dann eben per Hand alle Texte schwarz ...
		for( i = 0;  i < pCacheDib->bmiHeader.biHeight*ww;  i++ ) {
			if( pBits[i] ) {
				pBits[i] = 1;
			}
		}
	}
	ReleaseDC( NULL, hDC );

	//  Endlich Bitmapdaten erstellen
	//  Topografie zuerst
	startcol = 5;
	endcol = 4;
	if( pLinks ) {
		LONG x, y, lowlim;
		LPUWORD Daten = pLinks->puDaten;

		// Evt. sind die Daten nur ein Pointer
		if( (LONG)Daten <= 256l ) {
			switch( pLinks->Typ ) {
				case TOPO:
					Daten = pBmp->pSnom[(LONG)Daten-1].Topo.puDaten;
					break;

				case ERRO:
					Daten = pBmp->pSnom[(LONG)Daten-1].Error.puDaten;
					break;

				case LUMI:
					Daten = pBmp->pSnom[(LONG)Daten-1].Lumi.puDaten;
					break;
			}
		}

		endcol = startcol + ( (double)maxlinks*maxlinkscol+0.5 );
		if( !pLinks->bPseudo3D  ||  maxlinks <= 2 ) {
			pDest = pBits+pBmp->rectLinks.left+( h-pBmp->rectLinks.top-pSnom->h )*ww;
			if( !pLinks->bShowNoZ )	{
				// fï¿½r die Legende den Graustreifen ...
				for( y = 0;  y < pSnom->h;  y++ ) {
					// -1 bei y, da die Bitmap nur bis pSnom->h-1 lï¿½uft (for y<h!)
					for( x = 0;	 x < 8;	   x++ ) {
						if( !DontEmulContrast ) {
							pDest[x-pBmp->rectLinks.left+2+( pSnom->h-y-1 )*ww] = (BYTE)( startcol+pLinks->fStart*( endcol-startcol )/100.0 +( y*( endcol-startcol ) )*( pLinks->fEnde-pLinks->fStart )/100.0/pSnom->h );
						}
						else {
							pDest[x-pBmp->rectLinks.left+2+( pSnom->h-y-1 )*ww] = (BYTE)( startcol +( y*( endcol-startcol ) )/pSnom->h );
						}
					}
				}
			}

			pDest = pBits+pBmp->rectLinks.left+( h-pBmp->rectLinks.top-pSnom->h )*ww;
			pSrc = Daten+pSnom->w*( pSnom->h-1 );
			lScaleFactor = 1024l/maxlinkscol;
			lowlim = 0;
			if( DontEmulContrast ) {
				lowlim = pLinks->fStart*pLinks->uMaxDaten/100.0;
				maxlinks = lowlim + pLinks->fEnde*pLinks->uMaxDaten/100.0;
				lScaleFactor = 1024.0*( pLinks->fEnde )/( maxlinkscol*100.0 );
			}

			if( pColorConvert  &&  !pBmp->pMaske ) {
				// Tabelle zu schnellen Berechnung initialisieren
				for( x = 0;  x < pLinks->uMaxDaten;  x++ ) {
					if( pLinks->bModuloKonturen ) {
						pColorConvert[x] = (BYTE)( startcol+( x%pLinks->uModulo )*maxlinkscol+.5 );
					}
#ifdef SIMPLE_CONTOUR
					else if( pLinks->bKonturen  &&  x%pLinks->uKontur <= pLinks->uKonturToleranz ) { // Konturen
						pColorConvert[x] = 4;
					}
					else
#endif
					else if( x >= maxlinks ) {
						pColorConvert[x] = endcol;
					}
					else if( x < lowlim ) {
						pColorConvert[x] = startcol;
					}
					else {
						pColorConvert[x] = (BYTE)( startcol + ( ( (long)x-lowlim )<<10l )/lScaleFactor );
					}
				}

				for( y = 0;  y < pSnom->h;  y++ ) {
					for( x = 0;  x < pSnom->w;  x++ ) {
						pDest[x] = pColorConvert[ pSrc[x] ];
					}
					pDest += ww;
					pSrc -= pSnom->w;
				}
			}
			else {
				for( y = 0;  y < pSnom->h;  y++ ) {
					for( x = 0;  x < pSnom->w;  x++ ) {
						if( pBmp->pMaske  &&  ( ( x+y )%2 ) == 0  &&  IsMaske( pBmp, x, y ) ) {
							pDest[x] = 3;   // Diagonale Maske
						}
#ifdef SIMPLE_CONTOUR
						else if( pLinks->bKonturen  &&  pSrc[x]%pLinks->uKontur <= pLinks->uKonturToleranz ) {  // so einfach wï¿½ren Konturen einzufï¿½gen
							pDest[x] = 4;
						}
#endif
						else if( pLinks->bModuloKonturen ) {
							pDest[x] = (BYTE)( startcol+( (long)( pSrc[x]%pLinks->uModulo )<<10l )/lScaleFactor );
						}
						else if( pSrc[x] >= maxlinks ) {
							pDest[x] = endcol-1;
						}
						else if( pSrc[x] <= lowlim ) {
							pDest[x] = startcol;
						}
						else {
							pDest[x] = (BYTE)( startcol + ( ( (long)( pSrc[x]-lowlim ) )<<10l )/lScaleFactor );
						}
					}
					pDest += ww;
					pSrc -= pSnom->w;
				}
			}
#ifndef SIMPLE_CONTOUR
			// Best contours are overlaid now ...
			if( pLinks->uKontur ) {
				UWORD last_contour, diff = pLinks->uKontur;
				pSrc = Daten;
				pDest = pBits+pBmp->rectLinks.left+( h-pBmp->rectLinks.top-pSnom->h )*ww;

				// Zuerst Zeilenweise
				for( y = 0; y < pSnom->h; y++ )	{
					last_contour = ( pSrc[y*pSnom->w] )/diff;
					for( x = 1;  x < pSnom->w-1;  x++ ) {
						if( pSrc[y*pSnom->w+x]/diff != last_contour ) {
							last_contour = ( pSrc[y*pSnom->w+x]/diff );
							pDest[( pSnom->h-1-y )*ww+x] = 4; // set contour colour
						}
					}
				}
				// Dann Spaltenweise
				for( x = 0;  x < pSnom->w;  x++ ) {
					last_contour = ( pSrc[x] )/diff;
					for( y = 1;  y < pSnom->h;  y++ ) {
						if( pSrc[y*pSnom->w+x]/diff != last_contour ) {
							last_contour = ( pSrc[y*pSnom->w+x]/diff );
							pDest[( pSnom->h-1-y )*ww+x] = 4; // set contour colour
						}
					}
				}
			}
#endif
		}
		else {
			// 3D-Bitmap erstellen
			Draw3D( pBits, pBmp->rectLinks.left, pBmp->rectLinks.top, ww, pCacheDib->bmiHeader.biHeight,
			        Daten, pSnom->w, pSnom->h, 0, maxlinks,
			        maxlinkscol, startcol, pLinks );
		}
	}

	//  Endlich und Lumineszensdaten erstellen (oder was sonst rechts drinnen ist)
	if( pRechts ) {
		LONG x, y, lLowlim;
		LPUWORD	Daten = pRechts->puDaten;

		// Evt. sind die Daten nur ein Pointer
		if( (LONG)Daten <= 256l ) {
			switch( pRechts->Typ ) {
				case TOPO:
					Daten = pBmp->pSnom[(LONG)Daten-1].Topo.puDaten;
					break;

				case ERRO:
					Daten = pBmp->pSnom[(LONG)Daten-1].Error.puDaten;
					break;

				case LUMI:
					Daten = pBmp->pSnom[(LONG)Daten-1].Lumi.puDaten;
					break;
			}
		}

		startcol = endcol+1;
		endcol = startcol + maxrechts*maxrechtscol;
		if( !pRechts->bPseudo3D  ||  maxrechts <= 2 ) {
			pDest = pBits+pBmp->rectRechts.left+( h-pBmp->rectRechts.top-pSnom->h )*ww;

			if( !pRechts->bShowNoZ ) {
				// fï¿½r die Legende den Graustreifen ...
				for( y = 0;  y < pSnom->h;  y++ ) {
					// -1 bei y, da die Bitmap nur bis pSnom->h-1 lï¿½uft (for y<h!)
					for( x = 0;	 x < 8;	   x++ ) {
						if( ( !DontEmulContrast )|pRechts->bPseudo3D ) {
							pDest[x-skala_w-24+( pSnom->h-y-1 )*ww] = (BYTE)( startcol+pRechts->fStart*( endcol-startcol )/100.0 +( y*( endcol-startcol ) )*( pRechts->fEnde-pRechts->fStart )/100.0/pSnom->h );
						}
						else {
							pDest[x-skala_w-24+( pSnom->h-y-1 )*ww] = (BYTE)( startcol + ( y*( endcol-startcol ) )/pSnom->h );
						}
					}
				}
			}

			pSrc = Daten+pSnom->w*( pSnom->h-1 );
			lScaleFactor = 1024l/maxrechtscol;
			lLowlim = 0;
			if( DontEmulContrast ) {
				lLowlim = ( pRechts->fStart*pRechts->uMaxDaten )/100.0;
				maxrechts = lLowlim + pRechts->fEnde*pRechts->uMaxDaten/100.0;
				lScaleFactor = 1024.0*( pRechts->fEnde )/( maxrechtscol*100.0 );
			}

			if( pColorConvert  &&  !pBmp->pMaske ) {
				// Tabelle zu schnellen Berechnung initialisieren
				for( x = 0;  x < pRechts->uMaxDaten;  x++ ) {
					if( pRechts->bModuloKonturen ) {
						pColorConvert[x] = (BYTE)( startcol+( x%pRechts->uModulo )*maxrechtscol+.5 );
					}
#ifdef SIMPLE_CONTOUR
					else if( pRechts->bKonturen  &&  x%pRechts->uKontur <= pRechts->uKonturToleranz ) {     // Konturen
						pColorConvert[x] = 3;
					}
#endif
					else if( x >= maxrechts ) {
						pColorConvert[x] = endcol;
					}
					else if( x <= lLowlim ) {
						pColorConvert[x] = startcol;
					}
					else {
						pColorConvert[x] = (BYTE)( startcol + ( ( (long)( x-lLowlim ) )<<10l )/lScaleFactor );
					}
				}
				for( y = 0;  y < pSnom->h;  y++ ) {
					for( x = 0;  x < pSnom->w;  x++ ) {
						pDest[x] = pColorConvert[ pSrc[x] ];
					}
					pDest += ww;
					pSrc -= pSnom->w;
				}
			}
			else {
				for( y = 0;  y < pSnom->h;  y++ ) {
					for( x = 0;  x < pSnom->w;  x++ ) {
						if( pBmp->pMaske  &&  ( ( x+y )%2 ) == 0  &&  IsMaske( pBmp, x, y ) ) {
							pDest[x] = 4;
						}
#ifdef SIMPLE_CONTOUR
						else if( pRechts->bKonturen  &&  pSrc[x]%pRechts->uKontur <= pRechts->uKonturToleranz ) { // Konturen
							pDest[x] = 3;
						}
#endif
						else if( pRechts->bModuloKonturen ) {
							pDest[x] = (BYTE)( startcol+( pSrc[x]%pRechts->uModulo )*maxrechtscol+.5 );
						}
						else if( pSrc[x] >= maxrechts ) {
							pDest[x] = endcol;
						}
						else {
							pDest[x] = (BYTE)( startcol + ( ( (long)( pSrc[x]-lLowlim ) )<<10l )/lScaleFactor );
						}
					}
					pDest += ww;
					pSrc -= pSnom->w;
				}
			}
#ifndef SIMPLE_CONTOUR
			// Best contours are overlaid now ...
			if( pRechts->uKontur ) {
				UWORD last_contour, diff = pRechts->uKontur;
				pSrc = Daten;
				pDest = pBits+pBmp->rectRechts.left+( h-pBmp->rectRechts.top-pSnom->h )*ww;

				// Zuerst Zeilenweise
				for( y = 0; y < pSnom->h; y++ )	{
					last_contour = ( pSrc[y*pSnom->w] )/diff;
					for( x = 1;  x < pSnom->w-1;  x++ ) {
						if( pSrc[y*pSnom->w+x]/diff != last_contour ) {
							last_contour = ( pSrc[y*pSnom->w+x]/diff );
							pDest[( pSnom->h-1-y )*ww+x] = 3; // set contour colour
						}
					}
				}
				// Dann Spaltenweise
				for( x = 0;  x < pSnom->w;  x++ ) {
					last_contour = ( pSrc[x] )/diff;
					for( y = 1;  y < pSnom->h;  y++ ) {
						if( pSrc[y*pSnom->w+x]/diff != last_contour ) {
							last_contour = ( pSrc[y*pSnom->w+x]/diff );
							pDest[( pSnom->h-1-y )*ww+x] = 3; // set contour colour
						}
					}
				}
			}
#endif
		}
		else {
			// 3D-Bitmap erstellen
			Draw3D( pBits, pBmp->rectRechts.left, pBmp->rectRechts.top, ww, pCacheDib->bmiHeader.biHeight,
			        Daten, pSnom->w, pSnom->h, 0, maxrechts,
			        maxrechtscol, startcol, pRechts );
		}
	}

	if( !IsBadHugeWritePtr( pBmp->pCacheBits, 4 ) ) {
		MemFree( pBmp->pCacheBits );
	}
	pBmp->pCacheBits = pBits;
	pBmp->IsDirty = FALSE;  // Cache is Ok!
	return ( TRUE );
}
// 3.8.97


// The ONLY way to copy into Metafile!!!
void CopyDibToDC( HDC hDC, LPBITMAPINFO pDib, LPBYTE pDest, RECT xywh )
{
	HDC hMem;
	HGDIOBJ	hBitmap, hOldBitmap;

	hBitmap = CreateDIBitmap( hDC, &( pDib->bmiHeader ), CBM_INIT, pDest, pDib, DIB_RGB_COLORS );
	hMem = CreateCompatibleDC( hDC );
	SetMapMode( hMem, MM_TEXT );
	hOldBitmap = SelectObject( hMem, hBitmap );
	if( xywh.bottom < 0 ) {
		StretchBlt( hDC, xywh.left, xywh.top, xywh.right, xywh.bottom, hMem, 0, pDib->bmiHeader.biHeight-1, pDib->bmiHeader.biWidth, 1-pDib->bmiHeader.biHeight, SRCCOPY );
	}
	else {
		StretchBlt( hDC, xywh.left, xywh.top, xywh.right, xywh.bottom, hMem, 0, 0, pDib->bmiHeader.biWidth, pDib->bmiHeader.biHeight, SRCCOPY );
	}
	SelectObject( hMem, hOldBitmap );
	DeleteObject( hBitmap );
	DeleteDC( hMem );
}
// 4.10.2001


//****	2D-Skalierung in Bitmap
// returns the width in Device coords
int Draw2DAxisDC( HDC hDC, LPBILD pBild, LPBITMAPINFO pDib, RECT xywh )
{
	BYTE pDest[256*4];      // maximum 256 colors
	LONG endcol;
	LONG y, iWidth;

	ASSERT(  !pBild->bPseudo3D  &&  pBild != NULL  &&  pDib != NULL  );

	xywh.right = ( xywh.right-xywh.left )/32; // width of legend
	xywh.bottom -= xywh.top;

	endcol = pBild->uMaxDaten;
	if( endcol > 255 ) {
		endcol = 254;
	}
	SetDibPaletteColors( pDib, (COLORREF HUGE*)( pBild->Farben ), pBild, 0, endcol, 0, endcol );
	if( !pBild->bShowNoZ ) {
		// für die Legende den Graustreifen ...
		for( y = 0;  y < endcol;  y++ )	{
#if 0   // Emulating contrast make only sense when altering bitmap palette!
			// -1 bei y, da die Bitmap nur bis pSnom->h-1 lï¿½uft (for y<h!)
			// EmulContrast Resize only color
			if( !DontEmulContrast )	{
				if( y <= pBild->fStart*endcol/100.0 ) {
					pDest[y*4] = 0;
				}
				else if( y >= endcol*( pBild->fEnde-pBild->fStart )/100.0 ) {
					pDest[y*4] = endcol;
				}
				else {
					pDest[y*4] = (BYTE)( pBild->fStart*endcol/100.0 + ( y*endcol )*( pBild->fEnde-pBild->fStart )/100.0 );
				}
			}
			else
#endif
			pDest[y*4] = (BYTE)endcol-y-1;
		}
	}
	pDib->bmiHeader.biWidth = 1;
	pDib->bmiHeader.biHeight = endcol;
	SetStretchBltMode( hDC, COLORONCOLOR );
	CopyDibToDC( hDC, pDib, pDest, xywh );
	// Skalierung z-Achse
	if( !pBild->bShowNoZ ) {
		if( pBild->bSpecialZUnit ) {
			iWidth = DrawVerticalAxis( hDC, xywh.left+xywh.right, xywh.top, xywh.bottom, 2, 10, pBild->uMaxDaten*pBild->fSkal*pBild->fEnde/100.0, 0.0, pBild->strZUnit, NULL );
		}
		else {
			iWidth = DrawVerticalAxis( hDC, xywh.left+xywh.right, xywh.top, xywh.bottom, 2, 10, pBild->uMaxDaten*pBild->fSkal*pBild->fEnde/100.0, 0.0, STR_TOPO_UNIT, STR_TOPO_SUNIT );
		}
	}
	return ( iWidth+xywh.right );
}
// 1.10.2001


// Draws a Bitmap in DC
int DrawBildDC( HDC hDC, LPBMPDATA pBmp, LPBILD pBild, LPBITMAPINFO pDib, RECT xywh )
{
	HGDIOBJ	hOldFont;
	RECT dibXywh;

	LPSNOMDATA pSnom;
	LONG ww, hh, x, y, yy;
	LPBYTE pDest;
	BYTE pColorConvert[65536];
	LPUWORD pSrc, pOffset;
	double ScaleFactor;     // =1024/maxlinkscol bzw. 1024/maxrechtscol
	long endcol, lowlim, maxlim;

	// we want only width
	xywh.right -= xywh.left;
	xywh.bottom -= xywh.top;

	pSnom = &( pBmp->pSnom[pBmp->iAktuell] );
	// Evt. sind die Daten nur ein Pointer
	pSrc = pBild->puDaten;
	if( (LONG)pSrc <= 256l ) {
		switch( pBild->Typ ) {
			case TOPO:
				pSrc = pBmp->pSnom[(LONG)pSrc-1].Topo.puDaten;
				break;

			case ERRO:
				pSrc = pBmp->pSnom[(LONG)pSrc-1].Error.puDaten;
				break;

			case LUMI:
				pSrc = pBmp->pSnom[(LONG)pSrc-1].Lumi.puDaten;
				break;
		}
	}

	// Calculate size of the DibData in Pixel
	CalcDibSize( hDC, pSnom->w, pSnom->h, &dibXywh, pBild->bPseudo3D, FALSE );
	ww = ( ( dibXywh.right+3 )/4 )*4;
	hh = dibXywh.bottom;
	pDest = pMalloc( ww*hh );
	if( pDest == NULL ) {
		return ( 0 );
	}

	// Next is title
	SetTextAlign( hDC, TA_LEFT|TA_BOTTOM );
	lf.lfHeight *= 2;
	hOldFont = SelectObject( hDC, CreateFontIndirect( &lf ) );
	lf.lfHeight /= 2;
	TextOut( hDC, xywh.left, xywh.top, pBild->strTitel, lstrlen( pBild->strTitel ) );
	DeleteObject( SelectObject( hDC, hOldFont ) );

	// Init for copy
	endcol = pBild->uMaxDaten;
	if( endcol > 255 ) {
		endcol = 255;
	}
	lowlim = pBild->fStart*pBild->uMaxDaten/100.0;
	maxlim = lowlim + pBild->fEnde*(double)pBild->uMaxDaten/100.0;
	ScaleFactor = endcol/(double)( maxlim-lowlim ); // max 253 colors ...
	SetDibPaletteColors( pDib, (COLORREF HUGE*)( pBild->Farben ), pBild, 0, endcol, 0, endcol );

	if( pBild->bPseudo3D ) {
		// 3D-Bitmap erstellen
		Draw3D( pDest, 0, 0, ww, hh,
		        pSrc, pSnom->w, pSnom->h, 0, maxlim,
		        endcol, 0, pBild );
//		pDib->bmiHeader.biWidth = ww-1;
//		pDib->bmiHeader.biHeight = hh;
		CopyDibToDC( hDC, pDib, pDest, xywh );
		MemFree( pDest );
		Draw3DSkala( hDC, xywh.left, xywh.top, pBmp, pSnom, pBild, pBild->uMaxDaten );
		return ( xywh.right );
	}

	// Here only if 2D!
	// Table for faster color conversion
	for( x = 0;  x < pBild->uMaxDaten;  x++ ) {
		if( pBild->bKonturen  &&  x%pBild->uKontur <= pBild->uKonturToleranz ) { // Konturen
			pColorConvert[x] = 255;
		}
		else if( pBild->bModuloKonturen ) {
			pColorConvert[x] = (BYTE)( ( x%pBild->uModulo )*endcol+.5 );
		}
		else if( x >= maxlim ) {
			pColorConvert[x] = endcol;
		}
		else if( x <= lowlim ) {
			pColorConvert[x] = 0;
		}
		else {
			pColorConvert[x] = (BYTE)( ( (long)x-lowlim )*ScaleFactor ); //-10???
		}
	}

	ww = ( pSnom->w+3 )&0x7FFFFFFCl;
	pOffset = pSrc+pSnom->w*( pSnom->h-1 );
	// Convert data
	for( y = 0, yy = 0 /*(pSnom->h-1)*ww*/;  y < pSnom->h;  y++ ) {
		for( x = 0;  x < pSnom->w;  x++ ) {
			pDest[x+yy] = pColorConvert[ pOffset[x] ];
		}
		pOffset -= pSnom->w;
		yy += ww;
	}

	// finally draw it!
	pDib->bmiHeader.biWidth = pSnom->w;
	pDib->bmiHeader.biHeight = pSnom->h;
	CopyDibToDC( hDC, pDib, pDest, xywh );
	MemFree( pDest );
	// and Axis
	DrawHorizontalAxis( hDC, xywh.left, xywh.top+xywh.bottom, xywh.right, 3, 5, pSnom->w*pSnom->fX, pBmp->pPsi.cShowOffset*pSnom->fXOff, STR_X_UNIT, STR_X_SUNIT );
	return ( xywh.right );
}
// 1.10.2001


typedef struct {
	DWORD ident;           // This contains GDICOMMENT_IDENTIFIER.
	DWORD iComment;        // This contains GDICOMMENT_BEGINGROUP.
	RECTL reclOutput;       // This is the bounding rectangle for the
	// object in logical coordinates.
	DWORD nDescription;    // This is the number of characters in the
	// optional Unicode description string that
	// follows. This is 0 if there is no
	// description string.
} BG_TYPE;

typedef struct {
	DWORD ident;         // This contains GDICOMMENT_IDENTIFIER.
	DWORD iComment;      // This contains GDICOMMENT_ENDGROUP.
} EG_TYPE;

#ifndef GDICOMMENT_IDENTIFIER
#define GDICOMMENT_IDENTIFIER           0x43494447
#define GDICOMMENT_WINDOWS_METAFILE     0x80000001
#define GDICOMMENT_BEGINGROUP           0x00000002
#define GDICOMMENT_ENDGROUP             0x00000003
#define GDICOMMENT_MULTIFORMATS         0x40000004
#define EPS_SIGNATURE                   0x46535045
#endif

// Zeichnet die Bitmap neu in den DC (fuer Copy u.ae.)
BOOL DrawInDC( HDC hDC, LPBMPDATA pBmp, BOOL UsedOwnPixel, BOOL bIgnoreRect, double fZoom, RECT *xywh )
{
	LPSNOMDATA pSnom;
	LPBITMAPINFO pDib;
	LPBILD pLinks;
	LPBILD pRechts;
	SIZE size;
	int iAkt = pBmp->iAktuell, iDist, iSave, iOldLeft, iOldTop;
	HGDIOBJ	hOldPen;
	BG_TYPE	bg = {GDICOMMENT_IDENTIFIER, GDICOMMENT_BEGINGROUP, {0, 0, 0, 0}, 0};
	EG_TYPE	eg = {GDICOMMENT_IDENTIFIER, GDICOMMENT_ENDGROUP};

	// Temporaere bitmap init
	pDib = (LPBITMAPINFO )pMalloc( sizeof( BITMAPINFO )+256*sizeof( RGBQUAD ) );
	if( pDib == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( -1 );
	}
	pDib->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	pDib->bmiHeader.biPlanes = 1;
	pDib->bmiHeader.biBitCount = 8;
	pDib->bmiHeader.biCompression = BI_RGB;
	pDib->bmiHeader.biXPelsPerMeter = 0;
	pDib->bmiHeader.biYPelsPerMeter = 0;
	pDib->bmiHeader.biSizeImage = 0;
	pDib->bmiHeader.biClrUsed =
	        pDib->bmiHeader.biClrImportant = 0;

	// Zuerst Pointer fï¿½r rechte und linke Seite holen
	pSnom = &( pBmp->pSnom[iAkt] );
	pLinks = NULL;
	switch( pBmp->Links ) {
		case TOPO:
			pLinks = &( pBmp->pSnom[iAkt].Topo );
			break;

		case ERRO:
			pLinks = &( pBmp->pSnom[iAkt].Error );
			break;

		case LUMI:
			pLinks = &( pBmp->pSnom[iAkt].Lumi );
			break;
	}
	if( pLinks  &&  pLinks->Typ == NONE ) {
		pLinks = NULL;
	}
	pRechts = NULL;
	switch( pBmp->Rechts ) {
		case TOPO:
			pRechts = &( pBmp->pSnom[iAkt].Topo );
			break;

		case ERRO:
			pRechts = &( pBmp->pSnom[iAkt].Error );
			break;

		case LUMI:
			pRechts = &( pBmp->pSnom[iAkt].Lumi );
			break;
	}
	if( pRechts  &&  pRechts->Typ == NONE ) {
		pRechts = NULL;
	}
	// Nichts zu tun?
	if( pLinks == NULL  &&  pRechts == NULL ) {
		return ( FALSE );
	}

	// Die kopierten Bitmaps haben immer je 256 eigenen Farben, was das ganze recht einfach macht ...

	// Farbe 0 = Hintergrund: cHinten
	// Farbe 1 = Schwarz: 0
	// Farbe 2 = Vordergrund: cVorn
	// Farbe 3 = Markierung: cMarkierungLinks
	// Farbe 4 = !Markierung: cMarkierungRechts

	//****  Ab hier werden die Masze berechet
	SetBkMode( hDC, TRANSPARENT );
	SetTextColor( hDC, (COLORREF)cVorn );   // Black
	hOldPen = SelectObject( hDC, CreatePen( PS_SOLID, 0, (COLORREF)cHinten ) );
	MoveToEx( hDC, 0, 0, NULL );
	LineTo( hDC, 0, 1 );
	DeleteObject( hOldPen );
	hOldPen = SelectObject( hDC, CreatePen( PS_SOLID, 0, (COLORREF)cVorn ) );

	// Calculate staring position

	// first: y-Offset is height of title
	iSave = SetMapMode( hDC, MM_TEXT );
	SetTextAlign( hDC, TA_LEFT|TA_TOP );
	SelectObject( hDC, CreateFontIndirect( &lf ) );
	SetMapMode( hDC, iSave );
	GetTextExtentPoint( hDC, "Eg|", 3, &size );

	// Then x-offset (target image width and height is already given)
	if( bIgnoreRect ) {
		// xywh is not initialized ...
		xywh->left = 0;
		xywh->top = 0;
#ifdef _MSC_VER
		pSnom = &( pBmp->pSnom[iAkt] ); // MSVC braucht diese Zeile! Compilerfehler!!!
#endif
		xywh->right = pSnom->w*fZoom;
		xywh->bottom = pSnom->h*fZoom;
		DPtoLP( hDC, (LPPOINT)xywh, 2 );
	}
	iOldLeft = xywh->left*fZoom;
	iOldTop = xywh->top*fZoom;
	xywh->top += 2*size.cy;         // height of title
	xywh->bottom += 2*size.cy;              // height of title
	xywh->left += size.cx;
	xywh->right += size.cx;
	pBmp->rectPlot.right = xywh->right-xywh->left;
	pBmp->rectPlot.bottom = xywh->bottom-xywh->top;

	// Now drawing the Bitmaps
	if( pLinks ) {
		if( !pLinks->bPseudo3D ) { // draw vertical Axis and advance
			bg.reclOutput.left = xywh->left;
			bg.reclOutput.top = 0;
			bg.reclOutput.right = ( xywh->right-xywh->left )+size.cx;
			bg.reclOutput.bottom = xywh->bottom+( size.cy*3 );
			GdiComment( hDC, sizeof( bg ), &bg );
			iSave = SaveDC( hDC );
			iDist = Draw2DAxisDC( hDC, pLinks, pDib, *xywh )+size.cx;
			RestoreDC( hDC, iSave );
			GdiComment( hDC, sizeof( eg ), (const BYTE*)&eg );
			xywh->left += iDist;
			xywh->right += iDist;
		}

		// Now draw left Bitmap and advance
		bg.reclOutput.left = xywh->left;
		bg.reclOutput.top = 0;
		bg.reclOutput.right = ( xywh->right-xywh->left )+size.cx;
		bg.reclOutput.bottom = xywh->bottom+( size.cy*3 );
		GdiComment( hDC, sizeof( bg ), &bg );
		iDist = DrawBildDC( hDC, pBmp, pLinks, pDib, *xywh )+size.cx;
		GdiComment( hDC, sizeof( eg ), &eg );
		pBmp->rectLinks.left = xywh->left;
		pBmp->rectLinks.top = xywh->top;
		pBmp->rectLinks.bottom = xywh->bottom;
		pBmp->rectLinks.right = xywh->right;
		xywh->left += iDist;
		xywh->right += iDist;
		pBmp->rectPlot.left = xywh->left;
		pBmp->rectPlot.top = xywh->top;
		pBmp->rectPlot.bottom = xywh->bottom;
		pBmp->rectPlot.right = xywh->right;
	}

	// Now drawing the Bitmaps
	if( pRechts ) {
		if( !pRechts->bPseudo3D ) { // draw vertical Axis and advance
			bg.reclOutput.left = xywh->left;
			bg.reclOutput.top = 0;
			bg.reclOutput.right = ( xywh->right-xywh->left )+size.cx;
			bg.reclOutput.bottom = xywh->bottom+( size.cy*3 );
			GdiComment( hDC, sizeof( bg ), &bg );
			iDist = Draw2DAxisDC( hDC, pRechts, pDib, *xywh )+size.cx;
			GdiComment( hDC, sizeof( eg ), &eg );
			xywh->left += iDist;
			xywh->right += iDist;
		}

		// Now draw left Bitmap and advance
		bg.reclOutput.left = xywh->left;
		bg.reclOutput.top = 0;
		bg.reclOutput.right = ( xywh->right-xywh->left )+size.cx;
		bg.reclOutput.bottom = xywh->bottom+( size.cy*3 );
		GdiComment( hDC, sizeof( bg ), &bg );
		iDist = DrawBildDC( hDC, pBmp, pRechts, pDib, *xywh )+size.cx;
		GdiComment( hDC, sizeof( eg ), &eg );
		pBmp->rectRechts.left = xywh->left;
		pBmp->rectRechts.top = xywh->top;
		pBmp->rectRechts.bottom = xywh->bottom;
		pBmp->rectRechts.right = xywh->right;
		xywh->left += iDist;
		xywh->right += iDist;
		pBmp->rectPlot.left = xywh->left;
		pBmp->rectPlot.top = xywh->top;
		pBmp->rectPlot.bottom = xywh->bottom;
		pBmp->rectPlot.right = xywh->right;
	}
	xywh->left = iOldLeft;
	xywh->top = iOldTop;
	xywh->right -= iDist;
	MemFree( pDib );
	return ( TRUE );
}
// 3.8.97


/*****************************************************************************************/
// Folgende Routine malt in "beliebiger" Breite gestrichchelte oder gepunktete Linien

typedef struct {
	HDC hDC;
	BOOL bInit;
	int iModus;
	int iWidth;
	int iCount;
} DDA_DATA, HUGE*LP_DDA_DATA;

void CALLBACK DrawLine( int x, int y, DDA_DATA HUGE *pDData )
{
	int i, lim;

	switch( pDData->iModus ) {
		// Durchgezogen, geht anders besser ...
		case 0:
			if( pDData->bInit ) {
				LineTo( pDData->hDC, x, y );
			}
			break;

		// Gepunktet
		case 1:
			lim = 5*pDData->iWidth;
			if( ++pDData->iCount > lim ) { //abs(x-pDData->iLastX)+abs(y-pDData->iLastY)>lim  )
				Rectangle( pDData->hDC, x-1, y-1, x+1, y+1 );
				pDData->iCount = 0;
			}
			break;

		// Gestrichelt
		case 2:
			i = ++pDData->iCount; // abs(x-pDData->iLastX)+abs(y-pDData->iLastY);
			lim = 8*pDData->iWidth;
			if( pDData->bInit  &&  i <= lim ) {
				LineTo(  pDData->hDC, x, y );
			}
			else if( i == lim ) {
				LineTo( pDData->hDC, x, y );
			}
			else if( i >= 2*lim ) {
				MoveToEx( pDData->hDC, x, y, NULL );
				pDData->iCount = 0;
			}
			pDData->bInit = FALSE;
			break;

		// Gestrichpunktet
		case 3:
			i = ++pDData->iCount; // abs(x-pDData->iLastX)+abs(y-pDData->iLastY);
			lim = 8*pDData->iWidth;
			if( pDData->bInit  &&  i <= lim ) {
				LineTo(  pDData->hDC, x, y );
			}
			else if( i == lim ) {
				LineTo( pDData->hDC, x, y );
			}
			else if( i == lim+5 ) {
				Rectangle( pDData->hDC, x-1, y-1, x+1, y+1 );
				MoveToEx( pDData->hDC, x, y, NULL );
			}
			else if( i >= lim+10 ) {
				MoveToEx( pDData->hDC, x, y, NULL );
				pDData->iCount = 0;
			}
			pDData->bInit = FALSE;
			break;
	}
}


/*****************************************************************************************/


// Malt die Markierungslinie neu (in Topografie UND Lumineszens, wenn vorhanden)
void DrawScanLine( HDC hdc, LPBMPDATA pBmp, double fZoom )
{
	RECT rScan;
	HGDIOBJ	hOld;
	DDA_DATA DData;
	int i;
	int x1, y1, x2, y2;

	if( pBmp->lMaxScan <= 0 ) {
		return;
	}

	DData.iWidth = 1;
	if( fZoom < 1.44 ) {
		DData.iWidth = (int)( 1.44/fZoom );
	}

	if( pBmp->rectLinks.left  &&
	    ( ( pBmp->Links == TOPO  &&  !pBmp->pSnom[pBmp->iAktuell].Topo.bPseudo3D )  ||
	      ( pBmp->Links == ERRO  &&  !pBmp->pSnom[pBmp->iAktuell].Error.bPseudo3D ) ||
	      ( pBmp->Links == LUMI  &&  !pBmp->pSnom[pBmp->iAktuell].Lumi.bPseudo3D ) ) ) {
		hOld = SelectObject( hdc, CreatePen( PS_SOLID, DData.iWidth, cMarkierungLinks ) );
		for( i = 0;  i < pBmp->lMaxScan;  i++ )	{
			MemMove( &rScan, pBmp->rectScan+i, sizeof( RECT ) );
			DPtoLP( hdc, &rScan, 2 );       // to adjust for Mapmodes unequal to MM_TEXT
			x1 = (int)( ( pBmp->rectLinks.left+rScan.left*( pBmp->rectLinks.right-pBmp->rectLinks.left )/pBmp->pSnom[pBmp->iAktuell].w )/fZoom );
			y1 = (int)( ( pBmp->rectLinks.top+rScan.top*( pBmp->rectLinks.bottom-pBmp->rectLinks.top )/pBmp->pSnom[pBmp->iAktuell].h )/fZoom );
			x2 = (int)( ( pBmp->rectLinks.left+rScan.right*( pBmp->rectLinks.right-pBmp->rectLinks.left )/pBmp->pSnom[pBmp->iAktuell].w )/fZoom );
			y2 = (int)( ( pBmp->rectLinks.top+rScan.bottom*( pBmp->rectLinks.bottom-pBmp->rectLinks.top )/pBmp->pSnom[pBmp->iAktuell].h )/fZoom );
			if( x1 == x2  &&  y1 == y2 ) {
				continue;       // Nix zu zeichen!
			}
			DData.iModus = i;
			DData.hDC = hdc;
			DData.bInit = TRUE;
			DData.iCount = 0;

			MoveToEx( hdc, x1, y1, NULL );
			if( i == 0 ) {
				LineTo( hdc, x2, y2 );
			}
			else {
				LineDDA( x1, y1, x2, y2, (LINEDDAPROC)DrawLine, (LPARAM)(LP_DDA_DATA)&DData );
			}

			MoveToEx( hdc, x1, y1-10, NULL );
			LineTo( hdc, x1, y1+10 );

			MoveToEx( hdc, x1-10, y1, NULL );
			LineTo( hdc, x1+10, y1 );
		}
		DeleteObject( SelectObject( hdc, hOld ) );
	}

	if( pBmp->rectRechts.left  &&
	    ( ( pBmp->Rechts == TOPO  &&  !pBmp->pSnom[pBmp->iAktuell].Topo.bPseudo3D )  ||
	      ( pBmp->Rechts == ERRO  &&  !pBmp->pSnom[pBmp->iAktuell].Error.bPseudo3D ) ||
	      ( pBmp->Rechts == LUMI  &&  !pBmp->pSnom[pBmp->iAktuell].Lumi.bPseudo3D ) ) ) {
		hOld = SelectObject( hdc, CreatePen( PS_SOLID, DData.iWidth, cMarkierungRechts ) );
		for( i = 0;  i < pBmp->lMaxScan;  i++ )	{
			MemMove( &rScan, pBmp->rectScan+i, sizeof( RECT ) );
			DPtoLP( hdc, &rScan, 2 );       // to adjust for Mapmodes unequal to MM_TEXT
			x1 = (int)( ( pBmp->rectRechts.left+rScan.left*( pBmp->rectRechts.right-pBmp->rectRechts.left )/pBmp->pSnom[pBmp->iAktuell].w )/fZoom );
			y1 = (int)( ( pBmp->rectRechts.top+rScan.top*( pBmp->rectRechts.bottom-pBmp->rectRechts.top )/pBmp->pSnom[pBmp->iAktuell].h )/fZoom );
			x2 = (int)( ( pBmp->rectRechts.left+rScan.right*( pBmp->rectRechts.right-pBmp->rectRechts.left )/pBmp->pSnom[pBmp->iAktuell].w )/fZoom );
			y2 = (int)( ( pBmp->rectRechts.top+rScan.bottom*( pBmp->rectRechts.bottom-pBmp->rectRechts.top )/pBmp->pSnom[pBmp->iAktuell].h )/fZoom );
			if( x1 == x2  &&  y1 == y2 ) {
				continue;       // Nix zu zeichen!
			}
			DData.iModus = i;
			DData.hDC = hdc;
			DData.bInit = TRUE;
			DData.iCount = 0;

			MoveToEx( hdc, x1, y1, NULL );
			if( i == 0 ) {
				LineTo( hdc, x2, y2 );
			}
			else {
				LineDDA( x1, y1, x2, y2, (LINEDDAPROC)DrawLine, (LPARAM)(LP_DDA_DATA)&DData );
			}
		}
		DeleteObject( SelectObject( hdc, hOld ) );
	}
}
// 6.8.97


/***************************************************************************************/
// Kopiert die Hï¿½henwerte

/****	Kopiert die Daten einer Querschnittslinie nach puDest ****/
/**** Das sind maximal (w^2+h^2)^.5+1 Punkte ****/
/**** Wenn bNurX TRUE, gibt es als Ergebnis die X-Werte, ****
 **** wenn bNurY TRUE, gibt es nur die Y-Werte, sonst die Daten ****/
int BildGetLine( LPUWORD puDest, LPUWORD puDaten, LPRECT pRect, LONG w, LONG h, BOOLEAN bNurX, BOOLEAN bNurY )
{
	double m;
	int x, y, delta = 1;
	int iAnzahl = 0;

	if( pRect->left == pRect->right ) {
		// senkrechte Linie
		x = pRect->left;
		if( pRect->top > pRect->bottom ) {
			delta = -1;
		}
		for( y = pRect->top;  y != pRect->bottom;  y += delta )	{
			if( y >= 0  &&  y < h )	{
				if( bNurX ) {
					puDest[iAnzahl++] = (UWORD)x;
				}
				else if( bNurY ) {
					puDest[iAnzahl++] = (UWORD)y;
				}
				else {
					puDest[iAnzahl++] = puDaten[x+y*w];
				}
			}
		}
		return ( iAnzahl );
	}

	if( pRect->top == pRect->bottom ) {
		// X ist Zï¿½hlvariable
		if( pRect->left > pRect->right ) {
			delta = -1;
		}
		y = pRect->top;
		for( x = pRect->left;  x != pRect->right;  x += delta )	{
			if( x >= 0  &&  x <= w ) {
				if( bNurX ) {
					puDest[iAnzahl++] = (UWORD)x;
				}
				else if( bNurY ) {
					puDest[iAnzahl++] = (UWORD)y;
				}
				else {
					puDest[iAnzahl++] = puDaten[x+y*w];
				}
			}
		}
		return ( iAnzahl );
	}

	m =  (double)( pRect->left-pRect->right )/ (double)( pRect->top-pRect->bottom );
	if( fabs( m ) < 1.0 ) {
		// Y ist Zï¿½hlvariable
		if( pRect->top > pRect->bottom ) {
			delta = -1;
		}
		for( y = pRect->top;  y != pRect->bottom;  y += delta )	{
			x = pRect->left+(int)( ( y-pRect->top )*m+0.5 );
			if( y >= 0  &&  y < h  &&  x >= 0  &&  x <= w )	{
				if( bNurX ) {
					puDest[iAnzahl++] = (UWORD)x;
				}
				else if( bNurY ) {
					puDest[iAnzahl++] = (UWORD)y;
				}
				else {
					puDest[iAnzahl++] = puDaten[x+y*w];
				}
			}
		}
		return ( iAnzahl );
	}
	else {
		// X ist Zï¿½hlvariable
		if( pRect->left > pRect->right ) {
			delta = -1;
		}
		for( x = pRect->left;  x != pRect->right;  x += delta )	{
			y = pRect->top+(int)( ( x-pRect->left )/m+0.5 );
			if( y >= 0  &&  y < h  &&  x >= 0  &&  x <= w )	{
				if( bNurX ) {
					puDest[iAnzahl++] = (UWORD)x;
				}
				else if( bNurY ) {
					puDest[iAnzahl++] = (UWORD)y;
				}
				else {
					puDest[iAnzahl++] = puDaten[x+y*w];
				}
			}
		}
		return ( iAnzahl );
	}
}
// 18.2.98


// draws the line graph
void ScanLinePlot( HDC hdc, LPBMPDATA pBmp, LPBILD pBild, double fZoom, PROFILFLAGS wScanlineModus, LPUWORD puTemp, LPFLOAT pfTemp[4], int iSize, BOOLEAN bNeuZeichen )
{
	LPSNOMDATA pSnom = &pBmp->pSnom[pBmp->iAktuell];
	SIZE size;
	HPEN hOld;
	LPUWORD	puDaten;
	int i, x, y, w, h, iPunkte[4], iMaxPunkte, iScan, iMaxScan=0, iLineWidth;
	float fWeite[MAX_SCANLINE], fMaxWeite, fOffset=0.0, fMaxHoehe, fMaxMax = -1e20, fMaxMin = 1e20;

	fMaxWeite = fMaxHoehe = 0.0;
	iMaxPunkte = 0;

	iLineWidth = 1;
	if( fZoom < 1.44 ) {
		iLineWidth = (int)( 1.44/fZoom );
	}

	puDaten = GetDataPointer( pBmp, pBild->Typ );
	if(  !pBmp->bCountDots  ||  pBmp->dot_number<=1  ) {
		// scanline mode
		iMaxScan = 0;
		for( iScan = 0;  iScan < pBmp->lMaxScan;  iScan++ ) {
			iPunkte[iScan] = BildGetLine( puTemp, puDaten, &( pBmp->rectScan[iScan] ), pSnom->w, pSnom->h, FALSE, FALSE );
			if( iPunkte[iScan] == 0 ) {
				continue;
			}
			iMaxScan ++;
			pBmp->bIsScanLine = TRUE;
			for( i = 0;  i < iPunkte[iScan];  i++ ) {
				pfTemp[iScan][i] = puTemp[i]*pBild->fSkal;
			}

			// Evt. nicht nur die Daten darstellen
			wScanlineModus &= ~( P_DIST|P_X|P_Y );
			switch( wScanlineModus ) {
				case P_AUTOKORRELATION:
					Autokorrelation( pfTemp[iScan], pfTemp[iScan], iPunkte[iScan], FALSE );
					break;

				case P_PSD:
				{
					double fMittel = 0.0;

					for( i = 0;  i < iPunkte[iScan];  i++ ) {
						fMittel += pfTemp[iScan][i];
					}
					fMittel /= (float)iPunkte[iScan];
					for( i = 0;  i < iPunkte[iScan];  i++ ) {
						pfTemp[iScan][i] -= fMittel;
					}
					for(  ;  i < iSize;  i++ ) {
						pfTemp[iScan][i] = 0.0;
					}
					realft( pfTemp[iScan]-1, iSize/2, 1 );
					for( i = 0;  i < iPunkte[iScan];  i++ ) {
						pfTemp[iScan][i] = log( pfTemp[iScan][i] * pfTemp[iScan][i] );
					}
					pfTemp[iScan][0] = pfTemp[iScan][1];
					break;
				}
			}

			if( iPunkte[iScan] > iMaxPunkte ) {
				iMaxPunkte = iPunkte[iScan];
			}

			// Weite in x-Einheiten berechnen
			fWeite[iScan] = sqrt( pow( pSnom->fX*( pBmp->rectScan[iScan].left-pBmp->rectScan[iScan].right ), 2 ) + pow( pSnom->fY*( pBmp->rectScan[iScan].top-pBmp->rectScan[iScan].bottom ), 2 ) );
			if( fWeite[iScan] > fMaxWeite ) {
				fMaxWeite = fWeite[iScan];
			}

			// Maximum und Minimum finden
			for( i = 0;  i < iPunkte[iScan];  i++ )	{
				if( pfTemp[iScan][i] < fMaxMin ) {
					fMaxMin = pfTemp[iScan][i];
				}
				if( pfTemp[iScan][i] > fMaxMax ) {
					fMaxMax = pfTemp[iScan][i];
				}
			}
			if( ( fMaxMax-fMaxMin ) > fMaxHoehe ) {
				fMaxHoehe = fMaxMax-fMaxMin;
			}
		}
		if(  iMaxPunkte==1  ) {
			return;
		}
	}
	// dot mode
	else {
		// histogram => first get x_scale
		UWORD min_h = 65535;
		UWORD max_h = 0, threshold;
		double fThreshold;

		for(  i=0;  i<pBmp->dot_number;  i++  ) {
			min_h = min( min_h, pBmp->dot_histogramm[i].hgt );
			max_h = max( max_h, pBmp->dot_histogramm[i].hgt );
		}
		max_h -= min_h;
		if(  max_h==min_h  ||  max_h==0) {
			return;
		}
		fOffset = min_h * pBild->fSkal;
		fWeite[0] = fMaxWeite = max_h * pBild->fSkal;
		// how many bins
		iMaxPunkte = iPunkte[0] = min( pBmp->dot_number-1, max( 10, pBmp->dot_number/10 ) );
		for(  i=0;  i<pBmp->dot_number;  i++  ) {
			pfTemp[0][ ( (pBmp->dot_histogramm[i].hgt-min_h)*iPunkte[0]) / max_h ] += 1.0;
		}

		fMaxMin = 0.0;
		fMaxHoehe = 0.0;
		for(  i=0;  i<iPunkte[0];  i++  ) {
			puTemp[i] = i;
			fMaxHoehe = max( fMaxHoehe, pfTemp[0][i] );
		}
		iMaxScan = 1;
	}

	// Zeichenmodus setzen
	SetROP2( hdc, R2_COPYPEN );
	SetBkMode( hdc, TRANSPARENT );

	// Ausmaï¿½e feststellen
	x = (int)( pBmp->rectPlot.left/fZoom );
	y = (int)( pBmp->rectPlot.top/fZoom );
	w = (int)( ( pBmp->rectPlot.right-pBmp->rectPlot.left )/fZoom );
	h = (int)( ( pBmp->rectPlot.bottom-pBmp->rectPlot.top )/fZoom );

	// Achsenkreuz zeichen
	if( bNeuZeichen ) {
		hOld = SelectObject( hdc, CreatePen( PS_SOLID, iLineWidth, RGB( 0, 0, 0 ) ) );

		// Umriss
		MoveToEx( hdc, x, y, NULL );
		LineTo( hdc, x, y+h );
		LineTo( hdc, x+w, y+h );
		LineTo( hdc, x+w, y );
		LineTo( hdc, x, y );

		// Achsen samt Skalierung zeichen
		DrawHorizontalAxis( hdc, x, y+h, w, 3, 5, fMaxWeite, fOffset, STR_X_UNIT, STR_X_SUNIT );
		if( pBild->bSpecialZUnit ) {
			DrawVerticalAxis( hdc, x+w, y+h, -h, 2, 10, fMaxHoehe, fMaxMin, pBild->strZUnit, NULL );
		}
		else {
			DrawVerticalAxis( hdc, x+w, y+h, -h, 2, 10, fMaxHoehe, fMaxMin, STR_TOPO_UNIT, STR_TOPO_SUNIT );
		}

		DeleteObject( SelectObject( hdc, hOld ) );

		pBmp->fScanBorder[0] = 0.0;
		pBmp->fScanBorder[1] = 0.0;
		pBmp->fScanBorder[2] = fMaxWeite;
		pBmp->fScanBorder[3] = fMaxHoehe;
	}

	// Nun endlich Graph zeichnen
	GetTextExtentPoint( hdc, "X", 1, &size );
	if( bNeuZeichen ) {
		hOld = SelectObject( hdc, CreatePen( PS_SOLID, iLineWidth, cMarkierungLinks ) );
	}
	else {
		hOld = SelectObject( hdc, CreatePen( PS_SOLID, iLineWidth, cMarkierungRechts ) );
	}
	for( iScan = 0;  iScan < iMaxScan;  iScan++ ) {
		// noch die Legende
		if( iScan == 0 ) {
			if( bNeuZeichen ) {
				if( h > 0 ) {
					i = y+size.cy*2;
				}
				else {
					i = y-size.cy*2;
				}
			}
			else {
				if( h > 0 ) {
					i = y+h-size.cy*2;
				}
				else {
					i = y+h+size.cy*2;
				}
			}
			MoveToEx( hdc, x+10, i, NULL );
			LineTo( hdc, x+w/10, i );
			SetTextAlign( hdc, TA_LEFT|TA_BOTTOM );
			TextOut( hdc, x+w/10, i+size.cy/2, pBild->strTitel, lstrlen( pBild->strTitel ) );
		}

		if( fMaxHoehe != 0.0 ) {
			if( iScan == 0 ) {
				double fiFaktor = (double)w/( (double)iMaxPunkte-1.0 )*fWeite[0]/fMaxWeite;
				// Durchgezogene Linie
				MoveToEx( hdc, x, (int)( y+h-( ( pfTemp[0][0]-fMaxMin )*h )/fMaxHoehe ), NULL );
				for( i = 1;  i < iPunkte[0];  i++ ) {
					LineTo( hdc, (int)( x+( i*fiFaktor+0.5 ) ), (int)( y+h-( ( pfTemp[0][i]-fMaxMin )*h )/fMaxHoehe ) );
				}
			}
			else {
				// Gepunktet, Gestrichelt oder Gestrichpunktet
				DDA_DATA DData;
				int x_now, y_now, x_last, y_last;
				double fiFaktor = (double)w/( (double)iMaxPunkte-1.0 )*fWeite[iScan]/fMaxWeite;

				x_last = (int)x;
				y_last = (int)( y+h-( ( pfTemp[iScan][0]-fMaxMin )*h )/fMaxHoehe );
				MoveToEx( hdc, x_last, y_last, NULL );

				DData.iModus = iScan;
				DData.hDC = hdc;
				DData.iWidth = iLineWidth;
				DData.iCount = 0;

				for( i = 1;  i < iPunkte[iScan];  i++ )	{
					x_now = (int)( x+( i*fiFaktor+0.5 ) );
					y_now = (int)( y+h-( ( pfTemp[iScan][i]-fMaxMin )*h )/fMaxHoehe );
					DData.bInit = TRUE;
					LineDDA( x_last, y_last, x_now, y_now, (LINEDDAPROC)DrawLine, (LPARAM)(LP_DDA_DATA)&DData );
					x_last = x_now;
					y_last = y_now;
				}
			}
		}       // if(  fMaxHohe!=0.0  )
	}
	DeleteObject( SelectObject( hdc, hOld ) );
}


// Scanline im "Koordinatensystem" zeichnen
void DrawScanLinePlot( HDC hdc, LPBMPDATA pBmp, double fScale, BOOLEAN bWhiteOut )
{
	HGDIOBJ	hOld;
	BOOLEAN	bRahmenZeichnen = TRUE;
	LPSNOMDATA pSnom = &pBmp->pSnom[pBmp->iAktuell];
	LPFLOAT	pfTemp[4];
	LPUWORD	puTemp;
	int i, j, iSize = 2;
	WORD wModus;                            // Was ist zu zeichnen

	// Damit immer alle gleich
	pBmp->bPlotUnten = PlotsUnten;

	// Erst mal inititalisieren fï¿½r den Fall: Keine Scanline
	pBmp->bIsScanLine = FALSE;
	// Ausmaï¿½e Fenster berechnen

//	pBmp->rectFenster.right = pBmp->pDib->bmiHeader.biWidth;
//	pBmp->rectFenster.bottom = pBmp->pDib->bmiHeader.biHeight;
	pBmp->rectFenster.right = pBmp->pDib->bmiHeader.biWidth;
	pBmp->rectFenster.bottom = pBmp->pDib->bmiHeader.biHeight;

	wModus = WhatToDo( pBmp, show );
	if( wModus == 0 ) {
		return;
	}

	if(  pBmp->bCountDots  ) {
		if(  pBmp->dot_number<=1  ) {
			return;	// no histogram etc.
		}
		puTemp = pMalloc( pBmp->dot_number*sizeof( UWORD ) );
		pfTemp[0] = pMalloc( pBmp->dot_number*sizeof( float ) );
		iSize = i = pBmp->dot_number;
	}
	else {
		for( j = i = 0;  i < pBmp->lMaxScan;  i++ ) {
			j += !( pBmp->rectScan[i].left == pBmp->rectScan[i].right  &&  pBmp->rectScan[i].top == pBmp->rectScan[i].bottom );
		}
		if( j == 0 ) {
			return; // Nichts zu zeichnen
		}
		// Array mit ausreichender Größe (die auch noch Potenz von 2 sein soll) belegen
		i = (int)sqrt( pSnom->w*pSnom->w+pSnom->h*pSnom->h )+20;
		while( i > iSize )
			iSize <<= 1;
		puTemp = pMalloc( iSize*sizeof( UWORD ) );
		for( i = 0;  i < pBmp->lMaxScan;  i++ ) {
			pfTemp[i] = pMalloc( iSize*sizeof( float ) );
		}
		if( puTemp == NULL  ||  pfTemp[i-1] == NULL ) {
			return;
		}
	}

	// Platz fï¿½r Scanline dazurechnen
	pBmp->rectFenster.right = 64+pBmp->rectPlot.right;      // Rechts Platz lassen
	if( pBmp->bPlotUnten ) {
		if( pBmp->rectLinks.top > 0 ) {
			pBmp->rectFenster.bottom = pBmp->rectPlot.bottom+pBmp->rectLinks.top;
		}
		else {
			pBmp->rectFenster.bottom = pBmp->rectPlot.bottom+pBmp->rectRechts.top;
		}
	}

	// Zeichensatz anpassen
	i = lf.lfHeight;
	j = SetMapMode( hdc, MM_TEXT ); // to calculate correctly the size
	lf.lfHeight /= fScale;
	hOld = SelectObject( hdc, CreateFontIndirect( &lf ) );  // Fï¿½r alle Rechnungen
	SetMapMode( hdc, j );
	lf.lfHeight = i;

	if( bWhiteOut )	{       // Rahmen loeschen
		BitBlt( hdc, (int)( pBmp->pDib->bmiHeader.biWidth/fScale ), 0, 8192, 8192, NULL, 0, 0, WHITENESS );
		BitBlt( hdc, 0, (int)( pBmp->pDib->bmiHeader.biHeight/fScale ), 8192, 8192, NULL, 0, 0, WHITENESS );
	}

	if( wModus&TOPO ) {
		ScanLinePlot( hdc, pBmp, &( pSnom->Topo ), fScale, wProfilShowFlags, puTemp, pfTemp, iSize, bRahmenZeichnen );
		bRahmenZeichnen = FALSE;
	}
	if(  !pBmp->bCountDots  ) {
		if( wModus&LUMI ) {
			ScanLinePlot( hdc, pBmp, &( pSnom->Lumi ), fScale, NONE, puTemp, pfTemp, iSize, bRahmenZeichnen );
			bRahmenZeichnen = FALSE;
		}
		if( wModus&ERRO ) {
			ScanLinePlot( hdc, pBmp, &( pSnom->Error ), fScale, NONE, puTemp, pfTemp, iSize, bRahmenZeichnen );
		}
	}
	MemFree( puTemp );
	if(  !pBmp->bCountDots  ) {
		for( i = 0;  i < pBmp->lMaxScan;  i++ ) {
			MemFree( pfTemp[i] );
		}
	}
	else {
		MemFree( pfTemp[0] );
	}
	DeleteObject( SelectObject( hdc, hOld ) );
	return;
}


// Quantenpunkte im "Koordinatensystem" zeichnen
void DrawDotsPlot( HDC hdc, LPBMPDATA pBmp, double fScale )
{
	HGDIOBJ	hOld;
	BOOLEAN	bRahmenZeichnen = TRUE;
	LPSNOMDATA pSnom = &pBmp->pSnom[pBmp->iAktuell];
	int i, j, iSize = 2;
	// zoom factor for crosses and radius
	const double fZoom = fScale!=1.0 ? fScale : (double)pBmp->pSnom[pBmp->iAktuell].h / (double)( pBmp->rectLinks.bottom-pBmp->rectLinks.top );
	const int endwidth = max( 2, 2/fZoom );

	if( pBmp->dot_histogramm_count == 0  ||  pSnom->Topo.bPseudo3D ) {
		return;
	}

	hOld = SelectObject( hdc, CreatePen( PS_SOLID, 1, cMarkierungLinks ) );
	for( j = i = 0;  i < pBmp->dot_number;  i++ ) {

		POINT pt, d;

		pt.x = (int)( ( pBmp->rectLinks.left+pBmp->dot_histogramm[i].x*( pBmp->rectLinks.right-pBmp->rectLinks.left )/pBmp->pSnom[pBmp->iAktuell].w )/fScale );
		pt.y = (int)( ( pBmp->rectLinks.top+pBmp->dot_histogramm[i].y*( pBmp->rectLinks.bottom-pBmp->rectLinks.top )/pBmp->pSnom[pBmp->iAktuell].h )/fScale );
		d.x = max(2,pBmp->dot_histogramm[i].radius_x)/fZoom;
		d.y = max(2,pBmp->dot_histogramm[i].radius_y)/fZoom;

		// make a cross
		MoveToEx( hdc, pt.x-d.x, pt.y, NULL );
		LineTo( hdc, pt.x+d.x, pt.y );
		MoveToEx( hdc, pt.x, pt.y-d.y, NULL );
		LineTo( hdc, pt.x, pt.y+d.y );
		// and mark the ends
		MoveToEx( hdc, pt.x-endwidth, pt.y-d.y, NULL );
		LineTo( hdc, pt.x+endwidth, pt.y-d.y );
		MoveToEx( hdc, pt.x-endwidth, pt.y+d.y, NULL );
		LineTo( hdc, pt.x+endwidth, pt.y+d.y );
		MoveToEx( hdc, pt.x-d.x, pt.y-endwidth, NULL );
		LineTo( hdc, pt.x-d.x, pt.y+endwidth );
		MoveToEx( hdc, pt.x+d.x, pt.y-endwidth, NULL );
		LineTo( hdc, pt.x+d.x, pt.y+endwidth );
	}
	DeleteObject( SelectObject( hdc, hOld ) );
}


// Stellt eine Dib dar (NO_PALETTE: RGB-Farben verwenden
void DisplayDib( HDC hdc, LPBITMAPINFO lpDib, HWND TopHwnd, LPRECT pCoord, WORD wZoom, LPUCHAR lpDibBits )
{
	LPWORD col;
	WORD cxDib, cyDib, w;
	WORD MapMode = DIB_PAL_COLORS;
	BOOL freePal = FALSE;
	LPBITMAPINFOHEADER lpDib2;
	HPALETTE hPal;
	LOGPALETTE HUGE *lpPalette = NULL;

	if( lpDib == NULL  ||
	    ( lpDib2 = (LPBITMAPINFOHEADER)pMalloc( sizeof( BITMAPINFOHEADER )+256*sizeof( RGBQUAD ) ) ) == NULL ) {
		StatusLineRsc( E_MEMORY );
		return;
	}

	// Im 16-Farbenmodus evt. Palette weglassen (da vermutlich Drucker ...)
	if( GetDeviceCaps( hdc, NUMCOLORS ) <= 16 ) {
		TopHwnd = FALSE;
		MapMode = DIB_RGB_COLORS;
	}

	MemMove( lpDib2, lpDib, sizeof( BITMAPINFOHEADER )+256*sizeof( RGBQUAD ) );
	if( MapMode == DIB_PAL_COLORS  &&
	    ( lpPalette = (LOGPALETTE HUGE*)pMalloc( sizeof( LOGPALETTE )+256*sizeof( PALETTEENTRY ) ) ) != NULL ) {
		GetDibPalette( lpPalette, lpDib );
		freePal = TRUE;

		hPal = CreatePalette( (LOGPALETTE HUGE*)lpPalette );
		col = (LPWORD)( (LPBYTE)lpDib2+lpDib2->biSize );
		for( w = 0; w < 256;  w++ ) {
			*col++ = w;
		}
		SelectPalette( hdc, hPal, TopHwnd != NULL );
		RealizePalette( hdc );
		if( TopHwnd == NULL ) {
			DeleteObject( hCurrentPal );
			hCurrentPal = hPal;
		}
	}
	else {
		MapMode = DIB_RGB_COLORS;
	}

	SetStretchBltMode( hdc, COLORONCOLOR );
	cxDib = lpDib->bmiHeader.biWidth;
	cyDib = lpDib->bmiHeader.biHeight;
	if( wZoom <= 1  &&  pCoord == NULL ) {
		SetDIBitsToDevice( hdc, 0, 0, cxDib, cyDib,
		                   0, 0, 0, cyDib,
		                   (LPSTR) lpDibBits, (LPBITMAPINFO)lpDib2,
		                   MapMode ) ;
		BitBlt( hdc, 0, cyDib, 32000, 32000, NULL, 0, 0, WHITENESS );
		BitBlt( hdc, cxDib, 0, 32000, 32000, NULL, 0, 0, WHITENESS );
	}
	else {
		if( pCoord != NULL ) {
			// an Fenster anpassen
			BitBlt( hdc, 0, 0, 0x7FFF, 0x7FFF, NULL, 0, 0, WHITENESS );
			StretchDIBits( hdc, 0, 0, pCoord->right, pCoord->bottom,
			               0, 0, cxDib, cyDib,
			               (LPSTR) lpDibBits,
			               (LPBITMAPINFO) lpDib2,
			               MapMode, SRCCOPY ) ;
		}
		else {
			// herauszoomen
			StretchDIBits( hdc, 0, 0, cxDib/wZoom, cyDib/wZoom,
			               0, 0, cxDib, cyDib,
			               (LPSTR) lpDibBits,
			               (LPBITMAPINFO) lpDib2,
			               MapMode, SRCCOPY ) ;
			BitBlt( hdc, 0, cyDib/wZoom, 32000, 32000, NULL, 0, 0, WHITENESS );
			BitBlt( hdc, cxDib/wZoom, 0, 32000, 32000, NULL, 0, 0, WHITENESS );
		}
	}

	if( freePal ) {
		MemFree( lpPalette );
	}
	MemFree( lpDib2 );
	if( TopHwnd != NULL  &&  MapMode == DIB_RGB_COLORS ) {
		DeleteObject( hPal );
	}
}

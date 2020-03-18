// Routinen und #defines für SNOM_WRK.C

//#include <wingdi.h>
#include <windows.h>

// XXX Workaround: Old GCCs choke on type check.
#if defined __cplusplus
extern C {
#endif

// Sendet an alle Childs ein Redraw
#define UPDATE 1
#define CLEAR3D 2
#define SLIDER 4
BOOL	RedrawAll( WORD flags );

BOOL PointInRect( CONST RECT *lprc, POINT pt );
// Includes border!

BOOLEAN	WriteDib( LPSTR szFileName, LPBITMAPINFO lpDib, LPBYTE lpDaten );

// The ONLY way to copy into Metafile!!!
void	CopyDibToDC( HDC hDC, LPBITMAPINFO pDib, LPBYTE pDest, RECT xywh );

// Ende Draw3D
// April 97
// "Schnelle Integeroutinen": 28.12.97
// Berechnet die neue Bitmapfarben (nur für Recalc-Cache)
WORD	FarbenSetzen( LPBITMAPINFO pCacheDib, LPBILD pBild, LONG startcol, LONG max, double maxcol, BOOL StartEnde );

// Ermittelt Offset der Daten vom Start
LONG GetDibBitsAddr (LPBITMAPINFO lpDib);

// Kopiert die Farbewerte aus der Palette
WORD GetDibPalette ( LPLOGPALETTE palette, LPBITMAPINFO lpDib);

//**** Erzeugt neue Farbpalette, inklusive 2-Farbeverlauf und "Totfarben" ****
BOOL	SetDibPaletteColors( LPBITMAPINFO pDib, COLORREF HUGE *Farbe, LPBILD pBild, LONG StartFarbe, LONG EndFarbe, WORD ClipStart, WORD ClipEnde );

// Berechnet die "zweckmaessigste Einheit, d.h die nachste durch 1, 2, 2,5, 5 teilbare Zahl,
// die am ehesten zwischen min und max Anzahl von Strichen erzeugt ...
double	CalcIncrement( int min, int max, double len );
// 18.8.97

// Berechnet Ausmaße einer Bitmap mit Legende, 3D ...
void	CalcDibSize( HDC hdc, LONG cxDib, LONG cyDib, LPRECT pCoord, BOOL Show3D, BOOL ShowScanline );

// Zeichnet eine Bitmap neu
BOOL	RecalcCache( LPBMPDATA pBmp, BOOL AuchBitmapNeuBerechnen, BOOL AlleFarbenMitOffset );

// Zeichnet eine Bitmap in DC
BOOL	DrawInDC( HDC hdc, LPBMPDATA pBmp, BOOL AuchBitmapNeuBerechnen, BOOL AlleFarbenMitOffset, double fZoom, RECT *BitmapExtend );

/****	Kopiert die Daten einer Querschnittslinie nach puDest ****/
/**** Das sind maximal (w^2+h^2)^.5+1 Punkte ****/
/**** Wenn bNurX TRUE, gibt es als Ergebnis die X-Werte, ****
 **** wenn bNurY TRUE, gibt es nur die Y-Werte, sonst die Daten ****/
int	BildGetLine( LPUWORD puDest, LPUWORD puDaten, LPRECT pRect, LONG w, LONG h, BOOLEAN bNurX, BOOLEAN bNurY );
// 18.2.98, 20.12.98

// Malt die Markierungslinie neu (in Topografie UND Lumineszens, wenn vorhanden)
void	DrawScanLine( HDC hdc, LPBMPDATA pBmp, double fScale );
// 6.8.97

// Scanline im "Koordinatensystem" zeichnen
void	DrawScanLinePlot( HDC hdc, LPBMPDATA pBmp, double fScale, BOOLEAN bWhiteOut );
// 22.11.97

// Quantenpunkte im "Koordinatensystem" zeichnen
void	DrawDotsPlot( HDC hdc, LPBMPDATA pBmp, double fScale );
// 21.11.11

// BSF zeichnen
void	DrawLinesPlot( HDC hdc, LPBMPDATA pBmp, double fScale );
// 18.3.20

// Stellt die aktuelle Dib im Cache dar
void	DisplayDib( HDC hdc, LPBITMAPINFO lpDib, HWND TopHwnd, LPRECT pCoord, WORD wZoom, LPUCHAR lpDibBits );
// 27.7.97

// Malt eine waagerechte Achse
void	DrawHorizontalAxis( HDC hDC, int x, int y, int weite, int min, int max, double MaxWeite, double offset, const char *label, const char *alt_label );

#if defined __cplusplus
};
#endif

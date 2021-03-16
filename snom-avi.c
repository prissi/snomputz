/*************************************************************************/

#ifdef BIT32
#error "Nur 32Bit!"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>

#include "myportab.h"
#include "filebox.h"
#include "snom-typ.h"
#include "snom-var.h"
#include "snom-mem.h"
#include "snomlang.h"

#include "snom-dlg.h"
#include "snom-dsp.h"


/*************************************************************************/

#include <vfw.h>
#include "avi_utils.h"


typedef struct {
	SNOMDATA *pSnomData;
	RECT *sizes;
	DWORD lCurrent, lImages, lMaxImages;
	int xSize, ySize;
	double fHeightScale;
	COLORREF backgroundColor;
} MovieDataStruct;

MovieDataStruct CurrentMovie = {NULL, NULL, 0, 0, 0, 256, 256, 50.0, RGB( 96, 255, 255 ) };

// Routine, die die einzelnen Unterroutinen aufruft
LRESULT CALLBACK MovieWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam );


/*************************************************************************/


// copy the bitmap with the current settings to the display
void DrawSnomdataAt( HDC hDC, int iDisplay, int yoff, BOOL transparent )
{
	static LPBITMAPINFO pDib = NULL;
	static BYTE *pDest = NULL;
	LONG x, y, i, startcol, endcol, lowvalue, maxvalue, lScaleFactor;
	DWORD ww;
	UWORD *pSrc;
	BYTE *pBits;
	RECT rect = CurrentMovie.sizes[iDisplay];
	SNOMDATA *pSnomData = &( CurrentMovie.pSnomData[iDisplay] );

	// Temporaere bitmap init
	if( pDib == NULL ) {
		pDib = (LPBITMAPINFO )pMalloc( sizeof( BITMAPINFO )+256*sizeof( RGBQUAD ) );
	}
	if( pDib == NULL ) {
		return;
	}
	if( pDest != NULL ) {
		MemFree( pDest );
	}

	pDib->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	pDib->bmiHeader.biWidth = pSnomData->w;
	pDib->bmiHeader.biHeight = pSnomData->h;
	pDib->bmiHeader.biPlanes = 1;
	pDib->bmiHeader.biBitCount = 8;
	pDib->bmiHeader.biCompression = BI_RGB;
	pDib->bmiHeader.biXPelsPerMeter = GetDeviceCaps( hDC, LOGPIXELSX )*39.37;
	pDib->bmiHeader.biYPelsPerMeter = GetDeviceCaps( hDC, LOGPIXELSY )*39.37;
	pDib->bmiHeader.biSizeImage = 0;
	pDib->bmiHeader.biClrUsed =
	        pDib->bmiHeader.biClrImportant = 256;

	ww = ( pSnomData->w+3l )&( ~3l );
	pDest = pBits = (LPUCHAR)pMalloc( ww*pSnomData->h );
	if( pBits == NULL ) {
		return;
	}

	for( i = 0;  i < 256;  i++ ) {
		( (LPWORD)( pDib->bmiColors ) )[i] = i;
	}

	// calculate the offsets
	maxvalue = pSnomData->Topo.uMaxDaten/( ( pSnomData->Topo.uMaxDaten*pSnomData->Topo.fSkal )/CurrentMovie.fHeightScale );
	lowvalue = pSnomData->Topo.uModulo;
	lowvalue -= pSnomData->Topo.uMaxDaten;
	maxvalue += lowvalue;
	// Tabelle zu schnellen Berechnung initialisieren
	startcol = 0;
	endcol = 255;
	lScaleFactor = maxvalue-lowvalue;
	SetDibPaletteColors( pDib, pSnomData->Topo.Farben, &( pSnomData->Topo ), 0, 255, 0, 255 );
	for( x = 0;  x < pSnomData->Topo.uMaxDaten;  x++ ) {
		if( x >= maxvalue ) {
			pColorConvert[x] = endcol;
		}
		else if( x < lowvalue ) {
			pColorConvert[x] = startcol;
		}
		else {
			pColorConvert[x] = ( ( x-lowvalue )*255 )/lScaleFactor;
		}
	}

	pSrc = pSnomData->Topo.puDaten;
	for( y = pSnomData->h-1;  y >= 0;  y-- ) {
		for( x = 0;  x < pSnomData->w;  x++ ) {
			pBits[x] = pColorConvert[ pSrc[x+( y*pSnomData->w )] ];
		}
		pBits += ww;
	}

	// this should work on ANY device context
	rect.top += yoff;
//	CopyDibToDC( hDC, pDib, pDest, rect );
	if( transparent ) {
		SetStretchBltMode( hDC, WHITEONBLACK );
		StretchDIBits( hDC, rect.left, rect.top, rect.right, rect.bottom,
		               0, 0, pSnomData->w, pSnomData->h,
		               pDest, pDib, DIB_RGB_COLORS, SRCCOPY );
	}
	else {
		SetStretchBltMode( hDC, COLORONCOLOR );
		StretchDIBits( hDC, rect.left, rect.top, rect.right, rect.bottom,
		               0, 0, pSnomData->w, pSnomData->h,
		               pDest, pDib, DIB_RGB_COLORS, SRCCOPY );
	}
}


/*************************************************************************/


void render_to_avi( char *aviname )
{
	BITMAPINFOHEADER bih;
	HDC hdcscreen, hdc;
	void *bits;
	int frame;
	HBITMAP hbm;
	HGDIOBJ hbrush = CreateSolidBrush( CurrentMovie.backgroundColor );
	HAVI avi = CreateAvi( aviname, 333, NULL );

	bih.biSize = sizeof( bih );
	bih.biWidth = CurrentMovie.xSize;
	bih.biHeight = CurrentMovie.ySize;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = ( ( bih.biWidth*bih.biBitCount/8+3 )&0xFFFFFFFC )*bih.biHeight;
	bih.biXPelsPerMeter = 10000;
	bih.biYPelsPerMeter = 10000;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	hdcscreen = GetDC( 0 );
	hdc = CreateCompatibleDC( hdcscreen );
	hbm = CreateDIBSection( hdc, (BITMAPINFO*)&bih, DIB_RGB_COLORS, &bits, NULL, 0 );
	SelectObject( hdc, hbm );
	//
	for( frame = 0; frame < CurrentMovie.lImages; frame++ )	{
		// background
		SetMapMode( hdc, MM_TEXT );
		SelectObject( hdc, hbrush );
		Rectangle( hdc, -1, -1, CurrentMovie.xSize+2, CurrentMovie.ySize+2 );
		// current image
		DrawSnomdataAt( hdc, frame, 0, FALSE );
		AddAviFrame( avi, hbm );
	}
	CloseAvi( avi );
	//
	ReleaseDC( 0, hdcscreen );
	DeleteDC( hdc );
	DeleteObject( hbm );
	DeleteObject( hbrush );
}


/*************************************************************************/

HWND hwndMovieStatus = NULL;    // Handle for status bar
HWND hwndMovieMaker = NULL;

/* Movie-Dialog */
int WINAPI MovieDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	// color choose dialog for windows
	static CHOOSECOLOR cc;
	static DWORD cColors[16];

	static HWND hComboSrc, h;
	static HGDIOBJ brush;

	switch( message ) {
		case WM_INITDIALOG:
		{
			char str[32];
			int x = CurrentMovie.xSize;
			int y = CurrentMovie.ySize;
			RECT r;
			SetRect( &r, 0, 0, 256, 256 );
			if( CurrentMovie.sizes != NULL ) {
				RECT r;
				MemMove( &r, &CurrentMovie.sizes[CurrentMovie.lCurrent], sizeof( RECT ) );
			}
#ifdef _WIN32
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_XOFFSET, hInst, GetDlgItem( hdlg, MOVIE_XOFFSET ), 1024, -1024, r.left );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_YOFFSET, hInst, GetDlgItem( hdlg, MOVIE_YOFFSET ), 1024, -1024, r.top );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_FRAMESIZE_X, hInst, GetDlgItem( hdlg, MOVIE_FRAMESIZE_X ), 1024, 0, r.right );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_FRAMESIZE_Y, hInst, GetDlgItem( hdlg, MOVIE_FRAMESIZE_Y ), 1024, 0, r.bottom );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_XSIZE, hInst, GetDlgItem( hdlg, MOVIE_XSIZE ), 1024, 0, x );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_YSIZE, hInst, GetDlgItem( hdlg, MOVIE_YSIZE ), 1024, 0, y );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, MOVIE_FRAME_NO, hInst, GetDlgItem( hdlg, MOVIE_FRAME_NO ), 0, 256, CurrentMovie.lCurrent );
#else
			SetDlgItemInt( hdlg, MOVIE_XOFFSET, r.left, FALSE );
			SetDlgItemInt( hdlg, MOVIE_YOFFSET, r.top, FALSE );
			SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_X, r.right, FALSE );
			SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_Y, r.bottom, FALSE );
			SetDlgItemInt( hdlg, MOVIE_XSIZE, x, FALSE );
			SetDlgItemInt( hdlg, MOVIE_ySIZE, y, FALSE );
			SetDlgItemInt( hdlg, MOVIE_FRAME_NO, CurrentMovie.lCurrent, FALSE );
#endif
			// height scale
			gcvt( CurrentMovie.fHeightScale, 5, str );
			SetDlgItemText( hdlg, MOVIE_SCALE_Z, str );

			// ComboBoxen initialisieren
			hComboSrc = GetDlgItem( hdlg, MOVIE_COMBOBOX_FRAME );
			SendMessage( hComboSrc, CB_RESETCONTENT, 0, 0 );
			EnumChildWindows( hwndClient, EnumSetComboNames, (LPARAM)MAKELONG( hComboSrc, NULL ) );

			// nďż˝tige Handels holen
			h = GetDlgItem( hdlg, MOVIE_BACKGROUND );
			// Farbeinstellung vorbereiten
			brush = CreateSolidBrush( CurrentMovie.backgroundColor );
			cc.lStructSize = sizeof( CHOOSECOLOR );
			cc.hwndOwner = hdlg;
			cc.hInstance = hInst;
			cc.Flags = CC_RGBINIT|CC_FULLOPEN;
			cc.lCustData = 0L;
			cc.lpfnHook = NULL;
			cc.lpTemplateName = NULL;
		}
			return ( TRUE ) ;

#ifdef	BIT32
		case WM_CTLCOLORSTATIC:
		{
			if( (HWND)lParam == h ) {
				return ( (int)brush );
			}
			break;
		}

#else
		// Makierung fďż˝r die Buttons ...
		case WM_CTLCOLOR:
			if( (HWND)lParam == h ) {
				return ( brush );
			}
			break;
#endif

		case WM_HSCROLL:
		{
			int iRange, iStart;
			int iOffset = GetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL );
			GetScrollRange( GET_SCROLL_HANDLE( lParam ), SB_CTL, &iStart, &iRange );
			switch( LOWORD( wParam ) ) {
				case SB_LINEDOWN:
					iOffset++;
					break;

				case SB_LINEUP:
					iOffset--;
					break;

				case SB_PAGEDOWN:
					iOffset += iRange/16;
					break;

				case SB_PAGEUP:
					iOffset -= iRange/16;
					break;

				case SB_BOTTOM:
					iOffset = iRange-1;
					break;

				case SB_TOP:
					iOffset = 0;
					break;

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
#ifndef BIT32
					iOffset = LOWORD( lParam );
#else
					iOffset = HIWORD( wParam );
#endif
					break;
			}
			SetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL, (WORD)iOffset, TRUE );
			if( CurrentMovie.pSnomData  &&  CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.puDaten != NULL ) {
				CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uModulo = GetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL );
				InvalidateRect( GetParent( hdlg ), NULL, FALSE );
			}
			break;
		}

		case WM_COMMAND:
			switch( LOWORD( wParam ) ) {
				case MOVIE_TRANSITION:
					if( !IsDlgButtonChecked( hdlg, MOVIE_TRANSITION ) ) {
						KillTimer( GetParent( hdlg ), 0 );
						InvalidateRect( GetParent( hdlg ), NULL, 0 );
					}
					else {
						SetTimer( GetParent( hdlg ), 0, 10, NULL );
//					SetTimer( GetParent(hdlg), 0, 50, MovieWndProc );
					}
					break;

				case MOVIE_XOFFSET:
				case MOVIE_YOFFSET:
				case MOVIE_FRAMESIZE_X:
				case MOVIE_FRAMESIZE_Y:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						if( CurrentMovie.sizes != NULL ) {
							CurrentMovie.sizes[CurrentMovie.lCurrent].left = GetDlgItemInt( hdlg, MOVIE_XOFFSET, NULL, TRUE );
							CurrentMovie.sizes[CurrentMovie.lCurrent].top = GetDlgItemInt( hdlg, MOVIE_YOFFSET, NULL, TRUE );
							CurrentMovie.sizes[CurrentMovie.lCurrent].right = GetDlgItemInt( hdlg, MOVIE_FRAMESIZE_X, NULL, FALSE );
							CurrentMovie.sizes[CurrentMovie.lCurrent].bottom = GetDlgItemInt( hdlg, MOVIE_FRAMESIZE_Y, NULL, FALSE );
							InvalidateRect( GetParent( hdlg ), NULL, FALSE );
						}
					}
					break;

				case MOVIE_COMBOBOX_FRAME:
#ifdef BIT32
					if( HIWORD( wParam ) == CBN_SELCHANGE )
#else
					if( HIWORD( lParam ) == CBN_SELCHANGE )
#endif
					{
						int i = (WORD)SendMessage( hComboSrc, CB_GETCURSEL, 0, 0L );
						LONG SrcHwnd = (LONG)SendMessage( hComboSrc, CB_GETITEMDATA, i, 0L );
						LPBMPDATA pSrc = (LPBMPDATA)GetWindowLong( (HWND)SrcHwnd, 0 );

						if( pSrc ) {
							SNOMDATA *pOrgSnom = pAllocNewSnom( pSrc, TOPO );
							int x, y;

							// need to extent?
							if( CurrentMovie.lImages >= CurrentMovie.lMaxImages ) {
								DWORD lNewMax = ( CurrentMovie.lMaxImages+256 )&0x0100;
								if( CurrentMovie.lMaxImages > 0 ) {
									// reallac ...
								}
								else {
									CurrentMovie.pSnomData = pMalloc( sizeof( SNOMDATA )*lNewMax );
									CurrentMovie.sizes = pMalloc( sizeof( RECT )*lNewMax );
									CurrentMovie.lMaxImages = lNewMax;
								}
							}

							// movie will be always stored in the snom data
							if( CurrentMovie.pSnomData[CurrentMovie.lImages].Topo.puDaten != NULL ) {
								// free it
								MemFree( CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.puDaten );
							}

							MemMove( &( CurrentMovie.pSnomData[CurrentMovie.lCurrent] ), pOrgSnom, sizeof( SNOMDATA ) );
							if( CurrentMovie.lCurrent > 0 ) {
								x = CurrentMovie.sizes[CurrentMovie.lCurrent-1].left;
								y = CurrentMovie.sizes[CurrentMovie.lCurrent-1].top;
							}
							else {
								x = y = 0;
							}
							CurrentMovie.sizes[CurrentMovie.lCurrent].right = pOrgSnom->w;
							CurrentMovie.sizes[CurrentMovie.lCurrent].bottom = pOrgSnom->h;
							SetDlgItemInt( hdlg, MOVIE_XOFFSET, x, TRUE );
							SetDlgItemInt( hdlg, MOVIE_YOFFSET, y, TRUE );
							SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_X, pOrgSnom->w, FALSE );
							SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_Y, pOrgSnom->h, FALSE );
							// set start position
							CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uModulo = CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uMaxDaten;
							i = CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uMaxDaten;
							SetScrollRange( GetDlgItem( hdlg, MOVIE_ZOFFSET ), SB_CTL, 0, 3*i, FALSE );
							SetScrollPos( GetDlgItem( hdlg, MOVIE_ZOFFSET ), SB_CTL, i, FALSE );
							if( CurrentMovie.lImages <= CurrentMovie.lCurrent ) {
								CurrentMovie.lImages = CurrentMovie.lCurrent+1;
							}

							InvalidateRect( GetParent( hdlg ), NULL, FALSE );

							// delete the extra one of the original ...
							pSrc->iMax = pSrc->iAktuell;
							pSrc->iAktuell--;
						}
					}
					break;

				case MOVIE_SCALE_Z:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						char str[32];
						GetDlgItemText( hdlg, MOVIE_SCALE_Z, str, 32 );
						CurrentMovie.fHeightScale = atof( str )+1e-9;
						InvalidateRect( GetParent( hdlg ), NULL, FALSE );
					}
					break;

				case MOVIE_XSIZE:
				case MOVIE_YSIZE:
				{
					RECT xywh;
					int w, h;
					HWND hwnd = GetParent( hdlg );
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						CurrentMovie.xSize = GetDlgItemInt( hdlg, MOVIE_XSIZE, NULL, FALSE );
						CurrentMovie.ySize = GetDlgItemInt( hdlg, MOVIE_YSIZE, NULL, FALSE );
//					goto Redraw3D;
					}
					GetWindowRect( hwnd, &xywh );
					w = xywh.right-xywh.left;
					h = xywh.bottom-xywh.top;
					GetClientRect( hwnd, &xywh );
					w -= xywh.right;
					h -= xywh.bottom;
					GetWindowRect( hdlg, &xywh );
					// left space for the Movie
					w += ( xywh.right-xywh.left > CurrentMovie.xSize ) ? xywh.right-xywh.left : CurrentMovie.xSize;
					h += xywh.bottom-xywh.top;
					SetWindowPos( hwndMovieMaker, NULL, 0, 0, w, xywh.bottom, SWP_NOSIZE|SWP_NOZORDER );
					GetWindowRect( hwndMovieStatus, &xywh );
					h += xywh.bottom-xywh.top;
					h += CurrentMovie.ySize;
					SetWindowPos( hwnd, NULL, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER );
					SendMessage( hwndMovieStatus, WM_SIZE, SIZE_RESTORED, MAKELONG( w, h ) );
					InvalidateRect( GetParent( hdlg ), NULL, FALSE );
					break;
				}

				case MOVIE_CHOOSE_BACKGROUND:
					cc.rgbResult = CurrentMovie.backgroundColor;
					cc.lpCustColors = (LPDWORD)cColors;
					if( !ChooseColor( &cc ) ) {
						break;
					}
					CurrentMovie.backgroundColor = cc.rgbResult;
					DeleteObject( brush );
					brush = CreateSolidBrush( CurrentMovie.backgroundColor );
					InvalidateRect( GetParent( hdlg ), NULL, FALSE );
					break;

				case MOVIE_FRAME_NO:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						long l = GetDlgItemInt( hdlg, MOVIE_FRAME_NO, NULL, FALSE );
						if( l >= 0  &&  l <= CurrentMovie.lImages  &&  CurrentMovie.sizes != NULL ) {
							RECT r;
							CurrentMovie.lCurrent = l;
							MemMove( &r, &CurrentMovie.sizes[CurrentMovie.lCurrent], sizeof( RECT ) );
							if( CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.puDaten != NULL ) {
								int range = CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uMaxDaten;
								int offset = CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.uModulo;
								SetScrollRange( GetDlgItem( hdlg, MOVIE_ZOFFSET ), SB_CTL, 0, range*3, TRUE );
								SetScrollPos( GetDlgItem( hdlg, MOVIE_ZOFFSET ), SB_CTL, offset, TRUE );
							}
							SetDlgItemInt( hdlg, MOVIE_XOFFSET, r.left, TRUE );
							SetDlgItemInt( hdlg, MOVIE_YOFFSET, r.top, TRUE );
							SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_X, r.right, FALSE );
							SetDlgItemInt( hdlg, MOVIE_FRAMESIZE_Y, r.bottom, FALSE );
						}
					}
					break;

				case MOVIE_SELECT:
					if( CurrentMovie.pSnomData != NULL  &&  CurrentMovie.pSnomData[CurrentMovie.lCurrent].Topo.puDaten != NULL ) {
						CurrentMovie.lCurrent++;
						SetDlgItemInt( hdlg, MOVIE_FRAME_NO, CurrentMovie.lCurrent, FALSE );
						InvalidateRect( GetParent( hdlg ), NULL, FALSE );
						// ComboBoxen reinitialisieren
					}
					SendMessage( hComboSrc, CB_RESETCONTENT, 0, 0 );
					EnumChildWindows( hwndClient, EnumSetComboNames, (LPARAM)MAKELONG( hComboSrc, NULL ) );
					break;

				case IDHELP:
					// to be done
//				WinHelp(hwndFrame,szHilfedatei,HELP_KEY,(DWORD)(LPSTR)STR_HELP_VIEW);
					break;

				case IDOK:
					if( CurrentMovie.pSnomData != NULL ) {
						char str[1024];
						str[0] = 0;
						if( CMUFileSave( hwndFrame, "Save AVI", str, "*.avi\0", NULL ) > 0 ) {
							render_to_avi( str );
						}
					}

				case IDCANCEL:
					return ( TRUE ) ;
			}
	}
	// Weitergabe an DefFrameProc (ersetzt DefWindowProc)
	return ( FALSE );
}


// 3.5.97
// 19.11.97

/*************************************************************************/

// Identifier fĂĽr Tool/Statusbar
#define ID_MOVIESTATUSBAR 1920


// Routine, die die einzelnen Unterroutinen aufruft
LRESULT CALLBACK MovieWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
	extern HWND hModeLess;                                  // handle of current inlay dialog (only used for key strokes)
	static HWND hwndCreateParent;           // Handle fďż˝r die Erzeugung neuer Fenster ...
	static int iYOffset;
	static BOOL redraw_count = 0;


	switch( message ) {
		case WM_CREATE:
		{
			hwndCreateParent = (HWND)lParam;
			//|TBSTYLE_TOOLTIPS
			hwndMovieStatus = CreateStatusWindow( WS_CHILD|WS_BORDER|WS_VISIBLE, NULL, hwnd, ID_MOVIESTATUSBAR );
			hwndMovieMaker = CreateDialog( hInst, "MovieMaker", hwnd, MovieDialog );
			hModeLess = hwndMovieMaker;
			break;
		}

		case WM_MOUSEMOVE:
			//  dragging a window
			if( CurrentMovie.lCurrent < CurrentMovie.lImages ) {
				static int dx = -1, dy = -1;
				static int last_mb = 0;
				RECT pCoord;
				POINT pt;
				pt.x = GET_X_LPARAM( lParam );
				pt.y = GET_Y_LPARAM( lParam );
				GetClientRect( hwndMovieMaker, &pCoord );
				pCoord.right = pCoord.left + CurrentMovie.xSize + 1;
				pCoord.top = pCoord.bottom;
				pCoord.bottom = pCoord.top + CurrentMovie.ySize + 1;
				if( PtInRect( &pCoord, pt ) ) {
					if( wParam == MK_LBUTTON ) {
						if( last_mb == 0 ) {
							last_mb = 1;
						}
						else if( dx-pt.x != 0  ||  dy-pt.y != 0 ) {
							// moved!
							// ATTENTION: setting them will reset all other parameters => calculate them on the fly
							SetDlgItemInt( hwndMovieMaker, MOVIE_XOFFSET, CurrentMovie.sizes[CurrentMovie.lCurrent].left + ( dx-pt.x ), TRUE );
							SetDlgItemInt( hwndMovieMaker, MOVIE_YOFFSET, CurrentMovie.sizes[CurrentMovie.lCurrent].top + ( dy-pt.y ), TRUE );
							InvalidateRect( hwnd, &pCoord, FALSE );
						}
						dx = pt.x;
						dy = pt.y;
					}
					else {
						last_mb = 0;
					}
				}
			}
			break;

		case WM_TIMER:
		{
			static DWORD next_update_ms = 0;
			if( CurrentMovie.lCurrent > 0  &&  timeGetTime() > next_update_ms ) {
				RECT pCoord;
				redraw_count ^= 1;
				next_update_ms = timeGetTime()+25+25*redraw_count;
				GetClientRect( hwndMovieMaker, &pCoord );
				pCoord.right = pCoord.left + CurrentMovie.xSize + 1;
				pCoord.top = pCoord.bottom;
				pCoord.bottom = pCoord.top + CurrentMovie.ySize + 1;
				InvalidateRect( hwnd, &pCoord, FALSE );
			}
		}

		case WM_PAINT:
		{
			HWND TopHwnd = NULL;
			RECT pCoord;
			PAINTSTRUCT ps;
			HGDIOBJ hbrush = CreateSolidBrush( CurrentMovie.backgroundColor );
			HDC hdc = BeginPaint( hwnd, &ps );
			int yoff;
			BOOL flicker = CurrentMovie.lCurrent > 0  &&  IsDlgButtonChecked( hwndMovieMaker, MOVIE_TRANSITION );

			SetMapMode( hdc, MM_TEXT );
			GetWindowRect( hwndMovieMaker, &pCoord );

			yoff = pCoord.bottom-pCoord.top;

			// background
			SelectObject( hdc, hbrush );
			Rectangle( hdc, -1, yoff-1, CurrentMovie.xSize+1, yoff+CurrentMovie.ySize+1 );

#if 1
			// in Flicker mode drwa previous image too
			if( flicker  &&  redraw_count ) {
				DrawSnomdataAt( hdc, CurrentMovie.lCurrent-1, yoff, FALSE );
			}
			// current image
			if( CurrentMovie.lCurrent < CurrentMovie.lImages ) {
				DrawSnomdataAt( hdc, CurrentMovie.lCurrent, yoff, FALSE );
			}
			// in Flicker mode drwa previous image too
			if( flicker  &&  !redraw_count ) {
				DrawSnomdataAt( hdc, CurrentMovie.lCurrent-1, yoff, FALSE );
			}
#else
			if( flicker ) {
				DrawSnomdataAt( hdc, CurrentMovie.lCurrent-1, yoff, FALSE );
			}
			// current image
			if( CurrentMovie.lCurrent < CurrentMovie.lImages ) {
				DrawSnomdataAt( hdc, CurrentMovie.lCurrent, yoff, flicker );
			}
#endif
			DeleteObject( hbrush );
			EndPaint( hwnd, &ps );
		}
			return ( 0 ) ;

		case WM_DESTROY:
		{
			DestroyWindow( hwndMovieMaker );
			if( hModeLess == hwndMovieMaker ) {
				hwndMovieMaker = hModeLess = NULL;
			}
			break;
		}

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) ) {
				// would only handle toolbar messages
			}
			break;
		}
	}

	// Weitergabe an DefFrameProc (ersetzt DefWindowProc)
	return ( DefWindowProc( hwnd, message, wParam, lParam ) );
}


// 4.1.99



/*--------------------------------------------------------
	SNOMPUTZ	Routinen zum Filtern vom BMP-Dateien, Clientfensterverwaltung
	--------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <windows.h>
#include <windowsx.h>
//#include <ctl3d.h>
#include <commctrl.h>
#include <commdlg.h>
#if !defined(_WIN32) && !defined(__WIN32__)
#include <print.h>
#endif

#include "myportab.h"

#include "snomputz.h"
#include "snom-typ.h"
#include "snom-var.h"
#include "snomlang.h"

#include "snom-win.h"
#include "snom-mem.h"
#include "snom-dlg.h"
#include "snom-dsp.h"
#include "snom-dat.h"
#include "snom-wrk.h"
#include "snom-mat.h"

#include "filebox.h"


/***************************************************************************************/

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif


/*****************************************************************************************/
//** "Rückruffunktion" für die individuellen Daten von Bitmap-Dokumentenfenstern
long WINAPI	BmpWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
	extern HMENU	hMenuBmpWindow, hMenuInitWindow;
	static char		str[1024];
	static BOOL		bMouseMove;	// Damit nicht ein Stau beim Mousemove-Messages entsteht ...
	static POINT	OldPt;
	LPBMPDATA 		pBmp;
	HDC             hdc ;
	PAINTSTRUCT     ps ;
	HCURSOR			hCurSave;
	WORD			wButton;

	pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
	switch (message)
	{
		case WM_CREATE:
			{ASSERT(lParam!=0);}
		{
			LONG	lPtr=(((LPMDICREATESTRUCT)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam);

			// Platz für die individuellen Fensterdaten
			SetWindowLong( hwnd, 0, lPtr );
			if(  lPtr!=GetWindowLong( hwnd, 0 )  )
			{
				DestroyWindow( hwnd );
				return FALSE;
			}
			pBmp = (LPBMPDATA)lPtr;
			pBmp->iAktuell = 0;
			pBmp->iSaved = 0;
			pBmp->iMax = 1;
			pBmp->fZoom = 1.0;
			pBmp->bPlotUnten = PlotsUnten;
			pBmp->dot_histogramm = NULL;
			GetWindowText( hwnd, pBmp->szName, 256 );
			pBmp->wKurzname = lstrlen( (LPSTR)pBmp->szName );
			while(  pBmp->wKurzname>0  )
			{
				if(  pBmp->szName[pBmp->wKurzname]=='\\'  ||  pBmp->szName[pBmp->wKurzname]==':'  )
					break;
				else
					pBmp->wKurzname--;
			}
			pBmp->wKurzname++;

			// Noch keine Darstellung im Cache
			pBmp->pCacheBits = NULL;

			// Neuen Dib anlegen
			pBmp->pDib = (LPBITMAPINFO)pMalloc( sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256 );
			if(  pBmp->pDib==NULL  )
			{
				StatusLineRsc( E_MEMORY );
				DestroyWindow( hwnd );
			}
			pBmp->pDib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pBmp->pDib->bmiHeader.biWidth = 0;
			pBmp->pDib->bmiHeader.biHeight = 0;
			pBmp->pDib->bmiHeader.biPlanes = 1;
			pBmp->pDib->bmiHeader.biBitCount = 8;
			pBmp->pDib->bmiHeader.biCompression = BI_RGB;
			pBmp->pDib->bmiHeader.biClrUsed =
			pBmp->pDib->bmiHeader.biClrImportant = 256;	//usedcol+5 klappt nicht?!

			EnableUndoRedo( FALSE, FALSE );
			RecalcCache( pBmp, TRUE, TRUE );
			if(  pBmp->pDib->bmiHeader.biWidth>0  )
				SetWindowPos( hwnd, NULL, 0, 0, pBmp->pDib->bmiHeader.biWidth+GetSystemMetrics(SM_CXFRAME)+5, pBmp->pDib->bmiHeader.biHeight+GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION)+5, SWP_NOMOVE|SWP_NOZORDER );
			ShowScrollBar( hwnd, SB_BOTH, FALSE );
		} // WM_CREATE
		return 0 ;

		case WM_SIZE:
		case WM_RECALC_SLIDER:
			if(  wZoomFaktor==0  )
			{
				SetScrollPos( hwnd, SB_HORZ, 0, FALSE );
				SetScrollPos( hwnd, SB_VERT, 0, FALSE );
				ShowScrollBar( hwnd, SB_HORZ, FALSE );
				ShowScrollBar( hwnd, SB_VERT, FALSE );
			}
			else
			{
				BOOL vscroll = HIWORD(lParam)*wZoomFaktor<pBmp->rectFenster.bottom;
				BOOL hscroll = LOWORD(lParam)*wZoomFaktor<pBmp->rectFenster.right+GetSystemMetrics(SM_CXVSCROLL)*vscroll;

				SetScrollRange( hwnd, SB_HORZ, 0, pBmp->rectFenster.right/wZoomFaktor-LOWORD(lParam), FALSE );
				ShowScrollBar( hwnd, SB_HORZ, hscroll );
				if(  !hscroll  )
					SetScrollPos( hwnd, SB_HORZ, 0, FALSE );
				SetScrollRange( hwnd, SB_VERT, 0, pBmp->rectFenster.bottom/wZoomFaktor+GetSystemMetrics(SM_CYHSCROLL)*hscroll-HIWORD(lParam), FALSE );
				
				vscroll = HIWORD(lParam)*wZoomFaktor<pBmp->rectFenster.bottom+GetSystemMetrics(SM_CYHSCROLL)*hscroll;
				ShowScrollBar( hwnd, SB_VERT, vscroll );
				if(  !vscroll  )
					SetScrollPos( hwnd, SB_VERT, 0, FALSE );
			}
		break;

		case WM_KEYDOWN:
#if 0
{
	wsprintf( str, "%u:%u", 255&(WORD)(lParam>>16), (WORD)wParam );
	MessageBox( hwndFrame, str, NULL, MB_ICONSTOP|MB_OK );
}
#endif
		if(  wZoomFaktor!=0  )
		{
			UINT		wMessage=NONE;
			WPARAM	wScrollPar;
			switch( LOWORD(wParam) )
			{
				case VK_LEFT:
					wScrollPar = SB_LINEUP;
					wMessage = WM_HSCROLL;
					break;
				case VK_RIGHT:
					wScrollPar = SB_LINEDOWN;
					wMessage = WM_HSCROLL;
					break;
				case VK_HOME:
					wScrollPar = SB_PAGEUP;
					wMessage = WM_HSCROLL;
					break;
				case VK_END:
					wScrollPar = SB_PAGEDOWN;
					wMessage = WM_HSCROLL;
					break;
				case VK_UP:
					wScrollPar = SB_LINEUP;
					wMessage = WM_VSCROLL;
					break;
				case VK_DOWN:
					wScrollPar = SB_LINEDOWN;
					wMessage = WM_VSCROLL;
					break;
				case VK_PRIOR:
					wScrollPar = SB_PAGEUP;
					wMessage = WM_VSCROLL;
					break;
				case VK_NEXT:
					wScrollPar = SB_PAGEDOWN;
					wMessage = WM_VSCROLL;
					break;
			}
			if(  wMessage!=NONE  )
				SendMessage( hwnd, wMessage, wScrollPar, 0 );
		}
		break;

		// Horizontales Scrollen
		case WM_HSCROLL:
		{
			short		wOldX;
			short		wOffset = GetScrollPos( hwnd, SB_HORZ );
			wOldX = wOffset;

			switch( LOWORD(wParam) )
			{
				case SB_LINEDOWN:
					wOffset += 32;
					break;
				case SB_LINEUP:
					wOffset -= 32;
					break;
				case SB_PAGEDOWN:
					wOffset += pBmp->pDib->bmiHeader.biWidth/(wZoomFaktor*4);
					break;
				case SB_PAGEUP:
					wOffset -= pBmp->pDib->bmiHeader.biWidth/(wZoomFaktor*4);
					break;
				case SB_BOTTOM:
					wOffset = pBmp->pDib->bmiHeader.biWidth/wZoomFaktor;
					break;
				case SB_TOP:
					wOffset = 0;
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
#ifndef BIT32
					wOffset = LOWORD(lParam);
#else
					wOffset = HIWORD(wParam);
#endif
					break;
			}
			if(  wOffset<0  )
				wOffset = 0;
			SetScrollPos( hwnd, SB_HORZ, (WORD)wOffset, TRUE );
			ScrollWindowEx( hwnd, wOldX-GetScrollPos( hwnd, SB_HORZ ), 0, NULL, NULL, NULL, NULL, SW_INVALIDATE|SW_ERASE );
		}
		break;

		// Vertikales Scrollen analog
		case WM_VSCROLL:
		{
			int		wOldY;
			int		wOffset = GetScrollPos( hwnd, SB_VERT );
			wOldY = wOffset;

			switch( LOWORD(wParam) )
			{
				case SB_LINEDOWN:
					wOffset += 32;
					break;
				case SB_LINEUP:
					wOffset -= 32;
					break;
				case SB_PAGEDOWN:
					wOffset += pBmp->pDib->bmiHeader.biHeight/(wZoomFaktor*4);
					break;
				case SB_PAGEUP:
					wOffset -= pBmp->pDib->bmiHeader.biHeight/(wZoomFaktor*4);
					break;
				case SB_BOTTOM:
					wOffset = pBmp->pDib->bmiHeader.biHeight/wZoomFaktor;
					break;
				case SB_TOP:
					wOffset = 0;
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
#ifndef BIT32
					wOffset = LOWORD(lParam);
#else
					wOffset = HIWORD(wParam);
#endif
					break;
			}
			if(  wOffset<0  )
				wOffset = 0;
			SetScrollPos( hwnd, SB_VERT, (WORD)wOffset, TRUE );
			ScrollWindowEx( hwnd, 0, wOldY-GetScrollPos( hwnd, SB_VERT ), NULL, NULL, NULL, NULL, SW_INVALIDATE|SW_ERASE );
		}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDM_IMPORT_BINAER:
				case IDM_IMPORT_ASCII:
				{
					WORD	type, neu=FALSE;

					StatusLineRsc( I_IMPORT );
					szFselHelpStr = STR_HFILE_OPEN;
					str[0] = 0;
					type = CMUFileOpen(hwndFrame,STR_LOAD_IMPORT,str,STR_FILE_BMP );
					if(  type>0  )
					{
						lstrcpyn( pBmp->szName, str, 256 );
						if(  DialogBoxParam( hInst, "RohDialog", hwnd, (DLGPROC)RohImportDialog, (long)pBmp) )  
						{
							// Erfolgreich
						}
						RecalcCache( pBmp, TRUE, TRUE );
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				case IDM_SAVE:
				{
					BYTE	WhatFormat, bOk;

					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".hdz" );
					szFselHelpStr = STR_HFILE_SAVE;
					if (  CMUFileSave(hwndFrame,STR_SAVE_FILE,str,STR_FILE_SAVE_NAMES,&WhatFormat)  )
					{
						WarteMaus();
						switch( WhatFormat )
						{
							case 4:
								bOk = WriteRHK( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, str );
								break;
							case 3:
								bOk = WriteDigital( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, str );
								break;
							case 2:
							case 1:
								bOk = WriteHDF( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, str, 2-WhatFormat );
								break;
						}
						if(  bOk  )
						{
							lstrcpy( pBmp->szName, str );
							UpdateRecent( str, pBmp );
							SetWindowText( hwnd, (LPCSTR)str );
							pBmp->iSaved = pBmp->iAktuell;
						}
						NormalMaus();
					}
				}
				break;

				case IDM_COPY:
				case IDM_EXPORT_ANSICHT:
					// Diese Routine tut zweierlei: 
					//  1. Kopieren in Clipboard
					//  2. Schreiben einer Ansicht als Bitmap/Metafile in eine Datei
				if(  pBmp->pDib->bmiHeader.biWidth>0  )
				{
					LPBITMAPINFO	pDib;
					HGDIOBJ		hOld, hBitmap;
					HGLOBAL		hClip;
					HDC			hDC, hDC2;
					LPBYTE	pPtr;
					LONG		y, x;
					WORD		oldZoom = w3DZoom;
					WORD		c0, c1, c2, c3;
					LONG		w, h, ww, offset;

					w = pBmp->rectFenster.right;
					h = pBmp->rectFenster.bottom;
					ww = (w+3ul)&~3ul;
					offset = sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256l;
					hClip = (LPBITMAPINFO )GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, offset+ww*h );
					if(  hClip==NULL  )
					{
						FehlerRsc( E_MEMORY );
						break;
					}
					pDib = GlobalLock( hClip );

					// Ok, dann ist ja alles klar ...
					pDib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					pDib->bmiHeader.biWidth = w;
					pDib->bmiHeader.biHeight = h;
					pDib->bmiHeader.biPlanes = 1;
					pDib->bmiHeader.biBitCount = 8;
					pDib->bmiHeader.biCompression = BI_RGB;
					pDib->bmiHeader.biXPelsPerMeter = 0;
					pDib->bmiHeader.biYPelsPerMeter = 0;
					pDib->bmiHeader.biClrUsed = 0;
					pDib->bmiHeader.biClrImportant = 0;
					MemMove( pDib->bmiColors, pBmp->pDib->bmiColors, sizeof(RGBQUAD)*256l );

					w3DZoom = 1;
					if(  oldZoom  )
						RecalcCache( pBmp, TRUE, TRUE );
					w3DZoom = oldZoom;

					hDC = GetDC( NULL );
					hDC2 = CreateCompatibleDC( hDC );
					hBitmap = CreateDIBitmap( hDC, (LPBITMAPINFOHEADER)pDib, CBM_INIT, ((LPBYTE)pDib)+offset, (LPBITMAPINFO)pDib, DIB_RGB_COLORS );
					hOld = SelectObject( hDC2, hBitmap );

					BitBlt( hDC2, 0, 0, w, h, NULL, 0, 0, WHITENESS );
					DrawScanLine( hDC2, pBmp, 1 );
					DrawScanLinePlot( hDC2, pBmp, 1, FALSE );

					// Testfarben ermitteln!
					SelectObject( hDC2, CreatePen( PS_INSIDEFRAME, 1, cMarkierungRechts ) );
					MoveToEx( hDC2, 3, h-1, NULL );	LineTo( hDC2, 2, h-1 );
					DeleteObject( SelectObject( hDC2, CreatePen( PS_INSIDEFRAME, 1, cMarkierungLinks ) ) );
					LineTo( hDC2, 1, h-1 );
					DeleteObject( SelectObject( hDC2, GetStockObject(BLACK_PEN) ) );
					LineTo( hDC2, 0, h-1 );
					SelectObject( hDC2, GetStockObject(WHITE_PEN) );
					LineTo( hDC2, -1, h-1 );
					SelectObject( hDC2, hOld );
					GetDIBits( hDC2, hBitmap, 0, (WORD)(pDib->bmiHeader.biHeight), ((LPBYTE)pDib)+offset, (BITMAPINFO far *)pDib, DIB_PAL_COLORS );
					DeleteObject( hBitmap );
					DeleteDC( hDC2 );
					ReleaseDC( NULL, hDC );

					w = ((pBmp->pDib->bmiHeader.biWidth)+3ul)&~3ul;
					pPtr = ((LPBYTE)pDib)+offset;

					c0 = pPtr[0];
					c1 = pPtr[1];
					c2 = pPtr[2];		// Farbe Topo
					c3 = pPtr[3];		// Farbe Lumi

					pPtr[0] =
					pPtr[1] =
					pPtr[2] =
					pPtr[3] = c0;

					for(  y=0;  y<h;  y++  )
					{
						for(  x=0;  x<ww;  x++  )
						{
							if(  x<w  &&  pPtr[x]==c0  )
								pPtr[x] = pBmp->pCacheBits[y*w+x];
							else
							{
								if(  pPtr[x]==c0  )
									pPtr[x] = 0;
								else if (  pPtr[x]==c1  )
									pPtr[x] = 1;
								else if (  pPtr[x]==c2  )
									pPtr[x] = 3;
								else if (  pPtr[x]==c3  )
									pPtr[x] = 4;
							}
						}
						pPtr += ww;
					}
					MemMove( pDib->bmiColors, pBmp->pDib->bmiColors, sizeof(RGBQUAD)*256l );

					// Save the current View as Bitmap/Metafile
					if(  LOWORD(wParam)==IDM_EXPORT_ANSICHT  )
					{
						int			iDataFormat=2; //Default ist Metafile

						lstrcpy( str, pBmp->szName );
						szFselHelpStr = STR_HFILE_EXPORT;
						ChangeExt( str, ".emf" );
						if(  CMUFileSave(hwndFrame,STR_EXPORT_VIEW,str,STR_FILE_EXPORT,&iDataFormat)  )
						{
							if(  iDataFormat==1  )
								WriteDib( str, pDib, (LPBYTE)pDib+offset );
							else
							{	// schreibt ein Metafile
								RECT		xywh;
								double	fZoom;
								CHAR		desc[MAX_PATH];

								MemSet( desc, 0, MAX_PATH );
								lstrcpy( desc, "Snomputz" );
								lstrcpy( desc+9, pBmp->szName+pBmp->wKurzname );
								hDC = GetDC(hwnd);
								// Our Image should be about 7cm = 7000 height, therefore ...
								fZoom = 7000.0/(double)pBmp->rectFenster.bottom;
								hDC2 = CreateEnhMetaFile( hDC, str, NULL, desc );
								ReleaseDC( hwnd, hDC );
								lf.lfHeight *= (int)fZoom;
								DrawInDC( hDC2, pBmp, FALSE, TRUE, fZoom, &xywh );
								DrawScanLine( hDC2, pBmp, 1.0 );
								DrawScanLinePlot( hDC2, pBmp, 1.0, FALSE );
								lf.lfHeight /= (int)fZoom;
								RecalcCache( pBmp, FALSE, FALSE );
								CloseEnhMetaFile( hDC2 );
							}
						}
						StatusLineRsc( I_SAVE_WND );
						GlobalUnlock( hClip );
						GlobalFree( hClip );
					}
					else
					{
						RECT	xywh;
						HDC		hMeta;
						BYTE	sMetaName[MAX_PATH], sTempPath[MAX_PATH], desc[MAX_PATH];
						double	fZoom;

						// Copy ins Clipboard, so first trash content ...
						GlobalUnlock( hClip );
						OpenClipboard(hwnd);
						EmptyClipboard();
						SetClipboardData( CF_DIB, hClip );

						hDC = GetDC(hwnd);
						// in Metafile 20000 is 20 cm
						// Our Image should be about 7cm = 7000 height, therefore ...
						fZoom = 7000.0/(double)pBmp->rectFenster.bottom;
#if 0				// The Rectangle is not needed, since Windows calculates Rectangle itself
						{
							int		iMMPerPelX, iMMPerPelY;
							/* Use iWidthMM, iWidthPels, iHeightMM, and
							 * iHeightPels to determine the number of
							 * .01-millimeter units per pixel in the x- and y-directions. */
							iMMPerPelX = (GetDeviceCaps(hDC, HORZSIZE) * 100)/GetDeviceCaps(hDC, HORZRES);
							iMMPerPelY = (GetDeviceCaps(hDC, VERTSIZE) * 100)/GetDeviceCaps(hDC, VERTRES);
							xywh.left = 0;
							xywh.top = 0;
							xywh.right = (int)((pBmp->rectFenster.right+10)*iMMPerPelX*fZoom);
							xywh.bottom = (int)(pBmp->rectFenster.bottom*iMMPerPelY*fZoom);
						}
#endif
						// get temporay name for Metafile
						GetTempPath( MAX_PATH, sTempPath );
						GetTempFileName( sTempPath, "snm", 0, sMetaName );
						MemSet( desc, 0, MAX_PATH );
						lstrcpy( desc, "Snomputz" );
						lstrcpy( desc+9, pBmp->szName+pBmp->wKurzname );
						hDC2 = CreateEnhMetaFile( hDC, sMetaName, NULL, desc );
						ReleaseDC( hwnd, hDC );
						// Finally draw!
						lf.lfHeight *= (int)fZoom;
						DrawInDC( hDC2, pBmp, FALSE, TRUE, fZoom, &xywh );
						DrawScanLine( hDC2, pBmp, 1.0 );
						DrawScanLinePlot( hDC2, pBmp, 1.0, FALSE );
						lf.lfHeight /= (int)fZoom;
						hDC2 = CloseEnhMetaFile( hDC2 );
						// Copy Handle to Clipboard
						SetClipboardData( CF_ENHMETAFILE, CopyEnhMetaFile(hDC2,NULL) );
						CloseClipboard();
						DeleteEnhMetaFile( hDC2 );
						DeleteFile( sMetaName );
//						CloseEnhMetaFile( hDC2 );
//						DeleteFile( sMetaName );
						RecalcCache( pBmp, FALSE, FALSE );
					}
					ClearStatusLine();
				}
				break;

				// Daten exportieren
				case IDM_EXPORT_ASCII:
				{
					LPSNOMDATA 			pSnom;
					LPUWORD			 	pSrc;
					LONG				x, y, w, h;
					OFSTRUCT				 of;
					int              hFile ;

					w = pBmp->pSnom[pBmp->iAktuell].w;
					h = pBmp->pSnom[pBmp->iAktuell].h;

					pSnom = &(pBmp->pSnom[pBmp->iAktuell]);

					// Ok, dann ist ja alles klar ...
					StatusLineRsc( I_EXPORT );

					pSrc = pSnom->Topo.puDaten;
					if(  pSrc!=NULL  &&  (LONG)pSrc<=256  )
						pSrc = pBmp->pSnom[(WORD)pSrc-1].Topo.puDaten;
					if(  pSrc==NULL  )
					{
						break;
					}

					szFselHelpStr = STR_HFILE_EXPORT;
					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".dat" );
					if(  !CMUFileSave(hwndFrame,STR_EXPORT_DATA,str,STR_FILE_BMP,NULL)  )
						break;
					if (HFILE_ERROR == (hFile = OpenFile (str, &of, OF_WRITE | OF_CREATE)))
						break;

					// Vom Ende her mitteln (Offset=Speicherlänge ... !)
					for(  y=0;  y<h;  y++  )
					{
						char line[128];

						for(  x=0;  x<w;  x++  )
						{
							int len = sprintf( line, "%u\t", *pSrc );
							_hwrite (hFile, (LPSTR)line, len );
							pSrc ++;
						}
						_hwrite (hFile, (LPSTR)"\n", 1 );
					}
					_lclose (hFile);
				}
				break;

				case IDM_EXPORT_TOPO:
				case IDM_EXPORT_LUMI:
				{
					LPBITMAPINFO 		pDib;
					LPSNOMDATA 			pSnom;
					LPUWORD			 		pSrc;
					LPBYTE			 		pLine;
					LONG						x, y, w, h, ww, offset;
					WORD						min, max;
					double					faktor;

					w = pBmp->pSnom[pBmp->iAktuell].w;
					h = pBmp->pSnom[pBmp->iAktuell].h;
					ww = (w+3ul)&~3ul;

					offset = sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256l+ww*h;
					pDib = (LPBITMAPINFO)pMalloc( offset );
					if(  pDib==NULL  )
					{
						FehlerRsc( E_MEMORY );
						break;
					}

					pSnom = &(pBmp->pSnom[pBmp->iAktuell]);

					// Ok, dann ist ja alles klar ...
					StatusLineRsc( I_EXPORT );
					pDib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					pDib->bmiHeader.biWidth = w;
					pDib->bmiHeader.biHeight = h;
					pDib->bmiHeader.biPlanes = 1;
					pDib->bmiHeader.biBitCount = 8;
					pDib->bmiHeader.biCompression = BI_RGB;
					pDib->bmiHeader.biXPelsPerMeter = 0;
					pDib->bmiHeader.biYPelsPerMeter = 0;
					pDib->bmiHeader.biClrUsed = 0;
					pDib->bmiHeader.biClrImportant = 0;
					faktor = 255.0/(double)(max-min);

					switch(wParam)
					{
						case IDM_EXPORT_TOPO:
							pSrc = pSnom->Topo.puDaten;
							if(  pSrc!=NULL  &&  (LONG)pSrc<=256  )
								pSrc = pBmp->pSnom[(WORD)pSrc-1].Topo.puDaten;
							min = (pSnom->Topo.fStart*(double)pSnom->Topo.uMaxDaten/100.0+0.5);
							max = (pSnom->Topo.fEnde*(double)pSnom->Topo.uMaxDaten/100.0+0.5);
							SetDibPaletteColors( pDib, pSnom->Topo.Farben, &(pSnom->Topo), 0, 255, 0, 255 );
							break;
						case IDM_EXPORT_LUMI:
							pSrc = pSnom->Lumi.puDaten;
							if(  pSrc!=NULL  &&  (LONG)pSrc<=256  )
								pSrc = pBmp->pSnom[(WORD)pSrc-1].Lumi.puDaten;
							min = (pSnom->Lumi.fStart*(double)pSnom->Lumi.uMaxDaten/100.0+0.5);
							max = (pSnom->Lumi.fEnde*(double)pSnom->Lumi.uMaxDaten/100.0+0.5);
							SetDibPaletteColors( pDib, pSnom->Lumi.Farben, &(pSnom->Lumi), 0, 255, 0, 255 );
							break;
					}
					if(  pSrc==NULL  )
					{
						MemFree( pDib );
						break;
					}

					// Vom Ende her mitteln (Offset=Speicherlänge ... !)
					for(  y=0;  y<h;  y++  )
					{
						offset -= ww;
						pLine = (LPBYTE)pDib+offset;
						for(  x=0;  x<w;  x++  )
						{
							if(  *pSrc>max  )
								pLine[x] = 255;
							else if(  *pSrc<min  )
								pLine[x] = 0;
							else
								pLine[x] = (BYTE)(((double)(*pSrc-min)*faktor+0.5));
							pSrc ++;
						}
					}

					szFselHelpStr = STR_HFILE_EXPORT;
					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".bmp" );
					if (  pDib!=NULL  &&  CMUFileSave(hwndFrame,STR_EXPORT_DATA,str,STR_FILE_BMP,NULL)  )
						WriteDib( str, pDib, (LPBYTE)pDib+GetDibBitsAddr(pDib) );
					MemFree( pDib );
				}
				break;

				case IDM_DRUCKEN:
					if (  pBmp->pDib->bmiHeader.biWidth>0  )
					{
						PDlg.hwndOwner = hwndFrame;
						PDlg.Flags = PD_SHOWHELP|PD_ENABLEPRINTTEMPLATEHANDLE|PD_RETURNDC;
						PDlg.hInstance = hInst;
						PDlg.hPrintTemplate = LoadResource( hInst, FindResource( hInst, "DruckenDialog", RT_DIALOG ) );

						// Druckerabfrage und so
						if(  PrintDlg( &PDlg )  )
						{
							// Drucke Datei in höchster Qualität
							RECT	p_rect;
							WORD	old3DZoom=w3DZoom;
							double	hfaktor, vfaktor;
							DOCINFO	di;

							StatusLineRsc( I_REDRAW );
							w3DZoom = 1;
							RecalcCache( pBmp, TRUE, TRUE );
							hfaktor = (double)GetDeviceCaps( PDlg.hDC, HORZRES )/(double)pBmp->rectFenster.right;
							vfaktor = (double)GetDeviceCaps( PDlg.hDC, VERTRES )/(double)pBmp->rectFenster.bottom;
							if(  vfaktor>hfaktor  )
								vfaktor = hfaktor;
							else
								hfaktor = vfaktor;

							// Ausmaße Rechteck
							p_rect.left = p_rect.top = 0;
							p_rect.right = (int)((double)pBmp->pDib->bmiHeader.biWidth*hfaktor+0.5);
							p_rect.bottom = (int)((double)pBmp->pDib->bmiHeader.biHeight*vfaktor+0.5);

							hCurSave = SetCursor( LoadCursor (NULL, IDC_WAIT) );
							di.cbSize = sizeof(di);
							di.lpszDocName = pBmp->szName;
							di.lpszOutput = NULL;
#ifdef _WIN32
							di.lpszDatatype = "SPM Image";
							di.fwType = 0;
#endif
							StartDoc( PDlg.hDC, &di );
							StartPage( PDlg.hDC );
							DisplayDib( PDlg.hDC, pBmp->pDib, NULL, &p_rect, 0, pBmp->pCacheBits );
							DrawScanLine( PDlg.hDC, pBmp, 1.0/hfaktor );
							DrawScanLinePlot( PDlg.hDC, pBmp, 1.0/hfaktor, FALSE );
							StatusLineRsc( I_PRINTING );
							EndPage( PDlg.hDC );
							EndDoc( PDlg.hDC );
							DeleteDC( PDlg.hDC );
							SetCursor( hCurSave );
							w3DZoom = old3DZoom;
						}
						FreeResource( PDlg.hPrintTemplate );
					}
					ClearStatusLine();
				break;

				/**** Menü "Bearbeiten" ****/
				case IDM_UNDO:
					if (pBmp->iAktuell>0)
					{
						StatusLineRsc( I_UNDO );
						pBmp->iAktuell --;
						RecalcCache( pBmp, TRUE, TRUE );
					}
					// Infozeile und Menü auf Vordermann bringen
					InvalidateRect( hwnd, NULL, TRUE );
					break;

				case IDM_REDO:
					if( pBmp->iAktuell<pBmp->iMax-1 )
					{
						pBmp->iAktuell ++;
						RecalcCache( pBmp, TRUE, TRUE );
					}
					// Infozeile und Menü auf vordermann bringen
					InvalidateRect( hwnd, NULL, TRUE );
					break;

				// Scanline speichern/kopieren
				case IDM_SCANLINE:
				case IDM_COPY_SCANLINE:
				if (  pBmp->bIsScanLine  )
				{
					LONG		lTextLen, w, h;
					LPBYTE	pPtr;
					HGLOBAL	hClip;
					int			iCol=0, iSpalten, i, j, k, iPts, iDataPts, iScan=pBmp->lMaxScan-1;
					LPFLOAT	(pfDaten[12]);	// Maximale Anzahl an zu speichernden Daten
					LPUWORD	puDaten;
					float		f;
					WORKMODE	wCopyModus = WhatToDo( pBmp, wProfilMode );

					if(  !wCopyModus  &&  wProfilFlags==1  )
					{
						WarnungRsc( W_NIX );
						break;
					}

					w = pBmp->pSnom[pBmp->iAktuell].w;
					h = pBmp->pSnom[pBmp->iAktuell].h;
					if(  (puDaten=pMalloc( sizeof(UWORD)*(w+h) ))==NULL  )
					{
						FehlerRsc( E_MEMORY );
						break;
					}

					// Evt. erste Spalte freihalten
					if(  wProfilFlags&P_DIST  )
						iCol ++;

					// X-Daten kopieren ...
					if(  wProfilFlags&P_X  )
					{
						iDataPts = BildGetLine( puDaten, NULL, &(pBmp->rectScan[iScan]), w, h, TRUE, FALSE );
						if(  (pfDaten[iCol]=pMalloc( sizeof(float)*iDataPts ))==NULL  )
						{
							MemFree( puDaten );
							FehlerRsc( E_MEMORY );
							break;
						}
						f = pBmp->pSnom[pBmp->iAktuell].fX;
						for(  i=0;  i<iDataPts;  i++  )
							pfDaten[iCol][i] = puDaten[i]*f+pBmp->pSnom[pBmp->iAktuell].fXOff;
						iCol ++;
					}

					// Y-Daten kopieren ...
					if(  wProfilFlags&P_Y  )
					{
						iDataPts = BildGetLine( puDaten, NULL, &(pBmp->rectScan[iScan]), w, h, FALSE, TRUE );
						if(  (pfDaten[iCol]=pMalloc( sizeof(float)*iDataPts ))==NULL  )
						{
							MemFree( puDaten );
							while(  --iCol>0  )
								MemFree( pfDaten[iCol] );
							FehlerRsc( E_MEMORY );
							break;
						}
						f = pBmp->pSnom[pBmp->iAktuell].fY;
						for(  i=0;  i<iDataPts;  i++  )
							pfDaten[iCol][i] = puDaten[i]*f+pBmp->pSnom[pBmp->iAktuell].fYOff;
						iCol ++;
					}

					// Y-Daten kopieren ...
					iSpalten = 0;
					if(  wProfilFlags&P_Z  )
						iSpalten ++;
					if(  wProfilFlags&P_AUTOKORRELATION  )
						iSpalten ++;
					if(  wProfilFlags&P_PSD  )
						iSpalten ++;
					if(  iSpalten>0  )
					{
						// Alle drei Modi durch (j ist 1=TOPO, 2=ERRO, 4=LUMI !!!
						for(  j=1;  j<=4;  j*=2  )
							if(  wCopyModus&j  )
							{
								iDataPts = BildGetLine( puDaten, GetDataPointer( pBmp, j ), &(pBmp->rectScan[iScan]), w, h, FALSE, FALSE );
								for(  iPts=2;  iPts<iDataPts;  )
									iPts *= 2;	// Damit Zweierpotenz!
								for(  i=0;  i<iSpalten;  i++  )
								{
									if(  (pfDaten[iCol]=pMalloc( sizeof(float)*iPts ))==NULL  )
									{
										MemFree( puDaten );
										while(  --iCol>0  )
											MemFree( pfDaten[iCol+i] );
										FehlerRsc( E_MEMORY );
										break;
									}
									if(  j==TOPO  )
										f = pBmp->pSnom[pBmp->iAktuell].Topo.fSkal;
									else if(  j==ERRO  )
										f = pBmp->pSnom[pBmp->iAktuell].Error.fSkal;
									else
										f = pBmp->pSnom[pBmp->iAktuell].Lumi.fSkal;
									for(  k=0;  k<iDataPts;  k++  )
										pfDaten[iCol][k] = puDaten[k]*f;
									iCol ++;
								}
								iCol -= i;
								// Z-Daten kopieren?
								if(  wProfilFlags&P_Z  )
									iCol ++;	// Sind ja schon kopiert ...
								// Autokorrelation berechnen?
								if(  wProfilFlags&P_AUTOKORRELATION  )
								{
									Autokorrelation( pfDaten[iCol], pfDaten[iCol], iDataPts, FALSE );
									iCol++;
								}
								// Oder vielleicht lieber eine PSD?
								if(  wProfilFlags&P_PSD  )
								{
									double	fMittel=0.0;

									for(  i=0;  i<iDataPts;  i++  )
										fMittel += pfDaten[iCol][i];
									fMittel /= (float)iDataPts;
									for(  i=0;  i<iDataPts;  i++  )
										pfDaten[iCol][i] -= fMittel;
									realft( pfDaten[iCol]-1, iPts/2, 1 );
									for(  i=0;  i<iDataPts;  i++  )
										pfDaten[iCol][i] = log(pfDaten[iCol][i]*pfDaten[iCol][i]);
									iCol ++;
								}
							} // if richtiger Modus
						// For
					}

					// Schließlich die Abstandsachse ...
					if(  wProfilFlags&P_DIST  &&  iPts>0  )
					{
						if(  (pfDaten[0]=pMalloc( sizeof(float)*iDataPts ))==NULL  )
						{
							MemFree( puDaten );
							while(  --iCol>0  )
								MemFree( pfDaten[iCol] );
							FehlerRsc( E_MEMORY );
							break;
						}
						f = sqrt( pow(pBmp->pSnom[pBmp->iAktuell].fX*(pBmp->rectScan[iScan].left-pBmp->rectScan[iScan].right),2) + pow(pBmp->pSnom[pBmp->iAktuell].fY*(pBmp->rectScan[iScan].top-pBmp->rectScan[iScan].bottom),2) );
						for(  i=0;  i<iDataPts;  i++  )
							pfDaten[0][i] = (i*f)/(float)iDataPts;
					}

					MemFree( puDaten );
					lTextLen = 12l*(long)iCol*(long)iDataPts+1024l;
					// Headerlänge+Bitmaplänge
					hClip = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, lTextLen );
					if(  hClip==NULL  )
					{
						while(  --iCol>0  )
							MemFree( pfDaten[iCol] );
						FehlerRsc( E_MEMORY );
						break;
					}
					// Speicher füllen
					pPtr = GlobalLock( hClip );
					lTextLen = CopyFloat2Text( pfDaten, iCol, iDataPts, pPtr, lTextLen );
					while(  --iCol>0  )
						MemFree( pfDaten[iCol] );

					if(  LOWORD(wParam)==IDM_SCANLINE  )
					{
						HFILE hFile;
						OFSTRUCT	of;

						lstrcpy( str, pBmp->szName );
						szFselHelpStr = STR_HFILE_PROFIL;
						ChangeExt( str, ".prf" );
						if(  CMUFileSave( hwndFrame, STR_SAVE_PROFIL, str, STR_FILE_ASCII, NULL )
								 &&	(hFile=OpenFile(str,&of,OF_CREATE))!=HFILE_ERROR	   )
						{
							_lwrite( hFile, pPtr, lTextLen );
							_lclose( hFile );
						}
						GlobalUnlock( hClip );
						GlobalFree( hClip );
					}
					else
					{
						GlobalUnlock( hClip );
						OpenClipboard(hwnd);
						EmptyClipboard();
						SetClipboardData( CF_TEXT, hClip );
						CloseClipboard();
					}
					ClearStatusLine();
				}
				break;

#if 0
				case IDM_PASTE:
				{
					HANDLE	hClip;

					OpenClipboard(hwnd);
					hClip = GetClipboardData( CF_DIB );
					lpDib = (BITMAPINFO huge *)pMalloc( GMEM_MOVEABLE, GlobalSize( hClip ) );
					if(  lpDib==NULL  )
					{
						CloseClipboard();
						StatusLineRsc( E_MEMORY );
						return 0;
					}
					MemMove( lpDib, GlobalLock( hClip ), GlobalSize( hClip ) );
					GlobalUnlock( hClip );
					CloseClipboard();
					if (GetDibColors(lpDib)!=256)
					{
						MemFree (lpDib) ;
						MessageBox(hwnd,"Bitmap hat keine 256 Farben!",NULL,MB_ICONSTOP|MB_OK);
						return 0;
					}
					wsprintf( datei, "ClipBoardBitmap%i", ClipBoardNumber++ );
				} }
				break;

#endif

				case IDM_NUR_TOPO:
				case IDM_NUR_LUMI:
				case IDM_NUR_ERROR:
					if(  GetMenuState( hMenuBmp, wParam, 0 )&MF_CHECKED  )
					{
						modus &= ~(wParam-IDM_NUR_TOPO+1);
						CheckMenuItem( hMenuBmp, wParam, MF_UNCHECKED );
					}
					else
					{
						modus |= (wParam-IDM_NUR_TOPO+1);
						CheckMenuItem( hMenuBmp, wParam, MF_CHECKED );
					}
					break;

				case IDM_TRACK_ALL:
					if(  GetMenuState( hMenuBmp, wParam, 0 )&MF_CHECKED  )
					{
						modus &= ~TRACK_ALL;
						CheckMenuItem( hMenuBmp, wParam, MF_UNCHECKED );
					}
					else
					{
						modus |= TRACK_ALL;
						CheckMenuItem( hMenuBmp, wParam, MF_CHECKED );
					}
					break;

				// Redraw erzwingen
				case IDM_REDRAW:
					StatusLineRsc( I_REDRAW );
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
				break;

				//**** Farben verändern ****
				case IDM_FALSCHFARBEN:
					DialogBoxParam( hInst, "FarbenDialog", hwnd, (DLGPROC)FarbenDialog, (LPARAM)hwnd );
					break;

				/**** Menü "Verändern" ****/

				//** Bild zeilenweise mitteln **
				case IDM_ZEILENMITTEL:
					DialogBoxParam( hInst, "MittelDialog", hwnd, MittelDialog, (LPARAM)hwnd );
				break;

				// 3D-Mittel
				case IDM_3DMITTEL:
					DialogBoxParam( hInst, "Mittel3DDialog", hwnd, Mittel3DDialog, (LPARAM)hwnd );
				break;

				// Mittlere Steigung von Links nach Rechts ist Null (= Jede Terrasse eigene Farbe)
				case IDM_NULL:
				{
					LPSNOMDATA	pSnom;
					LONG				lTeiler=0;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					lTeiler = DialogBoxParam( hInst, "SteigungNull", hwnd, StdDialog, (LPARAM)1 );
					if(  lTeiler<=0  )
						break;
					StatusLineRsc( I_X_DIFF );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildSteigungX( &(pSnom->Topo), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildSteigungX( &(pSnom->Error), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildSteigungX( &(pSnom->Lumi), lTeiler, pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;


				// Mittlere Steigung von oben nach unten ist Null (= Jede Terrasse eigene Farbe)
				case IDM_DIFF_NULL:
				{
					LPSNOMDATA	pSnom;
					LONG				lTeiler=0;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					lTeiler = DialogBoxParam( hInst, "SteigungNull", hwnd, StdDialog, (LPARAM)1 );
					if(  lTeiler<=0  )
						break;
					StatusLineRsc( I_Y_DIFF );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildSteigungY( &(pSnom->Topo), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildSteigungY( &(pSnom->Error), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildSteigungY( &(pSnom->Lumi), lTeiler, pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				// Gleitendes Mittel berechen
				case IDM_GLEITEND_MITTEL:
				{
					LPSNOMDATA	pSnom;
					LONG				lTeiler=0;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					lTeiler = DialogBoxParam( hInst, "GleitendDialog", hwnd, StdDialog, (LPARAM)3 );
					if(  lTeiler<2  )
						break;
					StatusLineRsc( I_Y_DIFF );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildGleitendesMittel( &(pSnom->Topo), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildGleitendesMittel( &(pSnom->Error), lTeiler, pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildGleitendesMittel( &(pSnom->Lumi), lTeiler, pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				// Zeilenweise FFT-Mittel berechen
				case IDM_FFT_MITTEL:
					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					DialogBoxParam( hInst, "FFTDialog", hwnd, FFTDialog, (LPARAM)hwnd );
				break;

				// Einfach invertieren
				case IDM_NEGATIV:
				{
					LPSNOMDATA	pSnom;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					StatusLineRsc( I_INVERT );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildNegieren( &(pSnom->Topo), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildNegieren( &(pSnom->Error), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildNegieren( &(pSnom->Lumi), pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				// num. 1. Ableitung
				case IDM_DIFFERENTIAL:
				{
					LPSNOMDATA	pSnom;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					StatusLineRsc( I_DIFFERENTIAL );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildDifferential( &(pSnom->Topo), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildDifferential( &(pSnom->Error), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildDifferential( &(pSnom->Lumi), pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				// num. Integral (Hauptwert)
				case IDM_INTEGRAL:
				{
					LPSNOMDATA	pSnom;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					StatusLineRsc( I_DIFFERENTIAL );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						WarteMaus();

						if(  (LONG)pSnom->Topo.puDaten>256  )
							BildIntegral( &(pSnom->Topo), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Error.puDaten>256  )
							BildIntegral( &(pSnom->Error), pSnom->w, pSnom->h );
						if(  (LONG)pSnom->Lumi.puDaten>256  )
							BildIntegral( &(pSnom->Lumi), pSnom->w, pSnom->h );

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}
				break;

				case IDM_GROESSE:
					DialogBoxParam( hInst, "GrossDialog", hwnd, GrossDialog, (LPARAM)hwnd );
				break;

				case IDM_DESPIKE:
					DialogBoxParam( hInst, "DespikeDialog", hwnd, SpikeDialog, (LPARAM)hwnd );
				break;

				case IDM_MEDIAN:
					DialogBoxParam( hInst, "MedianDialog", hwnd, MedianDialog, (LPARAM)hwnd );
				break;

				// 90° Rechtsdrehung
				case IDM_DREHEN:
				{
					LPSNOMDATA	pSnom;

					if(  WhatToDo( pBmp, modus )==NONE  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					StatusLineRsc( I_TURN );
					pSnom = pAllocNewSnom( pBmp, modus );
					if(  pSnom!=NULL  )
					{
						LPUWORD	puDaten, puOldDaten;
						LONG		x, y, w, h;
						float		fTemp;

						WarteMaus();

						// Weite/Höhe Vertauschen
						w = pSnom->w ;
						h = pSnom->h;
						pSnom->w = h;
						pSnom->h = w;

						// Offsets vertauschen
						fTemp = pSnom->fXOff;
						pSnom->fXOff = pSnom->fYOff;
						pSnom->fYOff = -fTemp;
	
						// Skalierung vertauschen
						fTemp = pSnom->fX;
						pSnom->fX = pSnom->fY;
						pSnom->fY = fTemp;

						// Maske wird gelöscht ...
						if(  pBmp->pMaske  )
						{
							MemFree( pBmp->pMaske );
							pBmp->pMaske = NULL;
							pBmp->wMaskeW = 0;
						}

						// Um ungültige Pointer zu vermeiden => Scanline auf Null
						pBmp->bIsScanLine = FALSE;
						pBmp->lMaxScan = 0;
						pBmp->rectScan[0].left = pBmp->rectScan[0].right = 0;
						pBmp->rectScan[0].top =	pBmp->rectScan[0].bottom = 0;

						if(  (LONG)pSnom->Topo.puDaten>256  )
						{
							puDaten = pSnom->Topo.puDaten;
							puOldDaten = pBmp->pSnom[pBmp->iAktuell-1].Topo.puDaten;
							if(  (LONG)puOldDaten<=256  )
								puOldDaten = pBmp->pSnom[(int)puOldDaten-1].Topo.puDaten;
							for(  y=0;  y<w;  y++  )
							{
								for(  x=0;  x<h;  x++  )
									puDaten[x] = puOldDaten[w*(h-x-1)+y];
								puDaten += h;
							}
						}

						if(  (LONG)pSnom->Error.puDaten>256  )
						{
							puDaten = pSnom->Error.puDaten;
							puOldDaten = pBmp->pSnom[pBmp->iAktuell-1].Error.puDaten;
							if(  (LONG)puOldDaten<=256  )
								puOldDaten = pBmp->pSnom[(int)puOldDaten-1].Error.puDaten;
							for(  y=0;  y<w;  y++  )
							{
								for(  x=0;  x<h;  x++  )
									puDaten[x] = puOldDaten[w*(h-x-1)+y];
								puDaten += h;
							}
						}

						if(  (LONG)pSnom->Lumi.puDaten>256  )
						{
							puDaten = pSnom->Lumi.puDaten;
							puOldDaten = pBmp->pSnom[pBmp->iAktuell-1].Lumi.puDaten;
							if(  (LONG)puOldDaten<=256  )
								puOldDaten = pBmp->pSnom[(int)puOldDaten-1].Lumi.puDaten;
							for(  y=0;  y<w;  y++  )
							{
								for(  x=0;  x<h;  x++  )
									puDaten[x] = puOldDaten[w*(h-x-1)+y];
								puDaten += h;
							}
						}

						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
				}	

				break;

				case IDM_3DANSICHT:
					DialogBoxParam( hInst, "Ansicht3DDialog", hwnd, Ansicht3DDialog, (LPARAM)hwnd );
				break;

				// Skalierung eingeben ...
				case IDM_MASZE:
					DialogBoxParam( hInst, "HubDialog", hwnd, UnitDialog, (LPARAM)pBmp );
				break;

				case IDM_SHOW_TOPO:
				case IDM_SHOW_ERROR:
				case IDM_SHOW_LUMI:
					if(  GetMenuState( hMenuBmp, wParam, MF_BYCOMMAND)&MF_CHECKED  )
					{
						show &= ~(wParam-IDM_SHOW);
						CheckMenuItem( hMenuBmp, wParam, MF_UNCHECKED );
						if(  (pBmp->Links&show)==0  &&  pBmp->Links  )
							pBmp->IsDirty = TRUE;
						pBmp->Links &= show;
						if(  (pBmp->Rechts&show)==0  &&  pBmp->Rechts  )
							pBmp->IsDirty = TRUE;
						pBmp->Rechts &= show;
						InvalidateRect( hwnd, NULL, FALSE );
					}
					else
					{
						show |= (wParam-IDM_SHOW);
						CheckMenuItem( hMenuBmp, wParam, MF_CHECKED );
						switch(  (wParam-IDM_SHOW)  )
						{
							case TOPO:
								if(  pBmp->pSnom[pBmp->iAktuell].Topo.Typ==TOPO  )
								{
									// There is at least some topografical data ...
									pBmp->IsDirty = TRUE;
									if(  pBmp->Links==0  )
										pBmp->Links = TOPO;
									else if(  pBmp->Rechts==0  )
										pBmp->Rechts = TOPO;
									else
										pBmp->IsDirty = FALSE;
								}
								break;

							case ERRO:
								if(  pBmp->pSnom[pBmp->iAktuell].Error.Typ==ERRO  )
								{
									// There is at least some error data ...
									pBmp->IsDirty = TRUE;
									if(  pBmp->Links==0  )
										pBmp->Links = ERRO;
									else if(  pBmp->Rechts==0  )
										pBmp->Rechts = ERRO;
									else
										pBmp->IsDirty = FALSE;
								}
								break;

							case LUMI:
								if(  pBmp->pSnom[pBmp->iAktuell].Lumi.Typ==LUMI  )
								{
									// There is at least some topografical data ...
									pBmp->IsDirty = TRUE;
									if(  pBmp->Links==0  )
										pBmp->Links = LUMI;
									else if(  pBmp->Rechts==0  )
										pBmp->Rechts = LUMI;
									else
										pBmp->IsDirty = FALSE;
								}
								break;
						}
						if(  pBmp->IsDirty  )
							InvalidateRect( hwnd, NULL, FALSE );
					}
				break;

				/**** Menü Analyse ****/

				//**** Mark heights dialog ****
				case IDM_VOLUME:
					CreateDialogParam( hInst, "VolumeDialog", hwnd, VolumeDialog, (LPARAM)hwnd );
					break;

				case IDM_VOLUME_CALC:
				{
					if(  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  )
					{
						const LPUWORD	pTopoData=GetDataPointer(pBmp,TOPO);
						const LONG		w=pBmp->pSnom[pBmp->iAktuell].w, h=pBmp->pSnom[pBmp->iAktuell].h;
						const LONG		bw=pBmp->wMaskeW;
						const double fzSkal=pBmp->pSnom[pBmp->iAktuell].Topo.fSkal;
						const double fVoxel=pBmp->pSnom[pBmp->iAktuell].Topo.fSkal*pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].fY;
						const double fz2=fzSkal*fzSkal, fx2=pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].fX, fy2=pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].fY;
						double	dXPoly[10], dYPoly[10];

						LONG		x, y, iVolume=0, iMaskVolume=0, iDiffVolume=0, iPolyYWert=0;
						double	fArea=0, fMaskArea=0, fLocalArea;

						Fit3DBild( pBmp, pTopoData, w, h, 2, dXPoly, dYPoly );

						for(  y=0;  y<h;  y++  )
						{
							iPolyYWert = fPolyWert( y, dYPoly, 2 );
							for(  x=0;  x<w;  x++  )
							{
								if(  x<w-1  &&  y<h-1  )
								{	// Surface Area approximated by two triangles
									fLocalArea = TriangleArea( pTopoData[x+y*w], pTopoData[x+1+y*w], pTopoData[x+1+y*w+w], fx2, fy2, fz2 ); 
									fLocalArea += TriangleArea( pTopoData[x+y*w+w], pTopoData[x+1+y*w+w], pTopoData[x+y*w+w], fx2, fy2, fz2 ); 
								}
								iVolume += pTopoData[x+y*w];
								fArea += fLocalArea;
								if(  pBmp->pMaske  &&  (pBmp->pMaske[(h-1-y)*bw+(x>>3)]&(0x0080>>(x&7)))!=0  )
								{
									fMaskArea += fLocalArea;
									iMaskVolume += pTopoData[x+y*w];
									iDiffVolume += pTopoData[x+y*w]-iPolyYWert-fPolyWert( x, dXPoly, 2 );
								}
							}
						}
						if(  pBmp->pMaske  )
							sprintf( str, GetStringRsc(STR_VOLUME_MARK), fArea, fVoxel*iVolume, fMaskArea, fVoxel*iMaskVolume, fVoxel*iDiffVolume );
						else
							sprintf( str, GetStringRsc(STR_VOLUME), fArea, fVoxel*iVolume );
						MessageBox( hwnd, (LPSTR)str, "", MB_ICONINFORMATION|MB_OK );
					}
				}
				break;

				// RMS berechnen
				case IDM_RMS:
				{
					double		Mean, RMS, Median, MedianRMS;
					int			iAkt=pBmp->iAktuell;

					if(  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  )
					{
						RMSArea( GetDataPointer(pBmp,TOPO), pBmp->pSnom[iAkt].w, 0, 0, pBmp->pSnom[iAkt].w, pBmp->pSnom[iAkt].h, pBmp->pSnom[pBmp->iAktuell].Topo.fSkal, &Mean, &RMS, NULL );
						MeadianArea( GetDataPointer(pBmp,TOPO), pBmp->pSnom[iAkt].w, 0, 0, pBmp->pSnom[iAkt].w, pBmp->pSnom[iAkt].h, pBmp->pSnom[pBmp->iAktuell].Topo.fSkal, pBmp->pSnom[pBmp->iAktuell].Topo.uMaxDaten, &Median, &MedianRMS );
						sprintf( str, GetStringRsc(STR_TOPO_MEAN), Mean, RMS, Median, MedianRMS );
						MessageBox( hwnd, (LPSTR)str, "", MB_ICONINFORMATION|MB_OK );
					}
					if(  pBmp->pSnom[pBmp->iAktuell].Lumi.puDaten  )
					{
						RMSArea( GetDataPointer(pBmp,LUMI), pBmp->pSnom[iAkt].w, 0, 0, pBmp->pSnom[iAkt].w, pBmp->pSnom[iAkt].h, pBmp->pSnom[pBmp->iAktuell].Lumi.fSkal, &Mean, &RMS, NULL );
						MeadianArea( GetDataPointer(pBmp,LUMI), pBmp->pSnom[iAkt].w, 0, 0, pBmp->pSnom[iAkt].w, pBmp->pSnom[iAkt].h, pBmp->pSnom[pBmp->iAktuell].Lumi.fSkal, pBmp->pSnom[pBmp->iAktuell].Lumi.uMaxDaten, &Median, &MedianRMS );
						sprintf( str, GetStringRsc(STR_LUMI_MEAN), Mean, RMS, Median, MedianRMS );
						MessageBox( hwnd, (LPSTR)str, "", MB_ICONINFORMATION|MB_OK );
					}
				}
				break;

				// Rauhigkeitsexponent Alpha berechnen
				case IDM_ALPHA:
					DialogBoxParam( hInst, "FractalDialog", hwnd, FractalDialog, (LPARAM)hwnd );
				break;

				// Correlation function calculate
				case IDM_CORRELATION:
				{
					HFILE hFile;
					OFSTRUCT	of;

					szFselHelpStr = STR_HFILE_HIST;
					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".crl" );
					if(   pBmp->pSnom[pBmp->iAktuell].Topo.puDaten!=0  
							 &&  CMUFileSave( hwndFrame, STR_FILE_FRAC, str, STR_FILE_ASCII, NULL )
							 &&  (hFile=OpenFile(str,&of,OF_CREATE))!=HFILE_ERROR	 )
					{
						LPUWORD		pDaten;
						LONG			w, h;
						double		fSkal;

						pDaten = pBmp->pSnom[pBmp->iAktuell].Topo.puDaten;
						w = pBmp->pSnom[pBmp->iAktuell].w;
						h = pBmp->pSnom[pBmp->iAktuell].h;
						fSkal = pBmp->pSnom[pBmp->iAktuell].Topo.fSkal;
						if(  (LONG)pDaten<256l  )
						{
							pDaten = pBmp->pSnom[(LONG)pDaten].Topo.puDaten;
							w = pBmp->pSnom[(LONG)pDaten].w;
							h = pBmp->pSnom[(LONG)pDaten].h;
							fSkal = pBmp->pSnom[(LONG)pDaten].Topo.fSkal;
						}
						CorrelationFunction( hFile, pDaten, w, h, fSkal );
					}
				}
				break;

				// Histogramm speichern
				case IDM_HISTOGRAMM:
				if(   pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  ||  pBmp->pSnom[pBmp->iAktuell].Lumi.puDaten  )
				{
					HFILE hFile;
					LONG			i;
					OFSTRUCT	of;
					WORD			iAkt=pBmp->iAktuell, len;
					LPBILD		pTopo=NULL, pLumi=NULL;
					LPLONG		pTopoDat=NULL, pTopoIntegral=0l;
					LPLONG		pLumiDat=NULL, pLumiIntegral=0l;
					double		dTopo=0.0, dLumi=0.0;
					LONG			lMax=0;

					// Erst einmal aktuelle Pointer ermitteln
					if(  pBmp->pSnom[iAkt].Topo.puDaten  )
					{
						lMax = pBmp->pSnom[iAkt].Topo.uMaxDaten+1;
						if(  (LONG)pBmp->pSnom[iAkt].Topo.puDaten<256l  )
							pTopo = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Topo.puDaten-1].Topo);
						else
							pTopo = &(pBmp->pSnom[iAkt].Topo);
					}
					if(  pBmp->pSnom[iAkt].Lumi.puDaten  )
					{
						if(  lMax<=pBmp->pSnom[iAkt].Lumi.uMaxDaten  )
							lMax = pBmp->pSnom[iAkt].Lumi.uMaxDaten+1ul;
						if(  (LONG)pBmp->pSnom[iAkt].Lumi.puDaten<256l  )
							pLumi = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Lumi.puDaten-1].Lumi);
						else
							pLumi = &(pBmp->pSnom[iAkt].Lumi);
					}

					szFselHelpStr = STR_HFILE_HIST;
					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".hst" );
					if(  lMax!=0
							 &&  (pTopoDat=(LPLONG)pMalloc( lMax*sizeof(LONG) ))!=NULL
							 &&  (pLumiDat = (LPLONG)pMalloc( lMax*sizeof(LONG) ))!=NULL
							 &&  CMUFileSave( hwndFrame, STR_SAVE_HIST, str, STR_FILE_ASCII, NULL )
							 &&	(hFile=OpenFile(str,&of,OF_CREATE))!=HFILE_ERROR	   )
					{
						if(  pTopo  )
						{
							dTopo = pTopo->fSkal;
							i = pBmp->pSnom[iAkt].w*pBmp->pSnom[iAkt].h;
							while(  i-->0  )
								pTopoDat[(DWORD)pTopo->puDaten[i]] ++;
						}
						if(  pLumi  )
						{
							dLumi = pLumi->fSkal;
							i = pBmp->pSnom[iAkt].w*pBmp->pSnom[iAkt].h;
							while(  i-->0  )
								pLumiDat[(DWORD)pLumi->puDaten[i]]++;
						}
						for(  i=0;  i<lMax;  i++  )
						{
							if(  pTopoDat[i]+pLumiDat[i]  )
							{
								pTopoIntegral += pTopoDat[i];
								pLumiIntegral += pLumiDat[i];
								len = sprintf( str, "%li\t%lf\t%li\t%li\t%lf\t%li\t%li\xD\xA", i, dTopo*i, pTopoDat[i], pTopoIntegral, dLumi*i, pLumiDat[i], pLumiIntegral );
								_lwrite( hFile, str, len );
							}
						}
						_lclose( hFile );
					}

					// Wieder Freigeben oder wenigstens Warnen, falls nix war ...
					if(  pTopoDat  )
						MemFree( pTopoDat );
					else if(  pLumiDat  )
						MemFree( pLumiDat );
				}
				break;

				// Winkelverteilung berechnen
				case IDM_ORIENTIERUNG:
				{
					LPBILD 		pTopo=NULL;
					LPBMPDATA pNeuBmp;
					LPUWORD		pZiel;
					WORD			iAkt=pBmp->iAktuell;
					LONG			x, y, w, h, maxwinkel;
					BOOL			LOG=FALSE;

					// Erst einmal aktuelle Pointer ermitteln
					if(  pBmp->pSnom[iAkt].Topo.puDaten  )
					{
						w = pBmp->pSnom[iAkt].w;
						h = pBmp->pSnom[iAkt].h;
						if(  (int)pBmp->pSnom[iAkt].Topo.puDaten<256l  )
							pTopo = &(pBmp->pSnom[(int)pBmp->pSnom[iAkt].Topo.puDaten-1].Topo);
						else
							pTopo = &(pBmp->pSnom[iAkt].Topo);
					}

					maxwinkel = DialogBoxParam( hInst, "WinkelDialog", hwnd, WinkelDialog, -90 );
					if(  maxwinkel<0  )
					{
						LOG = TRUE;
						maxwinkel = -maxwinkel;
					}

					if(  pTopo  &&  maxwinkel>0
							 &&	 (pNeuBmp = (LPBMPDATA)pMalloc( sizeof(BMPDATA) ))!=NULL
							 &&  (pZiel = (LPUWORD)pMalloc( sizeof(WORD)*180*180 ))!=NULL  )
					{
						MDICREATESTRUCT mdicreate;
						char					str[256];
						LPUWORD 				pQuelle = pTopo->puDaten;
						WORD					plotx, ploty, max=0;
						// Einfach ein paar Abkuerzungen
						double					fx=pBmp->pSnom[iAkt].fX;
						double					fxf=pTopo->fSkal/(fx);
						double					fy=pBmp->pSnom[iAkt].fY;
						double					fyf=pTopo->fSkal/(fy);
						double					dx, dy, r, alpha;

						hCurSave = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

						// Defaultskalierung anwählen
						wsprintf( (LPSTR)str, GetStringRsc(STR_ORIENT), (LPSTR)pBmp->szName );
						pNeuBmp->pSnom[0].w = 180;
						pNeuBmp->pSnom[0].h = 180;
						pNeuBmp->pSnom[0].fX = maxwinkel/90.0;
						pNeuBmp->pSnom[0].fY = maxwinkel/90.0;
						pNeuBmp->pPsi.cShowOffset = TRUE;
						pNeuBmp->pSnom[0].fXOff = -maxwinkel;
						pNeuBmp->pSnom[0].fYOff = -maxwinkel;
						pNeuBmp->Links = TOPO;
						pNeuBmp->Rechts = NONE;
						pNeuBmp->pSnom[0].Topo.puDaten = pZiel;
						pNeuBmp->pSnom[0].Error.fSkal = 0.0;
						pNeuBmp->pSnom[0].Lumi.fSkal = 0.0;
						mdicreate.szClass = (LPSTR)szBmpClass ;
						mdicreate.szTitle = str;
						mdicreate.hOwner  = hInst;
						mdicreate.x       = CW_USEDEFAULT;
						mdicreate.y       = CW_USEDEFAULT;
						mdicreate.cx      = CW_USEDEFAULT;
						mdicreate.cy      = CW_USEDEFAULT;
						mdicreate.style   = WS_CHILD|WS_BORDER;
						mdicreate.lParam  = (LONG)((LPBMPDATA)pNeuBmp);

						// Daten von verkippten Dreiecken berechnen
						/********************************************************
						 * Es ist p1 links oben, p2 rechts oben, p3 links unten
						 * Der Normalenvektor ist dann
						 *
						 * _          / fZ/fX² (p2-p1) \
						 * n = -fX*fY | fZ/fY² (p3-p1) |
						 *						\       1        /
						 *
						 * n1 und n2 sind (da delta z = 1) der arcus tangens des
						 * Winkels in diese Richtungen ...
						 *
						 * Der Wert der Winkelbitmap wird also an dieser Stelle (=Winkel x, y)
						 * um Eins erhöht. Evt. wird, wenn gewünscht, das Ganze danach noch logarithmiert
						 */
						StatusLineRsc( I_WINKEL );
						for(  y=1;  y<h;  y++  )
						{
							for(  x=1;  x<w;  x++  )
							{
								// Zwei Dreiecke, einmal links und oben
								dx = fxf*((long)pQuelle[x-1]-(long)pQuelle[x]);
								dy = fyf*((long)pQuelle[x-1+w]-(long)pQuelle[x-1]);
								// Winkel und Richtung
								if(  fabs(dy)<1e-10  )
									alpha = 0.0;
								else
									alpha = atan( dx/dy );
								if(  dy<0.0  )
									alpha += M_PI;
								alpha += M_PI;	// Sonst steht's auf dem Kopf ...
								// Richtungswinkel
								r = atan( sqrt( dx*dx + dy*dy ) )*180.0*90.0/(M_PI*maxwinkel);
								plotx = sin(alpha)*r+90.5;
								ploty = cos(alpha)*r+90.5;
								if(  ploty>0  &&  ploty<180  &&  plotx>0  &&  plotx<180  )
								{
									if(  pZiel[plotx+180*ploty]<0xFFFEu  )
										pZiel[plotx+180*ploty] ++;
									if(  pZiel[plotx+180*ploty]>max  )
										max = pZiel[plotx+180*ploty];
								}
#if 0
								// Und rechts und unten ...
								// (liefert aber keine sehr anderen Ergebnisse, deshalb weggelassen)
								dx = fxf*((long)pQuelle[x-1+w]-(long)pQuelle[x+w]);
								dy = fyf*((long)pQuelle[x+w]-(long)pQuelle[x]);
								// Winkel und Richtung
								if(  fabs(dy)<1e-10  )
									alpha = 0.0;
								else
									alpha = atan( dx/dy );
								if(  dy<0.0  )
									alpha += M_PI;
								// Richtungswinkel
								r = atan( sqrt( dx*dx + dy*dy ) )*180.0*90.0/(M_PI*maxwinkel);
								plotx = sin(alpha)*r+90.5;
								ploty = cos(alpha)*r+90.5;
								if(  ploty>0  &&  ploty<180  &&  plotx>0  &&  plotx<180  )
								{
									if(  pZiel[plotx+180*ploty]<0xFFFEu  )
										pZiel[plotx+180*ploty] ++;
									if(  pZiel[plotx+180*ploty]>max  )
										max = pZiel[plotx+180*ploty];
								}
#endif
							}
							pQuelle += w;
						}

						if(  LOG  )
						{
							for(  y=0;  y<180*180l;  y++  )
								pZiel[y] = 23.0*log(pZiel[y]+1);
						}

						// So'n paar defaults setzen
						pTopo = &(pNeuBmp->pSnom[0].Topo);
						if(  LOG  )
						{
							pTopo->uMaxDaten = 23.0*log(max+1)+1;
							pTopo->fSkal = 1.0/23.0;
						}
						else
						{
							pTopo->uMaxDaten = max+1;
							pTopo->fSkal = 1.0/max;
						}
						pTopo->Typ = TOPO;
						pTopo->bPseudo3D = FALSE;
						pTopo->Farben[0] = RGB(255,255,255);
						pTopo->Farben[1] = RGB(127,127,127);
						pTopo->Farben[2] = RGB(0,0,0);
						pTopo->bNoLUT = TRUE;
						pTopo->fStart = 0.0;
						pTopo->fEnde = 100.0;
						pTopo->bSpecialZUnit = TRUE;
						lstrcpy( pTopo->strZUnit, "23 ln(n+1)" );
						lstrcpy( pTopo->strTitel, GetStringRsc(STR_ANGLE_PLOT) );

						// fehlende Werte gleich Interpolieren
						// dabei X-Richtung und Y-Richtung, untere Grenze ist Null, Fehlende Werte lin. interpolieren
						StatusLineRsc( I_DESPIKE );
						BildDespike( pTopo, 180, 180, 10, 10, 0, 0, TRUE, TRUE, TRUE, FALSE, TRUE );

						// Fenster nun darstellen
						ClearStatusLine();
						SetCursor( hCurSave );
						SendMessage (hwndClient, WM_MDICREATE, 0, (long) (LPMDICREATESTRUCT) &mdicreate) ;
						pNeuBmp->wKurzname = 0;
						pNeuBmp->iSaved = 999;
					}
				}
				break;

				// Konturen hervorheben
				case IDM_KONTOUR:
				{
					LONG			diff, x, y, w, h;
					WORD			iAkt=pBmp->iAktuell;
					UWORD			last_kontur, bw, *pLine;
					WORD			iType=NONE;
					LPBILD		pData=NULL;

					// Erst einmal aktuelle Pointer ermitteln
					if(  modus&TOPO  &&  pBmp->pSnom[iAkt].Topo.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Topo.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Topo.puDaten-1].Topo);
						else
							pData = &(pBmp->pSnom[iAkt].Topo);
						iType = TOPO;
					}
					else if(  modus&ERRO  &&  pBmp->pSnom[iAkt].Error.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Error.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Error.puDaten-1].Error);
						else
							pData = &(pBmp->pSnom[iAkt].Error);
						iType = ERRO;
					}
					else if(  modus&LUMI  &&  pBmp->pSnom[iAkt].Lumi.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Lumi.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Lumi.puDaten-1].Lumi);
						else
							pData = &(pBmp->pSnom[iAkt].Lumi);
						iType = LUMI;
					}
					if(  pData==NULL  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					diff = DialogBoxParam( hInst, "KonturDialog", hwnd, StdDialog, (pData->uMaxDaten*pData->fSkal/10.0) );
					if(  diff>0  ) {
						diff = (diff/pData->fSkal);

						w = pBmp->pSnom[iAkt].w;
						h = pBmp->pSnom[iAkt].h;

						// Maskespeicher anlegen
						if(  pBmp->pMaske==NULL  )
						{
							pBmp->wMaskeW = (WORD)(pBmp->pSnom[pBmp->iAktuell].w+7u) / 8;
							pBmp->pMaske = pMalloc( pBmp->wMaskeW*(pBmp->pSnom[pBmp->iAktuell].h) );
							if(  pBmp->pMaske==NULL  )
							{
								pBmp->rectMaske.left = pBmp->rectMaske.right = 0;
								FehlerRsc( E_MEMORY );
								break;
							}
						}
						bw = pBmp->wMaskeW;

						// Zuerst Zeilenweise
						for (y=0; y<h; y++ )
						{
							pLine = pData->puDaten+y*w;
							last_kontur = (*pLine++)/diff;
							for(  x=1;  x<w-1;  x++  )
							{
								if(  *pLine/diff!=last_kontur  )
								{
									last_kontur = (*pLine/diff);
									pBmp->pMaske[(h-1-y)*bw+(x>>3)] |= (BYTE)(0x0C0>>(x&7));
								}
								pLine ++;
							}
						}
						// Dann Spaltenweise
						for(  x=0;  x<w;  x++  )
						{
							pLine = pData->puDaten+x;
							last_kontur = *pLine/diff;
							pLine += w;
							for(  y=1;  y<h-1;  y++  )
							{
								if(  *pLine/diff!=last_kontur  )
								{
									last_kontur = *pLine/diff;
									pBmp->pMaske[(h-1-y)*bw+(x>>3)] |= (0x00C0>>(x&7));
									if(  y<h  )
										pBmp->pMaske[(h-y)*bw+(x>>3)] |= (0x00C0>>(x&7));
								}
								pLine += w;
							}
						}

						InvalidateRect( hwnd, NULL, TRUE );
						ClearStatusLine();
					}
				}
				break;


				// dots manuell zählen
				case IDM_DOTS:
				if(  !pBmp->bCountDots  )
				{
					LONG			x, y, w, h;
					WORD			iAkt=pBmp->iAktuell;
					UWORD			last_kontur, bw, *pLine;
					WORD			iType=NONE;
					LPBILD			pData=NULL;

					// Erst einmal aktuelle Pointer ermitteln
					if(  modus&TOPO  &&  pBmp->pSnom[iAkt].Topo.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Topo.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Topo.puDaten-1].Topo);
						else
							pData = &(pBmp->pSnom[iAkt].Topo);
						iType = TOPO;
					}
					else if(  modus&ERRO  &&  pBmp->pSnom[iAkt].Error.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Error.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Error.puDaten-1].Error);
						else
							pData = &(pBmp->pSnom[iAkt].Error);
						iType = ERRO;
					}
					else if(  modus&LUMI  &&  pBmp->pSnom[iAkt].Lumi.puDaten  )
					{
						if(  (LONG)pBmp->pSnom[iAkt].Lumi.puDaten<256l  )
							pData = &(pBmp->pSnom[(WORD)pBmp->pSnom[iAkt].Lumi.puDaten-1].Lumi);
						else
							pData = &(pBmp->pSnom[iAkt].Lumi);
						iType = LUMI;
					}
					if(  pData==NULL  )
					{
						WarnungRsc( W_NIX );
						break;
					}
					pBmp->dot_radius = DialogBoxParam( hInst, "DotDialog", hwnd, StdDialog, 3 );
					if(  pBmp->dot_radius>0  ) {
						double Median, MedianRMS;

						MeadianArea( GetDataPointer(pBmp,iType), pBmp->pSnom[iAkt].w, 0, 0, pBmp->pSnom[iAkt].w, pBmp->pSnom[iAkt].h, 1.0, pData->uMaxDaten, &Median, &MedianRMS );
						pBmp->dot_mean_level = Median;
						pBmp->dot_quantisation = MedianRMS;
						free( pBmp->dot_histogramm );
						pBmp->dot_histogramm_count = 0;
						pBmp->dot_histogramm = NULL;

						w = pBmp->pSnom[iAkt].w;
						h = pBmp->pSnom[iAkt].h;

						// Maskespeicher anlegen
						if(  pBmp->pMaske==NULL  )
						{
							pBmp->wMaskeW = (WORD)(pBmp->pSnom[pBmp->iAktuell].w+7u) / 8;
							pBmp->pMaske = pMalloc( pBmp->wMaskeW*(pBmp->pSnom[pBmp->iAktuell].h) );
							if(  pBmp->pMaske==NULL  )
							{
								pBmp->rectMaske.left = pBmp->rectMaske.right = 0;
								FehlerRsc( E_MEMORY );
								break;
							}
							pBmp->dot_number = 0;
						}
	
						sprintf( str, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/(pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].w*pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].h) );
						StatusLine( str );
						pBmp->bCountDots = TRUE;
					}
				}
				break;

				case IDM_MATHE:
					DialogBoxParam( hInst, "MatheDialog", hwnd, MatheDialog, (LPARAM)hwnd );
				break;

				case IDM_UMBENNEN:
					if(  DialogBoxParam( hInst, (LPSTR)"StringDialog", hwnd, StringDialog, (LPARAM)(LPSTR)pBmp->szName )  )
					{
						pBmp->wKurzname = lstrlen( pBmp->szName );
						while(  pBmp->wKurzname>0  )
							if(  pBmp->szName[pBmp->wKurzname]=='\\'  ||  pBmp->szName[pBmp->wKurzname]==':'  )
								break;
							else
								pBmp->wKurzname--;
						pBmp->wKurzname++;
						SetWindowText( hwnd, pBmp->szName );
					}
				break;

				case	IDM_AUTOKORRELATION:
				if(  modus&TOPO  &&  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  )
				{
					// for progress bar
					HWND	hProgress, hwndPB;
					RECT	rcClient;
					int		cyVScroll;
					// 2D Autokorrekation
					LPSNOMDATA	pNewSnom, pSnom=&(pBmp->pSnom[pBmp->iAktuell]);
					LPUWORD			puDaten = pSnom->Topo.puDaten;
					LONG				dx, dy, i, j;
					LONG				x, y, w, h, yoff, offset;
					LPLONG			pf1;
					LPFLOAT			pf2;
					float				fMax=0.0, fMin=0.0, fMittel;
					double			fTemp=0;

					if(  (LONG)puDaten<256  )
						puDaten = pBmp->pSnom[(WORD)pBmp->pSnom[pBmp->iAktuell].Topo.puDaten-1].Topo.puDaten;
					WarteMaus();

					if(  (pNewSnom=pAllocNewSnom( pBmp, modus ))!=NULL  )
					{
						pf1 = pMalloc( sizeof(float)*pSnom->w*pSnom->h );
						pf2 = pMalloc( sizeof(float)*pSnom->w*pSnom->h );
#if 1
						/*** Die Autokorrelation ist folgendermaßen definiert:
						 *** G(l,m) := 1/((H-m)*(W-l)) \sum_{j=1}^{H-m} \sum_{i=1}^{W-l} z_{i,j}*z_{i+l,j+m}
						 *** ACHTUNG: Die Einheit ist Länge^2!
						 *** ACHTUNG: <z_i> := 0, sonst abziehen!
						 ***/

							w = pSnom->w;
							h = pSnom->h;

							InitCommonControls(); 
							hProgress = CreateDialog( hInst, "DialogProgress", pBmp->hwnd, StdDialog );
					    GetClientRect(hProgress, &rcClient); 
					    cyVScroll = GetSystemMetrics(SM_CYVSCROLL); 
					    hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPSTR) NULL, WS_CHILD | WS_VISIBLE, rcClient.left, rcClient.bottom - cyVScroll, rcClient.right, cyVScroll, hProgress, (HMENU) 0, hInst, NULL); 
					    // Set the range and increment of the progress bar
							SetWindowText( hProgress, "Calculating 2D autocorrelation ..." );
					    SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0,h+h*(h+1)/20 ) );

							// substract mean value before starting
									//	pSnom->Topo.fSkal lassen wir noch unter den Tisch fallen!
							for(  i=0;  i<w*h;  i++  )
								fTemp += (double)(pf1[i] = puDaten[i]);
							fMittel = fTemp/(w*h);	// mittelwert für Autokorrelation
							for(  i=0;  i<w*h;  i++  )
								pf1[i] -= fMittel;

							offset = 0;
							for(  dy=0;  dy<h;  dy++  )
							{
						    SendMessage(hwndPB, PBM_SETSTEP, (WPARAM)(1+(h-dy)/10), 0); 
				        SendMessage(hwndPB, PBM_STEPIT, 0, 0); 
								Sleep(0);
								for(  dx=0;  dx<w;  dx++  )
								{
									// Calculate now autocorrelation
									fTemp = 0;
									for(  yoff=y=0;  y<h-dy-1;  y++,yoff+=w  )
									{
										for(  x=0;  x<w-dx-1;  x++  )
											fTemp += pf1[x+yoff]*pf1[x+yoff+offset];
									}
									pf2[offset] = fTemp/(double)((h-dy)*(w-dx));
									offset ++;	// one element further ...
								}
							}
							DestroyWindow( hwndPB );
							DestroyWindow( hProgress );
#else
						// Does not work -- phase is lost!!!

						// do use of the autocorrelation funktion!
						w = pSnom->w;
						h = pSnom->h;

						// substract mean value before starting
						for(  i=0;  i<w*h;  i++  )
							fTemp += pf2[i] = puDaten[i]*pSnom->Topo.fSkal;
						fMittel = fTemp/(w*h);	// mittelwert für Autokorrelation
						for(  i=0;  i<w*h;  i++  )
							pf2[i] -= fMittel;

						// do autocorrelation in x axis
						for(  y=0;  y<h;  y++  )
							Autokorrelation( pf1+(y*w), pf2+(y*w), w, TRUE );
						// turn right
						for(  y=0;  y<w;  y++  )
						{
							for(  x=0;  x<h;  x++  )
								pf2[x+y*h] = pf1[(h-x-1)*w+y];
						}
						// beware now w is heigth and h is width!
						// so x ist y ...
						for(  x=0;  x<w;  x++  )
							Autokorrelation( pf1+(x*h), pf2+(x*h), h, TRUE );
						// And now we need to turn this back ...
						for(  y=0;  y<h;  y++  )
						{
							for(  x=0;  x<w;  x++  )
								pf2[x+y*w] = pf1[h*x+y];
						}
						// result is now in pf2!
#endif
						// Neu Skalieren
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
						{
							if(  pf2[i]>fMax  )
								fMax = pf2[i];
							if(  pf2[i]<fMin  )
								fMin = pf2[i];
						}
						fMax -= fMin; // to prevent overflow

						// floats wieder nach WORD
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
							pNewSnom->Topo.puDaten[i] = (pf2[i]-fMin)*65000.0/fMax;

						BildMax( &(pNewSnom->Topo), pSnom->w, pSnom->h );
						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
						MemFree( pf1 );
						MemFree( pf2 );
					}
				}
				break;

				case	IDM_FFT:
				if(  modus&TOPO  &&  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  )
				{
					// 2D fft
					LPSNOMDATA	pNewSnom, pSnom=&(pBmp->pSnom[pBmp->iAktuell]);
					LPUWORD			puDaten = pSnom->Topo.puDaten;
					LONG				x, y, width2=1, height2=1, i;
					LPFLOAT			pf1, pf2;
					float				fMax=0.0, fMin=0.0, fMittel;
					double			fTemp=0;

					if(  (LONG)puDaten<256  )
						puDaten = pBmp->pSnom[(WORD)pBmp->pSnom[pBmp->iAktuell].Topo.puDaten-1].Topo.puDaten;
					WarteMaus();

					// Zweierpotenz!
					while(  width2<pBmp->pSnom[pBmp->iAktuell].w  )
						width2 *= 2;
					while(  height2<pBmp->pSnom[pBmp->iAktuell].h  )
						height2 *= 2;

					for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
						fTemp += puDaten[i];
					fMittel = fTemp/pSnom->w*pSnom->h;	// Mittelwert für FFT

					// Hilfsmatrix initialisieren
					pf1 = pMalloc( sizeof(float)*width2*pSnom->h );
					pf2 = pMalloc( sizeof(float)*width2*height2 );
					for(  y=0;  y<pSnom->h;  y++  )
					{
						for(  x=0;  x<pSnom->w;  x++  )
							pf1[x+y*width2] = puDaten[x+y*pSnom->w] - fMittel;
						for(  ;  x<width2;  x++  )
							pf1[x+y*width2] = 0.0;
					}

					// Endlich loslegen ...
					if(  (pNewSnom=pAllocNewSnom( pBmp, modus ))!=NULL  )
					{
						for(  y=0;  y<pSnom->h;  y++  )
						{
							realft( pf1+(y*width2)-1, width2/2, 1 );
							sprintf( str, "%s %i", "Zeile", y+1 );
							StatusLine( str );
							Sleep(0);
						}
						// Umkopieren
						for(  y=0;  y<pSnom->h;  y++  )
							for(  x=0;  x<width2;  x++  )
								pf2[y+x*height2] = pf1[x+y*width2];
#if 0
						for(  x=0;  x<width2;  x++  )
						{
							realft( pf2+x*height2-1, height2/2, 1 );
							sprintf( str, "%s %i", "Spalte", x+1 );
							StatusLine( str );
							Sleep(0);
						}
#endif
						// Neu Skalieren
						for(  i=0;  i<width2*height2;  i++  )
						{
							if(  pf2[i]>fMax  )
								fMax = pf2[i];
							if(  pf2[i]<fMin  )
								fMin = pf2[i];
						}

						// floats wieder nach WORD
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
							pNewSnom->Topo.puDaten[i] = (pf1[i]-fMin)*65000.0/(fMax-fMin);

						BildMax( &(pNewSnom->Topo), pSnom->w, pSnom->h );
						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
					}
					MemFree( pf1 );
					MemFree( pf2 );
				}
				break;

/**** Hier kommen jetzt einige Funktionen, die mit dem Markieren von Profillinien bzw.
 ****	Auschlussregionen zu tun haben
 ****/

				// Daten in der Ausschlussregion in neues Fenster kopieren
				case IDM_COPY_MASKE:
				if(  pBmp->pMaske!=NULL  )
				{
					MDICREATESTRUCT mdicreate;
					LPBMPDATA				pNeuBmp;
					LPSNOMDATA			pSnom;
					int							iAkt=pBmp->iAktuell;
					LONG						y, x, ymin, ymax, xmin, xmax;
					LPUWORD					pSrc;
					WORD						wMin, wert;

					pNeuBmp = (LPBMPDATA)pMalloc( sizeof(BMPDATA) );
					MemMove( pNeuBmp, pBmp, sizeof(BMPDATA) );
					wsprintf( (LPSTR)str, GetStringRsc(STR_PART_OF), (LPSTR)pBmp->szName );
					pNeuBmp->wKurzname = 0;
					pNeuBmp->pMaske = NULL;
					pNeuBmp->pDib = NULL;
					pNeuBmp->pCacheBits = NULL;
					pNeuBmp->pScanLine = NULL;
					pNeuBmp->pExtra = pMalloc( pBmp->lExtraLen );
					MemMove( pNeuBmp->pExtra, pBmp->pExtra, pBmp->lExtraLen );

					pBmp->bIsScanLine = FALSE;
					pBmp->lMaxScan = 0;
					pBmp->rectScan[0].left = pBmp->rectScan[0].right = 0;
					pBmp->rectScan[0].top =	pBmp->rectScan[0].bottom = 0;

					pSnom = &(pNeuBmp->pSnom[0]);
					MemMove( pSnom, &(pBmp->pSnom[iAkt]), sizeof(SNOMDATA) );
					pNeuBmp->iAktuell = pNeuBmp->iSaved = 0;

					// Die maximalen Ausmasse der neuen Bitmap errechnen
					// Beware: Maske steht auf dem Kopf!
					xmin = pSnom->w;
					ymin = pSnom->h;
					xmax = 0;
					ymax = 0;
					for(  y=0;  y<pSnom->h;  y++  )
						for(  x=0;  x<pSnom->w;  x++  )
						{
							if(  IsMaske( pBmp, x, y )  )
							{
								if(  x<xmin  )
									xmin = x;
								if(  x>xmax  )
									xmax = x;
								if(  y<ymin  )
									ymin = y;
								if(  y>ymax  )
									ymax = y;
							}
						}

					// Und entsprechend eintragen
					pSnom->w = (xmax-xmin);
					pSnom->h = (ymax-ymin);
					pSnom->fXOff += xmin*pSnom->fX;
					pSnom->fYOff += ymin*pSnom->fY;

					// Und nun endlich kopieren ...
					if(  modus&TOPO  &&  pSnom->Topo.Typ==TOPO  &&
							 (pSnom->Topo.puDaten = (LPUWORD)pMalloc( sizeof(WORD)*pSnom->w*pSnom->h ))!=NULL  )
					{
						StatusLineRsc( I_COPY );
						if(  (long)pBmp->pSnom[iAkt].Topo.puDaten<=256  )
							pSrc = pBmp->pSnom[(long)pBmp->pSnom[iAkt].Topo.puDaten-1].Topo.puDaten;
						else
							pSrc = pBmp->pSnom[iAkt].Topo.puDaten;
						wMin = 0xFFFFu;
						for(  y=ymin;  y<ymax;  y++  )
							for(  x=xmin;  x<xmax;  x++  )
								if(  IsMaske( pBmp, x, y )  )
								{
									wert = pSrc[x+(pBmp->pSnom[iAkt].h-y-1)*pBmp->pSnom[iAkt].w];
									if(  wert<wMin  )
										wMin = wert;
									pSnom->Topo.puDaten[(x-xmin)+(ymax-y-1)*pSnom->w] = wert;
								}
						// Neues Minimum anwenden ...
						BildCalcConst( &(pSnom->Topo), pSnom->w, pSnom->h, '-', wMin, TRUE );
					}
					else
					{
						pSnom->Topo.puDaten = NULL;
						pSnom->Topo.Typ = NONE;
					}

					// Und nun endlich kopieren: Fehler ...
					if(  modus&ERRO  &&  pSnom->Error.Typ==ERRO  &&
							 (pSnom->Error.puDaten = (LPUWORD)pMalloc( sizeof(WORD)*pSnom->w*pSnom->h ))!=NULL  )
					{
						StatusLineRsc( I_COPY );
						if(  (long)pBmp->pSnom[iAkt].Error.puDaten<=256  )
							pSrc = pBmp->pSnom[(long)pBmp->pSnom[iAkt].Error.puDaten-1].Error.puDaten;
						else
							pSrc = pBmp->pSnom[iAkt].Error.puDaten;
						wMin = 0xFFFFu;
						for(  y=ymin;  y<ymax;  y++  )
							for(  x=xmin;  x<xmax;  x++  )
								if(  IsMaske( pBmp, x, y )  )
								{
									wert = pSrc[x+(pBmp->pSnom[iAkt].h-y-1)*pBmp->pSnom[iAkt].w];
									if(  wert<wMin  )
										wMin = wert;
									pSnom->Error.puDaten[(x-xmin)+(ymax-y-1)*pSnom->w] = wert;
								}
						// Neues Minimum anwenden ...
						BildCalcConst( &(pSnom->Error), pSnom->w, pSnom->h, '-', wMin, TRUE );
					}
					else
					{
						pSnom->Error.Typ = NONE;
						pSnom->Error.puDaten = NULL;
					}

					// Und nun endlich kopieren: Fehler ...
					if(  modus&LUMI  &&  pSnom->Lumi.Typ==LUMI  &&
							 (pSnom->Lumi.puDaten = (LPUWORD)pMalloc( sizeof(WORD)*pSnom->w*pSnom->h ))!=NULL  )
					{
						StatusLineRsc( I_COPY );
						if(  (long)pBmp->pSnom[iAkt].Lumi.puDaten<=256  )
							pSrc = pBmp->pSnom[(long)pBmp->pSnom[iAkt].Lumi.puDaten-1].Lumi.puDaten;
						else
							pSrc = pBmp->pSnom[iAkt].Lumi.puDaten;
						wMin = 0xFFFFu;
						for(  y=ymin;  y<ymax;  y++  )
							for(  x=xmin;  x<xmax;  x++  )
								if(  IsMaske( pBmp, x, y )  )
								{
									wert = pSrc[x+(pBmp->pSnom[iAkt].h-y-1)*pBmp->pSnom[iAkt].w];
									if(  wert<wMin  )
										wMin = wert;
									pSnom->Lumi.puDaten[(x-xmin)+(ymax-y-1)*pSnom->w] = wert;
								}
						// Neues Minimum anwenden ...
						BildCalcConst( &(pSnom->Lumi), pSnom->w, pSnom->h, '-', wMin, TRUE );
					}
					else
					{
						pSnom->Lumi.Typ = NONE;
						pSnom->Lumi.puDaten = NULL;
					}

					// Defaultskalierung anwählen
					mdicreate.szClass = (LPSTR)szBmpClass ;
					mdicreate.szTitle = str;
					mdicreate.hOwner  = hInst;
					mdicreate.x       = CW_USEDEFAULT;
					mdicreate.y       = CW_USEDEFAULT;
					mdicreate.cx      = CW_USEDEFAULT;
					mdicreate.cy      = CW_USEDEFAULT;
					mdicreate.style   = WS_CHILD|WS_BORDER;
					mdicreate.lParam  = (LONG)((LPBMPDATA)pNeuBmp);

					// Fenster nun darstellen
					SendMessage (hwndClient, WM_MDICREATE, 0, (long) (LPMDICREATESTRUCT) &mdicreate) ;
					pNeuBmp->wKurzname = 0;
					pNeuBmp->iSaved = 999;
					ClearStatusLine();
				}
				break;

				// Daten in der Ausschlussregion horizontal interpolieren
				// Dazu fittet man am besten durch den Rest ein Polynom n-ten Grades ...
				case IDM_MASKE_VERTIKAL_MITTELN:
				if(  pBmp->pMaske!=NULL  )
				{
					LPSNOMDATA pSnom;
					int					grad;

					grad = DialogBoxParam( hInst, "RegionInterpolieren", hwnd, StdDialog, 10 );
					if(  grad==0  )
						break;
					grad --;

					// Neue Bitmaps erstellen
					if(  (pSnom=pAllocNewSnom( pBmp, modus ))==NULL  )
					{
						StatusLineRsc( E_MEMORY  );
						break;
					}

					WarteMaus();

					if(  (LONG)pSnom->Topo.puDaten>256  )
					{
						InterpolateVertikalBild( pBmp, &(pSnom->Topo), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Topo), pSnom->w, pSnom->h );
					}
					if(  (LONG)pSnom->Error.puDaten>256  )
					{
						InterpolateVertikalBild( pBmp, &(pSnom->Error), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Error), pSnom->w, pSnom->h );
					}
					if(  (LONG)pSnom->Lumi.puDaten>256  )
					{
						InterpolateVertikalBild( pBmp, &(pSnom->Lumi), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Lumi), pSnom->w, pSnom->h );
					}

					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					NormalMaus();
				}
				break;

				// Daten in der Ausschlussregion horizontal interpolieren
				// Dazu fittet man am besten durch den Rest ein Polynom n-ten Grades ...
				case IDM_MASKE_HORIZONTAL_MITTELN:
				if(  pBmp->pMaske!=NULL  )
				{
					LPSNOMDATA	pSnom;
					int					grad;

					grad = DialogBoxParam( hInst, "RegionInterpolieren", hwnd, StdDialog, 10 );
					if(  grad==0  )
						break;
					grad --;

					// Neue Bitmaps erstellen
					if(  (pSnom=pAllocNewSnom( pBmp, modus ))==NULL  )
					{
						StatusLineRsc( E_MEMORY  );
						break;
					}

					WarteMaus();

					if(  (LONG)pSnom->Topo.puDaten>256  )
					{
						InterpolateHorizontalBild( pBmp, &(pSnom->Topo), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Topo), pSnom->w, pSnom->h );
					}
					if(  (LONG)pSnom->Error.puDaten>256  )
					{
						InterpolateHorizontalBild( pBmp, &(pSnom->Error), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Error), pSnom->w, pSnom->h );
					}
					if(  (LONG)pSnom->Lumi.puDaten>256  )
					{
						InterpolateHorizontalBild( pBmp, &(pSnom->Lumi), pSnom->w, pSnom->h, grad );
						BildMax( &(pSnom->Lumi), pSnom->w, pSnom->h );
					}

					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					NormalMaus();
				}
				break;

				// Ausschlussregion invertieren
				case IDM_INVERT_MASKE:
					if(  (DWORD)pBmp->pMaske>256  )
					{
						LONG	x, y, w, h;
						LONG	bw;

						w = pBmp->pSnom[pBmp->iAktuell].w;
						h = pBmp->pSnom[pBmp->iAktuell].h;
						w = 0x0000FF00ul>>(w&7);
						bw = pBmp->wMaskeW;
						for(  y=0;  y<h*pBmp->wMaskeW;  y+=pBmp->wMaskeW  )
							for(  x=0;  x<bw;  x++  )
							{
								if(  x<bw-1  )
									pBmp->pMaske[y+x] = ~pBmp->pMaske[y+x];
								else
									pBmp->pMaske[y+x] = (~pBmp->pMaske[y+x])&w;
							}

						RecalcCache( pBmp, TRUE, TRUE );
						InvalidateRect( hwnd, NULL, FALSE );
					}
				break;

				// Ausschlussregion löschen
				case IDM_LOESCHE_MASKE:
					if(  pBmp->pMaske  )
						MemFree( pBmp->pMaske );
					pBmp->pMaske = NULL;
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
				break;
			}
			break;


			case WM_LBUTTONUP:
			FertigScanLine:
				if(  pBmp->bMarkScanLine  )
				{
					RECT	ClientRect;

					ReleaseCapture();
					pBmp->bMarkScanLine = FALSE;
					if(  pBmp->lMaxScan>0  &&  pBmp->rectScan[pBmp->lMaxScan-1].left==pBmp->rectScan[pBmp->lMaxScan-1].right  &&  pBmp->rectScan[pBmp->lMaxScan-1].top==pBmp->rectScan[pBmp->lMaxScan-1].bottom  )
						pBmp->lMaxScan--;
					GetClientRect( hwnd, &ClientRect );
					InvalidateRect( hwnd, NULL, FALSE );
					ClearStatusLine();
					SendMessage( hwnd, WM_RECALC_SLIDER, 0, (DWORD)ClientRect.right | (((DWORD)ClientRect.bottom)<<16ul) );
				}
			break;

			case WM_RBUTTONUP:
			FertigMaske:
				if(  pBmp->bAddMaske  )
				{
					HBRUSH		hOld;
					LONG			x, y;

					ReleaseCapture();
					pBmp->bAddMaske = FALSE;
					hdc = GetDC( hwnd );
					SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ ), GetScrollPos( hwnd, SB_VERT), NULL );
#if 0
					SelectObject( hdc, GetStockObject( NULL_PEN ) );
					SetROP2( hdc, R2_XORPEN );
					hOld = SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungLinks ) );
					if(  pBmp->rectLinks.left  )
						Rectangle( hdc, (pBmp->rectLinks.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectLinks.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
					DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungRechts ) ) );
					if(  pBmp->rectRechts.left  )
						Rectangle( hdc, (pBmp->rectRechts.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectRechts.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.bottom)*pBmp->fZoom );

					if(  pBmp->rectMaske.top!=pBmp->rectMaske.bottom  &&  pBmp->rectMaske.left!=pBmp->rectMaske.right  )
					{
						// Rechteck dauerhaft hinzufügen
						SetBkMode( hdc, TRANSPARENT );
						SetROP2( hdc, R2_COPYPEN );
						DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungLinks ) ) );
						if(  pBmp->rectLinks.left  )
							Rectangle( hdc, (pBmp->rectLinks.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectLinks.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
						DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungRechts ) ) );
						if(  pBmp->rectRechts.left  )
							Rectangle( hdc, (pBmp->rectRechts.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectRechts.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
						DeleteObject( SelectObject( hdc, hOld ) );
						ReleaseDC( hwnd, hdc );
#else
					if(  pBmp->rectMaske.top!=pBmp->rectMaske.bottom  &&  pBmp->rectMaske.left!=pBmp->rectMaske.right  )
					{
#endif
						// Falls nötig neuen Speicher beschaffen
						if(  pBmp->pMaske  &&  pBmp->wMaskeW!=((pBmp->pSnom[pBmp->iAktuell].w+7u)/8)  )
						{
							MemFree( pBmp->pMaske );
							pBmp->pMaske = NULL;
						}
						if(  pBmp->pMaske==NULL  )
						{
							pBmp->wMaskeW = (WORD)(pBmp->pSnom[pBmp->iAktuell].w+7u) / 8;
							pBmp->pMaske = pMalloc( pBmp->wMaskeW*(pBmp->pSnom[pBmp->iAktuell].h) );
							if(  pBmp->pMaske==NULL  )
							{
								pBmp->rectMaske.left = pBmp->rectMaske.right = 0;
								FehlerRsc( E_MEMORY );
								break;
							}
						}
						// Koordinaten evt. vertauschen
						if(  pBmp->rectMaske.left>pBmp->rectMaske.right  )
						{
							y = pBmp->rectMaske.left;
							pBmp->rectMaske.left = pBmp->rectMaske.right;
							pBmp->rectMaske.right = y;
						}
						// Bitmaps stehen leider bei Windows auf dem Kopf ...
						pBmp->rectMaske.top = pBmp->pSnom[pBmp->iAktuell].h-pBmp->rectMaske.top;
						pBmp->rectMaske.bottom = pBmp->pSnom[pBmp->iAktuell].h-pBmp->rectMaske.bottom;
						// Koordinaten evt. vertauschen
						if(  pBmp->rectMaske.top>pBmp->rectMaske.bottom  )
						{
							y = pBmp->rectMaske.top;
							pBmp->rectMaske.top = pBmp->rectMaske.bottom;
							pBmp->rectMaske.bottom = y;
						}
						// und Rechteck zur Maske hinzufügen
						for(  y=pBmp->rectMaske.top;  y<pBmp->rectMaske.bottom;  y++  )
							for(  x=pBmp->rectMaske.left;  x<pBmp->rectMaske.right;  x++  )
								pBmp->pMaske[y*pBmp->wMaskeW+(x/8)] |= 0x80>>(x%8);
						pBmp->IsDirty = TRUE;
					}
					ClearStatusLine();
					InvalidateRect( hwnd, NULL, FALSE );
				}
			break;

			// Linie/Rechteck aufziehen
			case WM_MOUSEMOVE:
			{
				POINT		pt;

				// Fall Maus innerhalb der Bitmap bewegt, dann Kreuz als Mauszeiger
				GetCursorPos( &pt );
				ScreenToClient( hwnd, &pt );
				pt.x = (pt.x+GetScrollPos( hwnd, SB_HORZ ))/pBmp->fZoom;
				pt.y = (pt.y+GetScrollPos( hwnd, SB_VERT ))/pBmp->fZoom;
				if(  pt.x==OldPt.x  &&  pt.y==OldPt.y  )
					break;
				MemMove( &OldPt, &pt, sizeof(POINT) );
				if(  PointInRect(&pBmp->rectLinks,pt) ||  PointInRect(&pBmp->rectRechts,pt)  ||  PointInRect(&pBmp->rectPlot,pt)  )
				{
					pBmp->bMouseMoveMode = TRUE;
					SetCursor( LoadCursor (NULL, IDC_CROSS) );
				}
				else
				{
					if(  pBmp->bMouseMoveMode  )
					{
						pBmp->bMouseMoveMode = FALSE;
						SetCursor( LoadCursor (NULL, IDC_ARROW) );
						if(  pBmp->bCountDots  ) {
							sprintf( str, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/(pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].w*pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].h) );
							StatusLine( str );
						}
						else {
							ClearStatusLine();
						}
					}
				}

				// Evt. Markierung fertigstellen
				if(  pBmp->bMarkScanLine  &&  !(wParam&MK_LBUTTON)  )
					goto	FertigScanLine;
				if(  pBmp->bAddMaske  &&  !(wParam&MK_RBUTTON)  )
					goto	FertigMaske;

				// Ansonsten einfach markieren
				if(  pBmp->bMarkScanLine  &&  wParam&MK_LBUTTON  )
				{
					double	dWinkel;
					int			iScan=pBmp->lMaxScan-1;

					hdc = GetDC( hwnd );
					SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ), GetScrollPos( hwnd, SB_VERT), NULL );
					SetROP2( hdc, R2_XORPEN );
					DrawScanLine( hdc, pBmp, 1.0/pBmp->fZoom );

					// Fall Maus innerhalb der Bitmap bewegt, dann updaten
					if(  PointInRect(&pBmp->rectLinks,pt)  )
					{
						pBmp->rectScan[iScan].right = (pt.x-pBmp->rectLinks.left);
						pBmp->rectScan[iScan].bottom = (pt.y-pBmp->rectLinks.top);
					}
					if(  PointInRect(&pBmp->rectRechts,pt)  )
					{
						pBmp->rectScan[iScan].right = (pt.x-pBmp->rectRechts.left);
						pBmp->rectScan[iScan].bottom = (pt.y-pBmp->rectRechts.top);
					}

					{
						int			iSc=(iScan<=0?0:iScan);
						double	dDist=sqrt( pow((pBmp->rectScan[iSc].bottom-pBmp->rectScan[iSc].top)*pBmp->pSnom[pBmp->iAktuell].fY,2) + pow((pBmp->rectScan[iSc].right-pBmp->rectScan[iSc].left)*pBmp->pSnom[pBmp->iAktuell].fX,2) );

						if(  pBmp->rectScan[iSc].left==pBmp->rectScan[iSc].right  )
							dWinkel = 90.0;
						else
							dWinkel = atan( (double)(pBmp->rectScan[iSc].bottom-pBmp->rectScan[iSc].top)/(double)(pBmp->rectScan[iSc].left-pBmp->rectScan[iSc].right)  )*180.0/M_PI;

						sprintf( str, GetStringRsc(STR_SCANLINE), pBmp->rectScan[iSc].left, pBmp->rectScan[iSc].top, pBmp->rectScan[iSc].right, pBmp->rectScan[iSc].bottom, (int)(dWinkel+0.5), (int)dDist );
						StatusLine( str );
					}
					DrawScanLine( hdc, pBmp, 1.0/pBmp->fZoom );
					if(  !bMouseMove  )
					{
						bMouseMove = TRUE;
						DrawScanLinePlot( hdc, pBmp, 1.0/pBmp->fZoom, TRUE );
						bMouseMove = FALSE;
					}

					ReleaseDC( hwnd, hdc );
					break;
					// Das war die/eine Scanlinie
				}

				// Sonst evt. Maske weiter markieren
				if(  pBmp->bAddMaske  &&  wParam&MK_RBUTTON  )
				{
					HBRUSH	hOld;

					hdc = GetDC( hwnd );
					SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ), GetScrollPos( hwnd, SB_VERT), NULL );
					SelectObject( hdc, GetStockObject( NULL_PEN ) );

					// keine Transparenten Hintergrund mehr, damit man auch auf 256 Farben-Bildschirmen etwas sieht
					SetROP2( hdc, R2_XORPEN );
					hOld = SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungLinks ) );
					if(  pBmp->rectLinks.left  )
						Rectangle( hdc, (pBmp->rectLinks.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectLinks.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
					if(  pBmp->rectRechts.left  )
					{
						DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungRechts ) ) );
						Rectangle( hdc, (pBmp->rectRechts.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectRechts.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
						DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungLinks ) ) );
					}

					// Falls Maus innerhalb der Bitmap bewegt, dann updaten
					if(  pBmp->rectLinks.left  &&  PointInRect(&pBmp->rectLinks,pt)  )
					{
						pBmp->rectMaske.right = (pt.x-pBmp->rectLinks.left);
						pBmp->rectMaske.bottom = (pt.y-pBmp->rectLinks.top);
					}
					if(  pBmp->rectRechts.left  &&  PointInRect(&pBmp->rectRechts,pt)  )
					{
						pBmp->rectMaske.right = (pt.x-pBmp->rectRechts.left);
						pBmp->rectMaske.bottom = (pt.y-pBmp->rectRechts.top);
					}

					if(  pBmp->rectLinks.left  )
						Rectangle( hdc, (pBmp->rectLinks.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectLinks.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectLinks.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
					if(  pBmp->rectRechts.left  )
					{
						DeleteObject( SelectObject( hdc, CreateHatchBrush( HS_FDIAGONAL, cMarkierungRechts ) ) );
						Rectangle( hdc, (pBmp->rectRechts.left+pBmp->rectMaske.left)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.top)*pBmp->fZoom, (pBmp->rectRechts.left+pBmp->rectMaske.right)*pBmp->fZoom, (pBmp->rectRechts.top+pBmp->rectMaske.bottom)*pBmp->fZoom );
					}
					{
						BYTE	pStr[256];
						sprintf( (LPSTR)pStr, GetStringRsc(STR_MASK), pBmp->rectMaske.left, pBmp->rectMaske.top, pBmp->rectMaske.right, pBmp->rectMaske.bottom, abs(pBmp->rectMaske.left-pBmp->rectMaske.right), abs(pBmp->rectMaske.top-pBmp->rectMaske.bottom), (double)abs(pBmp->rectMaske.left-pBmp->rectMaske.right)*pBmp->pSnom[pBmp->iAktuell].fX, (double)abs(pBmp->rectMaske.top-pBmp->rectMaske.bottom)*pBmp->pSnom[pBmp->iAktuell].fY );
						StatusLine( pStr );
					}
					DeleteObject( SelectObject( hdc, hOld ) );
					ReleaseDC( hwnd, hdc );
					break;
				}

				// sonst einfach nur Position anzeigen
				if(  pBmp->bMouseMoveMode  )
				{
					// Falls Maus innerhalb der Bitmap bewegt, dann updaten
					if(  PointInRect(&pBmp->rectLinks,pt)  )
					{
						LPWORD	pData=GetDataPointer(pBmp,pBmp->Links);
						LPBILD	pBild=GetBildPointer(pBmp,pBmp->Links);
						LPSNOMDATA	pSnom=&(pBmp->pSnom[pBmp->iAktuell]);
						WORD		x = pt.x-pBmp->rectLinks.left;
						WORD		y = pt.y-pBmp->rectLinks.top;
						if(  x<pSnom->w  &&  y<pSnom->h  &&  pData!=0l  &&  pBild!=0l  )
							sprintf( (LPSTR)str, "x(%i)=%.2f nm  y(%i)=%.2f nm  z=%.2f %s", (int)x, (double)x*pSnom->fX, (int)y, (double)y*pSnom->fY, /*pData[x+(y*pSnom->w)],*/ pData[x+(y*pSnom->w)]*pBild->fSkal, pBild->strZUnit );
					}
					if(  PointInRect(&pBmp->rectRechts,pt)  )
						wsprintf( (LPSTR)str, "x:%i   y:%i", (int)(pt.x-pBmp->rectRechts.left), (int)(pt.y-pBmp->rectRechts.top) );
					if(  PointInRect(&pBmp->rectPlot,pt)  )
						sprintf( (LPSTR)str, "x:%.2lf   y:%.3lf", pBmp->fScanBorder[0]+(pt.x-pBmp->rectPlot.left)*pBmp->fScanBorder[2]/(double)(pBmp->rectPlot.right-pBmp->rectPlot.left), pBmp->fScanBorder[1]+(pt.y-pBmp->rectPlot.top)*pBmp->fScanBorder[3]/(double)(pBmp->rectPlot.bottom-pBmp->rectPlot.top) );
					if(  pBmp->bCountDots  ) {
						char str2[256];
						double size = pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].w*pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].h;
						strcpy(  str2, str );
						sprintf( str, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/size );
						strcat( str, str2 );
					}
					StatusLine( (LPSTR)str );
				}
			}
			break;

			// Start Maske (=Ausschlussregion) markieren
			case WM_RBUTTONDOWN:
			if(  pBmp->bCountDots  ) {
				if(  pBmp->dot_number>0  &&  MessageBox( hwnd, "Do you want to save the results?", NULL, MB_ICONQUESTION|MB_YESNO )==IDYES  ) {
					HFILE hFile;
					OFSTRUCT	of;

					szFselHelpStr = STR_HFILE_HIST;
					lstrcpy( str, pBmp->szName );
					ChangeExt( str, ".hst" );
					if(  CMUFileSave( hwndFrame, STR_SAVE_HIST, str, STR_FILE_ASCII, NULL )  &&
						(hFile=OpenFile(str,&of,OF_CREATE))!=HFILE_ERROR  ) {
						LPBILD	pBild=GetBildPointer(pBmp,pBmp->Links);
						int i;
						
						int len = sprintf( str, "List of dot heights\xD\xA" );
						_lwrite( hFile, str, len );

						len = sprintf( str, "zero=%f +/- %f\xD\xA", pBmp->dot_mean_level*pBild->fSkal, pBmp->dot_quantisation*pBild->fSkal );
						_lwrite( hFile, str, len );

						len = sprintf( str, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/(pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].w*pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].h) );
						_lwrite( hFile, str, len );
						_lwrite( hFile, "\xD\xA", 2 );

						for(  i=0;  i<pBmp->dot_number;  i++  ) {
							len = sprintf( str, "%lf\xD\xA", pBild->fSkal*pBmp->dot_histogramm[i] );
							_lwrite( hFile, str, len );
						}
						_lclose( hFile );
					}

					
				}
				pBmp->bCountDots = FALSE;
				break;
			}
			// now mark regions
			{
				POINT	pt;

				if(  pBmp->rectLinks.left==0  &&  pBmp->rectRechts.left==0  )
					break;
				bMouseMove = FALSE;

				pt.x = (LOWORD(lParam)+GetScrollPos( hwnd, SB_HORZ ))/pBmp->fZoom;
				pt.y = (HIWORD(lParam)+GetScrollPos( hwnd, SB_VERT ))/pBmp->fZoom;
				if(  !pBmp->bAddMaske  )
				{
					if(  PointInRect(&pBmp->rectLinks,pt)  )
					{
						pBmp->rectMaske.left = pBmp->rectMaske.right = (pt.x-pBmp->rectLinks.left);
						pBmp->rectMaske.top = pBmp->rectMaske.bottom = (pt.y-pBmp->rectLinks.top);
						pBmp->bAddMaske = TRUE;
						SetCapture( hwnd );
					}
					else if(  PointInRect(&pBmp->rectRechts,pt)  )
					{
						pBmp->rectMaske.left = pBmp->rectMaske.right = (pt.x-pBmp->rectRechts.left);
						pBmp->rectMaske.top = pBmp->rectMaske.bottom = (pt.y-pBmp->rectRechts.top);
						pBmp->bAddMaske = TRUE;
						SetCapture( hwnd );
					}
					else
						pBmp->bAddMaske = FALSE;
				}
			}
			break;

			// Start Scanlinie markieren
			// Dazu werden einfach die passenden Strukturen ausgefüllt
			case WM_LBUTTONDOWN:
			if(  pBmp->bCountDots  ) {
				POINT	pt;
				int x, y;
				pt.x = (LOWORD(lParam)+GetScrollPos( hwnd, SB_HORZ ))/pBmp->fZoom;
				pt.y = (HIWORD(lParam)+GetScrollPos( hwnd, SB_VERT ))/pBmp->fZoom;
				if(  PointInRect(&pBmp->rectLinks,pt)  ) {
					LPWORD		pData = GetDataPointer(pBmp,pBmp->Links);
					LPSNOMDATA	pSnom = &(pBmp->pSnom[pBmp->iAktuell]);
					LPBILD		pBild=GetBildPointer(pBmp,pBmp->Links);
					// offset and invert h
					pt.x -= pBmp->rectLinks.left;
					pt.y = pSnom->h - (pt.y-pBmp->rectLinks.top);
					// add to histogramm
					if(  pBmp->dot_histogramm_count<=pBmp->dot_number+1  ) {
						pBmp->dot_histogramm = realloc( pBmp->dot_histogramm, sizeof(UWORD)*(pBmp->dot_histogramm_count+512) );
						pBmp->dot_histogramm_count += 512;
					}
					pBmp->dot_histogramm[pBmp->dot_number] = pData[pt.x+((pSnom->h-pt.y)*pSnom->w)];
					// mask
					if(  pBmp->pMaske  ) {
						double size = pSnom->fX*pSnom->w*pSnom->fY*pSnom->h;
						for(  y=max(0,pt.y-pBmp->dot_radius);  y<pt.y+pBmp->dot_radius  &&  y<pBmp->pSnom[pBmp->iAktuell].h;  y++  ) {
							for(  x=max(0,pt.x-pBmp->dot_radius);  x<pt.x+pBmp->dot_radius  &&  x<pBmp->pSnom[pBmp->iAktuell].w;  x++  ) {
								pBmp->pMaske[y*pBmp->wMaskeW+(x/8)] |= 0x80>>(x%8);
							}
						}
						pBmp->dot_number ++;
						InvalidateRect( hwnd, NULL, FALSE );
						{
							char str2[256];
							sprintf( str2, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/size );
							sprintf( (LPSTR)str, "%sx(%i)=%.2f nm  y(%i)=%.2f nm  z=%.2f %s", str2, (int)pt.x, (double)pt.x*pSnom->fX, (int)(pSnom->h-pt.y), (double)(pSnom->h-pt.y)*pSnom->fY, /*pData[x+(y*pSnom->w)],*/ pBmp->dot_histogramm[pBmp->dot_number-1]*pBild->fSkal, pBild->strZUnit );
						}
						StatusLine( str );
						pBmp->IsDirty = TRUE;
						break;
					}
					else {
						pBmp->bCountDots = FALSE;
					}
				}
			}
			// ok, no masking any more ...
			{
				POINT	pt;
				int		iScan=pBmp->lMaxScan-1;

				if(  pBmp->rectLinks.left==0  &&  pBmp->rectRechts.left==0  )
					break;
				bMouseMove = FALSE;

				pt.x = (LOWORD(lParam)+GetScrollPos( hwnd, SB_HORZ ))/pBmp->fZoom;
				pt.y = (HIWORD(lParam)+GetScrollPos( hwnd, SB_VERT ))/pBmp->fZoom;
				if(  !pBmp->bMarkScanLine  )
				{
					if(  PointInRect(&pBmp->rectLinks,pt)  )
					{
						// Neue Scanline hinzufügen?
						if(  wParam&MK_SHIFT)
						{
							if(  pBmp->lMaxScan<4  )
							{
								pBmp->lMaxScan++;
								iScan ++;
							}
							else
								MemMove( &pBmp->rectScan[0], &pBmp->rectScan[1], sizeof(RECT)*3 );
						}
						else if(  iScan<0  )
						{
							iScan = 0;
							pBmp->lMaxScan = 1;
						}
						pBmp->rectScan[iScan].left = pBmp->rectScan[iScan].right = (pt.x-pBmp->rectLinks.left);
						pBmp->rectScan[iScan].top = pBmp->rectScan[iScan].bottom = (pt.y-pBmp->rectLinks.top);
						pBmp->bMarkScanLine = TRUE;
						SetCapture( hwnd );
					}
					else if(  PointInRect(&pBmp->rectRechts,pt)  )
					{
						// Neue Scanline hinzufügen?
						if(  wParam&MK_SHIFT)
						{
							if(  pBmp->lMaxScan<4  )
							{
								pBmp->lMaxScan++;
								iScan ++;
							}
							else
								MemMove( &(pBmp->rectScan[0]), &(pBmp->rectScan[1]), sizeof(RECT)*3 );
						}
						else if(  iScan<0  )
						{
							iScan = 0;
							pBmp->lMaxScan = 1;
						}
						pBmp->rectScan[iScan].left = pBmp->rectScan[iScan].right = (pt.x-pBmp->rectRechts.left);
						pBmp->rectScan[iScan].top = pBmp->rectScan[iScan].bottom = (pt.y-pBmp->rectRechts.top);
						pBmp->bMarkScanLine = TRUE;
						SetCapture( hwnd );
					}
					else
						pBmp->bMarkScanLine = FALSE;
				}
			}
			break;

		case WM_PAINT:
		{
			HWND	TopHwnd=NULL;
			RECT	pCoord;

			hdc = BeginPaint (hwnd, &ps) ;
			if(  pBmp->IsDirty  )
				RecalcCache( pBmp, TRUE, TRUE );
			if(  hwnd==GetTopWindow(hwndClient)  )
			{
				EnableUndoRedo( pBmp->iAktuell>0, pBmp->iAktuell<pBmp->iMax-1 );
				hCurrentPal = hwnd;
			}
			else
				TopHwnd = GetTopWindow(hwndClient);
			if(  IsIconic(hwnd)  )
			{
				// Icon neu zeichnen
				GetClientRect( hwnd, &pCoord );
				DisplayDib( hdc, pBmp->pDib, TopHwnd, &pCoord, 0, pBmp->pCacheBits );
			}
			else
			{
				// Fenster neu zeichnen
				if(  wZoomFaktor==0  ||  wZoomFaktor==5  )	// Fit image to window or double size
				{
					double	xf, yf, fZoom;

					if(  wZoomFaktor==0  )
					{
						SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ), GetScrollPos( hwnd, SB_VERT), NULL );
						GetClientRect( hwnd, &pCoord );
						xf = (double)pCoord.right/(double)pBmp->rectFenster.right;
						yf = (double)pCoord.bottom/(double)pBmp->rectFenster.bottom;
						if(  xf<yf  )
							fZoom = xf;
						else
							fZoom = yf;
						pBmp->fZoom = fZoom;
					}
					else
					{
						SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ), GetScrollPos( hwnd, SB_VERT), NULL );
						GetClientRect( hwnd, &pCoord );
						pBmp->fZoom = fZoom = 2.0;
					}
					pCoord.right = pBmp->pDib->bmiHeader.biWidth * fZoom;
					pCoord.bottom = pBmp->pDib->bmiHeader.biHeight * fZoom;
					DisplayDib( hdc, pBmp->pDib, TopHwnd, &pCoord, 0, pBmp->pCacheBits );
					DrawScanLine( hdc, pBmp, 1.0/fZoom );
					DrawScanLinePlot( hdc, pBmp, 1.0/fZoom, TRUE );
					//  pBmp->fZoom = fZoom;	// Damit die nächste Scanline richtig markiert wird ...
				}
				else
				{
					// Fenster neu zeichnen
					SetWindowOrgEx( hdc, GetScrollPos( hwnd, SB_HORZ), GetScrollPos( hwnd, SB_VERT), NULL );
#if 0 // This is a test for copy only !! (but displays well anyway ... )
if(wZoomFaktor==1){
RECT xywh={0,0,512,512};
SetMapMode( hdc, MM_TEXT );
DrawInDC( hdc, pBmp, FALSE, TRUE, &xywh );
DrawScanLine( hdc, pBmp, 1.0 );
DrawScanLinePlot( hdc, pBmp, 1.0, FALSE );}else{
#endif
					DisplayDib( hdc, pBmp->pDib, TopHwnd, NULL, wZoomFaktor, pBmp->pCacheBits );
					DrawScanLine( hdc, pBmp, wZoomFaktor );
					DrawScanLinePlot( hdc, pBmp, wZoomFaktor, TRUE );
					pBmp->fZoom = 1.0/(double)wZoomFaktor;
				}
			}
			EnableMenuItem( hMenuBmp, IDM_SCANLINE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );
			SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY_SCANLINE, FALSE );
			if(  pBmp->bIsScanLine  )
			{
				EnableMenuItem( hMenuBmp, IDM_SCANLINE, MF_BYCOMMAND|MF_ENABLED );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY_SCANLINE, TRUE );
			}
			if(  !pBmp->bCountDots  ) {
				ClearStatusLine();
			}
			EndPaint( hwnd, &ps );
		}
		return 0 ;

		case WM_MDIACTIVATE:
		{
			// Eingabefokus wechselt hierher?
			if ((HWND)lParam == hwnd)
			{
				// Zeigen, welche Angezeigt werden
				if(  pBmp->Links  ||  pBmp->Rechts  )
				{
					WORD	i;

					show = pBmp->Links|pBmp->Rechts;
					for(  i=1;  i<8;  i<<=1  )
					{
						if(  show&i  )
							CheckMenuItem( hMenuBmp, IDM_SHOW+i, MF_CHECKED );
						else
							CheckMenuItem( hMenuBmp, IDM_SHOW+i, MF_UNCHECKED );
					}
				}

#ifndef BIT32
				SendMessage (hwndClient, WM_MDISETMENU, 0, MAKELONG (hMenuBmp, hMenuBmpWindow));
#else
				SendMessage (hwndClient, WM_MDISETMENU, (WPARAM)hMenuBmp, 0 );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_SAVE, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DRUCKEN, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DREHEN, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_ZEILENMITTEL, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MATHE, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_FALSCHFARBEN, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_3DANSICHT, TRUE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MASZE, TRUE );
				// Wahlpunkt mit oder ohne Häkchen?
				EnableUndoRedo( pBmp->iAktuell>0, pBmp->iAktuell<pBmp->iMax-1 );
#endif
				hdc = GetDC( hwnd );
				if(  GetDeviceCaps( hdc, NUMCOLORS )<=256  )
					RedrawAll( UPDATE );
				ReleaseDC( hwnd, hdc );
			}

			// Eingabefokus wechselt? -> INIT-Menü setzen
			if ((HWND)wParam == hwnd)
			{
#ifndef BIT32
				SendMessage (hwndClient, WM_MDISETMENU, 0, MAKELONG (hMenuInit, hMenuInitWindow));
#else
				SendMessage (hwndClient, WM_MDISETMENU, (WPARAM)hMenuInit, 0 );
				// Disable some entries
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_SAVE, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DRUCKEN, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_UNDO, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_REDO, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DREHEN, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_ZEILENMITTEL, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MATHE, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_FALSCHFARBEN, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_3DANSICHT, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MASZE, FALSE );
				SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY_SCANLINE, FALSE );
				EnableMenuItem( hMenuBmp, IDM_SCANLINE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );
#endif
			}
			DrawMenuBar (hwndFrame) ;
		}
		return 0 ;

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			BYTE	datei[256], Typ=1;

			if (IsIconic(hwnd))
				SendMessage(GetParent(hwnd), WM_MDIRESTORE, (WPARAM)hwnd, 0L);
			lstrcpy( (LPSTR)datei, (LPSTR)pBmp->szName );
			if(  pBmp->iAktuell!=pBmp->iSaved  )
			{
				wsprintf( str, GetStringRsc( I_SAVE_FILE ), (LPSTR)datei );
				wButton = MessageBox (hwndFrame, str, STR_SAVE_FILE, MB_ICONQUESTION|MB_YESNOCANCEL);
				if(  wButton==IDCANCEL  )
					return FALSE;
				szFselHelpStr = STR_HFILE_SAVE;
				// speichern oder verwerfen
				if(  wButton==IDYES  &&  CMUFileSave(hwndFrame,STR_SAVE_FILE,datei,STR_FILE_SAVE_NAMES,&Typ)  )
				{
					switch( Typ )
					{
						case 4:
							WriteRHK( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, datei );
							break;
						case 3:
							WriteDigital( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, datei );
							break;
						case 2:
						case 1:
							WriteHDF( pBmp, pBmp->iAktuell, TOPO|ERRO|LUMI, datei, 2-Typ );
							return;
						case 0:
						return 0;	// war nicht erfolgreich: dann Abbruch!
					}
					ClearStatusLine();
				}
			}
			ClearStatusLine();

			// Alles freigeben
			MemFree( pBmp->pDib );
			while(  pBmp->iMax-->0)
			{
				if(  (LONG)pBmp->pSnom[pBmp->iMax].Topo.puDaten>256  )
					MemFree( pBmp->pSnom[pBmp->iMax].Topo.puDaten );
				if(  (LONG)pBmp->pSnom[pBmp->iMax].Error.puDaten>256  )
					MemFree( pBmp->pSnom[pBmp->iMax].Error.puDaten );
				if(  (LONG)pBmp->pSnom[pBmp->iMax].Lumi.puDaten>256  )
					MemFree( pBmp->pSnom[pBmp->iMax].Lumi.puDaten );
			}
			pBmp->iAktuell = 0;
		}
		break ;   // -> weiter mit DefMDIChildProc

		case WM_DESTROY:
			MemFree( (LPBMPDATA)GetWindowLong( hwnd, 0 ) );
			return 0 ;
	}
	// Weitergabe an DefMDIChildProc (ersetzt DefWindowProc)
	return DefMDIChildProc (hwnd, message, wParam, lParam) ;
}




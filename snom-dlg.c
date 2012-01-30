/****************************************************************************************
****	Dialogverwaltung für SNOMPUTZ
****************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>


#include "myportab.h"
#include "filebox.h"
#include "snomputz.h"
#include "snom-var.h"
#include "snomlang.h"

#include "snom-dlg.h"
#include "snom-dat.h"
#include "snom-dsp.h"
#include "snom-mem.h"
#include "snom-mat.h"
#include "snom-wrk.h"
#include "snom-win.h"
#include "snom-dsp.h"
#include "snom-mat.h"
#include "snom-fmax.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif


/************************************************************************************/

// Windows-Fehlermeldung im Klartext ... (hoffentlich)
DWORD GetLastErrorBox( HWND hWnd, LPSTR lpTitle )
{
	LPVOID lpv;
	DWORD dwRv;

	if( GetLastError() == 0 ) {
		return ( 0 ) ;
	}

	dwRv = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	                      FORMAT_MESSAGE_FROM_SYSTEM,
	                      NULL,
	                      GetLastError(),
//                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
	                      MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
	                      (LPVOID)&lpv,
	                      0,
	                      NULL );

	MessageBox( hWnd, lpv, lpTitle, MB_OK );

	if( dwRv ) {
		LocalFree( lpv );
	}

	SetLastError( 0 );
	return ( dwRv ) ;
}


/*************************************************************************************************/

/* Flächenmittelung-Dialog */
BOOL WINAPI Mittel3DDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static HWND hwnd;
	static LPBMPDATA pBmp;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pBmp == NULL ) {
				EndDialog( hdlg, wParam != IDOK );
			}

			if( !WhatToDo( pBmp, modus ) ) {
				StatusLineRsc( W_NIX );
				EndDialog( hdlg, wParam != IDOK );
			}
			CheckRadioButton( hdlg, MITTEL_0, MITTEL_2, MITTEL_2 );
			SetDlgItemInt( hdlg, MITTEL_N, 2, FALSE );
			break;

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_PLANES );
					break;

				case IDOK:
				{
					LPSNOMDATA pSnom;
					WORD n, what, err = TRUE;
					HCURSOR	hCurSave;

					// Neue Bitmaps erstellen
					StatusLineRsc( I_ZEILENMITTEL );
					if( ( pSnom = pAllocNewSnom( pBmp, modus ) ) == NULL ) {
						EndDialog( hdlg, FALSE );
						return ( FALSE ) ;
					}
					EndDialog( hdlg, TRUE );
					hCurSave = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

					// Ordnung feststellen
					if( IsDlgButtonChecked( hdlg, MITTEL_0 ) ) {
						what = 1;
					}
					if( IsDlgButtonChecked( hdlg, MITTEL_1 ) ) {
						what = 2;
					}
					if( IsDlgButtonChecked( hdlg, MITTEL_2 ) ) {
						what = 3;
					}
					n = GetDlgItemInt( hdlg, MITTEL_N, NULL, FALSE );

					if( n > 0 ) {
						// Hier 3D-Mittelung mit Polynom n-ter Ordnung (n<10)

						// Topografie mitteln?
						if( (LONG)pSnom->Topo.puDaten > 256 ) {
							err = MittelFit3DBild( pBmp, &( pSnom->Topo ), pSnom->w, pSnom->h, n, what );
						}
						// Fehler mitteln?
						if( err  &&  (LONG)pSnom->Error.puDaten > 256 ) {
							err = MittelFit3DBild( pBmp, &( pSnom->Error ), pSnom->w, pSnom->h, n, what );
						}
						// Lumineszenz mitteln?
						if( err  &&  (LONG)pSnom->Lumi.puDaten > 256 ) {
							err = MittelFit3DBild( pBmp, &( pSnom->Lumi ), pSnom->w, pSnom->h, n, what );
						}

						if( !err ) {
							FehlerRsc( E_NOT_FITTED );
							FreeBmp( pBmp, pBmp->iAktuell );
						}

						InvalidateRect( hwnd, NULL, FALSE );
						RecalcCache( pBmp, TRUE, TRUE );
					}
					SetCursor( hCurSave );
					StatusLine( NULL );
				}

					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, wParam != IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


// 24.3.00


/*************************************************************************************************/

/* Verwaltet Info-Dialog; dieser hat *KEINE* Button, sondern wird mit jedem Return und jeden Linksklick beendet! */
BOOL WINAPI InfoDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_INITDIALOG:
			if( lParam != 0 ) {
				SetTimer( hdlg, 15, lParam*1000l, NULL );
			}
			return ( TRUE );

		case WM_SYSKEYDOWN:
			if( wParam != VK_RETURN ) {
				return ( FALSE );
			}

		case WM_LBUTTONDOWN:
		case WM_TIMER:
			KillTimer( hdlg, 15 );
			EndDialog( hdlg, FALSE );
			return ( TRUE );
	}
	return ( FALSE );
}


/* Verwaltet Dialoge */
BOOL WINAPI StdDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_INITDIALOG:
			// Verwaltet auch Dialog mit Zahlen!
			if( GetDlgItem( hdlg, IDD_ZAHL ) != NULL ) {
				SetDlgItemInt( hdlg, IDD_ZAHL, (UINT)lParam, TRUE );
#ifdef _WIN32
				CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ALIGNRIGHT, 0, 0, 10, 10,
				                     hdlg, -1, hInst, GetDlgItem( hdlg, IDD_ZAHL ), 100, 1, lParam );
#endif
			}
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
				{
					BYTE str[256];
					if( GetDlgItemText( hdlg, HILFETEXT, str, 256 ) > 0 ) {
						WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)str );
					}
					break;
				}

				case IDOK:
					if( GetDlgItem( hdlg, IDD_ZAHL ) != NULL ) {
						EndDialog( hdlg, GetDlgItemInt( hdlg, IDD_ZAHL, NULL, TRUE ) );
					}
					else {
						EndDialog( hdlg, TRUE );
					}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/* Verwaltet Dialoge */
BOOL WINAPI StringDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPSTR buffer;

	switch( message ) {
		case WM_INITDIALOG:
			// Verwaltet auch Dialog mit Zahlen!
			buffer = (LPSTR)lParam;
			if( GetDlgItem( hdlg, IDD_ZAHL ) != NULL ) {
				SetDlgItemText( hdlg, IDD_ZAHL, buffer );
			}
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDOK:
					if( GetDlgItem( hdlg, IDD_ZAHL ) != NULL ) {
						EndDialog( hdlg, GetDlgItemText( hdlg, IDD_ZAHL, buffer, 256 ) );
					}
					else {
						EndDialog( hdlg, TRUE );
					}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


// 28.5.97


/***************************************************************************************/

/* Was tun bei Überlauf? */
BOOL WINAPI OverrunDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_INITDIALOG:
			// Verwaltet auch Dialog mit Zahlen!
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDOK:
				{
					WORD OverUnder = 0;

					if( IsDlgButtonChecked( hdlg, OVERRUN_OVERFLOW ) ) {
						OverUnder = 1;
					}
					if( IsDlgButtonChecked( hdlg, OVERRUN_UNDERFLOW ) ) {
						OverUnder |= 2;
					}
					EndDialog( hdlg, OverUnder );
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, 0 );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/
// Show Browsing Window

// Record with data for small file overview
typedef struct {
	BYTE Img128by128[128*128];
	BYTE strName[256];
	FILETIME FT;
	LONG iNr;
	float size;
	BOOL bLoaded;
} OVERVIEW_FILE;

// so it can be saved as preference
WORD SortOrder = OVER_NUMBER;

// Add files to Listbox ...
WORD AddFilesToListbox( HWND hListBox, CHAR *path )
{
	BYTE buffer[MAX_PATH], *p;
	HANDLE hs;
	WIN32_FIND_DATA	fd;
	OVERVIEW_FILE *pItem;

	SetCurrentDirectory( path );
	GetCurrentDirectory( MAX_PATH, buffer );
	lstrcat( buffer, "\\*.*" );

	SendMessage( hListBox, LB_RESETCONTENT, 0, 0 );
	SendMessage( hListBox, LB_SETCOLUMNWIDTH, 128, 0 );
	hs = FindFirstFile( buffer, &fd );
	if( hs != INVALID_HANDLE_VALUE ) {
		goto AddFile;
	}
	return ( 0 );
	while( FindNextFile( hs, &fd ) ) {
AddFile:
		if( fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY ) {
			continue;
		}
		// check extension
		p = fd.cFileName+lstrlen( fd.cFileName )-3;
		if( p[-1] != '.' ) {
			continue;
		}
		// check extension for: par, hdf, hdz, xyz 000...999
		if( !( *p == '0'  ||  atoi( p ) > 0 )  &&  lstrcmpi( p, "par" )  &&  lstrcmpi( p, "hdf" )  &&  lstrcmpi( p, "hdz" )  &&  lstrcmpi( p, "afm" )  &&  lstrcmpi( p, "xqd" ) ) {
			continue;
		}
		// add file here!
		pItem = pMalloc( sizeof( OVERVIEW_FILE ) );
		if( pItem != 0 ) {
			// read file and add date
			lstrcpy( (LPSTR)( pItem->strName ), fd.cFileName );
			for( p = fd.cFileName;  !isdigit( *p )  &&  *p > 0;  p++ ) {
				;
			}
			// Get the number
			pItem->iNr = atol( p );
			pItem->FT = fd.ftLastWriteTime;
			SendMessage( hListBox, LB_ADDSTRING, 0, (LPARAM)pItem );
		}
	}
	FindClose( hs );
	return ( 1 );
}


// Searches throught a directory and add all suitable files ...


/* Verwaltet Dialoge */
BOOL WINAPI HandleBrowseDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	extern HWND hwndFrame;
	static LPBITMAPINFO pDib;
	static HWND hwnd, hComboAverage, hComboFlatten;
	static HPALETTE	hPal;
	static WORD flatten = 1;
	static BOOL invert = FALSE;
	static WORD averaging = 1;

	switch( message ) {
		case WM_INITDIALOG:
		{
			LOGPALETTE HUGE *lpPalette;
			COLORREF Greys[3] = { 0, 0x00004080, 0xFFFFFFFF };
			LPSTR path;

			// Make pathname
			hwnd = (HWND) ( ( (LONG*)lParam )[0] );
			path = (LPSTR) ( ( (LONG*)lParam )[1] );
			SetDlgItemText( hdlg, OVER_DIR, path );

			// ComboBoxen initialisieren
			hComboAverage = GetDlgItem( hdlg, OVER_SMOOTH );
			SendMessage( hComboAverage, CB_RESETCONTENT, 0, 0 );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"None" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"Linewise 3" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"Linewise 5" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"Linewise 7" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"L-size 2" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"Area 3" );
			SendMessage( hComboAverage, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"Area 5" );
			SendMessage( hComboAverage, CB_SETCURSEL, averaging, 0 );

			hComboFlatten = GetDlgItem( hdlg, OVER_FLATTEN );
			SendMessage( hComboFlatten, CB_RESETCONTENT, 0, 0 );
			SendMessage( hComboFlatten, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"no flatten" );
			SendMessage( hComboFlatten, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"by line" );
			SendMessage( hComboFlatten, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)"by plane" );
			SendMessage( hComboFlatten, CB_SETCURSEL, flatten, 0 );

			CheckDlgButton( hdlg, OVER_INVERT, invert );

			// Create Bitmap for Display
			pDib = pMalloc( sizeof( BITMAPINFOHEADER )+sizeof( COLORREF )*256 );
			pDib->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
			pDib->bmiHeader.biWidth = 128;
			pDib->bmiHeader.biHeight = 128;
			pDib->bmiHeader.biPlanes = 1;
			pDib->bmiHeader.biBitCount = 8;
			pDib->bmiHeader.biCompression = BI_RGB;
			pDib->bmiHeader.biClrUsed = pDib->bmiHeader.biClrImportant = 256;
			SetDibPaletteColors( pDib, Greys, (LPBILD)1, 0, 255, 0, 255 );

			// Try to make Palette
			hPal = NULL;
			if( ( lpPalette = (LOGPALETTE HUGE*)pMalloc( sizeof( LOGPALETTE )+256*sizeof( PALETTEENTRY ) ) ) != NULL ) {
				LPWORD col;
				WORD i;

				GetDibPalette( lpPalette, pDib );
				hPal = CreatePalette( (LOGPALETTE HUGE*)lpPalette );
				col = (LPWORD)( (LPBYTE)pDib+pDib->bmiHeader.biSize );
				for( i = 0; i < 256;  i++ ) {
					*col++ = i;
				}
				MemFree( lpPalette );
			}

			// change to right sort order
			CheckRadioButton( hdlg, OVER_DATE, OVER_NAME, SortOrder );

			// Fill listbox with filenames
			AddFilesToListbox( GetDlgItem( hdlg, OVER_LIST ), path );
		}
			return ( TRUE ) ;

		case WM_COMPAREITEM:
			if( wParam == OVER_LIST ) {
				OVERVIEW_FILE *pItem1 = (OVERVIEW_FILE*) ( (LPCOMPAREITEMSTRUCT)lParam )->itemData1;
				OVERVIEW_FILE *pItem2 = (OVERVIEW_FILE*) ( (LPCOMPAREITEMSTRUCT)lParam )->itemData2;

				switch( SortOrder ) {
					case OVER_DATE:
						return ( CompareFileTime(  &( pItem1->FT ), &( pItem2->FT ) ) );

					case OVER_NUMBER:
						return ( ( pItem1->iNr > pItem2->iNr ) ? -1 : 1 );

					case OVER_NAME:
						return ( lstrcmpi( pItem1->strName, pItem2->strName ) ) ;
				}
				return ( 0 ) ;
			}

		case WM_DRAWITEM:
			if( wParam == OVER_LIST ) {
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
				OVERVIEW_FILE *pItem;

				/* If there are no list box items, skip this message. */
				if( lpdis->itemID == -1 ) {
					break;
				}
				pItem = (OVERVIEW_FILE*)lpdis->itemData;
				if( !pItem->bLoaded ) {
					LPBMPDATA pBmp = pMalloc( sizeof( BMPDATA ) );
					HWND nix;
					LONG i, max;

					// Zuerst mal "brute force"
					pBmp->Links = pBmp->Rechts = NONE;
					nix = hwndFrame;
					hwndFrame = NULL;
					if( !ReadAll( pItem->strName, pBmp ) ) {
						MemFree( pBmp );
						SendMessage( GetDlgItem( hdlg, OVER_LIST ), LB_DELETESTRING, lpdis->itemID, 0 );
						break;
					}
					if( pBmp->pSnom[0].Topo.puDaten != NULL ) {
						if( averaging > 0 ) {
							if( averaging <= 3 ) {
								BildMedianSpalten( pBmp->pSnom[0].Topo.puDaten, pBmp->pSnom[0].w, pBmp->pSnom[0].h, 1+( averaging*2 ) );
							}
							else {
								BildMedian( pBmp->pSnom[0].Topo.puDaten, pBmp->pSnom[0].w, pBmp->pSnom[0].h, 1+( ( averaging-4 )*2 ) );
							}
						}
						// resize
						BildResize( &( pBmp->pSnom[0].Topo ), pBmp->pSnom[0].w, pBmp->pSnom[0].h, 128, 128 );
						// Invert?
						if( invert ) {
							BildNegieren( &( pBmp->pSnom[0].Topo ), 128, 128 );
						}
						//  and do a second order fit ...
						if( flatten==2 ) {
							if( pBmp->pSnom[0].Topo.uMaxDaten > 8096u ) {
								BildCalcConst( &( pBmp->pSnom[0].Topo ), 128, 128, '/', pBmp->pSnom[0].Topo.uMaxDaten/8096.0, FALSE );
							}
							BildSteigungY( &( pBmp->pSnom[0].Topo ), 0, 128, 128 );
							MittelFit3DBild( pBmp, &( pBmp->pSnom[0].Topo ), 128, 128, 2, 3 );
						}
						if( flatten==1 ) {
							if( pBmp->pSnom[0].Topo.uMaxDaten > 8096u ) {
								BildCalcConst( &( pBmp->pSnom[0].Topo ), 128, 128, '/', pBmp->pSnom[0].Topo.uMaxDaten/8096.0, FALSE );
							}
							MittelBild( pBmp, &( pBmp->pSnom[0].Topo ), 128, 128, 2 );
						}
						// Make 8 Bit Bitmap
						max = pBmp->pSnom[0].Topo.uMaxDaten;
						for( i = 0;  i < 128*128;  i++ ) {
							pItem->Img128by128[i] = (BYTE)( ( 256l*(LONG)( pBmp->pSnom[0].Topo.puDaten[i] ) )/max );
						}
						pItem->bLoaded = TRUE;
					}
					if( (LONG)pBmp->pSnom[0].Topo.puDaten > 256 ) {
						MemFree( pBmp->pSnom[0].Topo.puDaten );
					}
					if( (LONG)pBmp->pSnom[0].Error.puDaten > 256 ) {
						MemFree( pBmp->pSnom[0].Error.puDaten );
					}
					if( (LONG)pBmp->pSnom[0].Lumi.puDaten > 256 ) {
						MemFree( pBmp->pSnom[0].Lumi.puDaten );
					}
					MemFree( pBmp );
					hwndFrame = nix;
				}

				/*
				 * Draw the bitmap and text for the list box item. Draw a
				 * rectangle around the bitmap if it is selected.
				 */
				switch( lpdis->itemAction ) {
					case ODA_DRAWENTIRE:
					case ODA_SELECT:
					{
						// We need extra clipping! (Otherwise DisplayDib erases all other windows down and right of this one!
						HANDLE hOldRegion = SelectObject( lpdis->hDC, CreateRectRgnIndirect( &lpdis->rcItem ) );
						BitBlt( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, lpdis->rcItem.right-lpdis->rcItem.left, lpdis->rcItem.bottom-lpdis->rcItem.top, NULL, 0, 0, WHITENESS );
						TextOut( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, pItem->strName, lstrlen( pItem->strName ) );
						// Since the drawing routine always expect this at (0,0), we better change the origin
						if( pItem->Img128by128 != NULL ) {
							SetWindowOrgEx( lpdis->hDC, -lpdis->rcItem.left, -( lpdis->rcItem.top+12 ), NULL );
							SelectPalette( lpdis->hDC, hPal, FALSE );
							RealizePalette( lpdis->hDC );
							SetDIBitsToDevice( lpdis->hDC, 0, 0, pDib->bmiHeader.biWidth, pDib->bmiHeader.biHeight,
							                   0, 0, 0, pDib->bmiHeader.biHeight,
							                   (LPSTR)pItem->Img128by128, (LPBITMAPINFO)pDib,
							                   ( hPal == NULL ) ? DIB_RGB_COLORS : DIB_PAL_COLORS );
							SetWindowOrgEx( lpdis->hDC, 0, 0, NULL );
						}
						else {
							SelectObject( lpdis->hDC, GetStockObject( GRAY_BRUSH ) );
							Rectangle( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, lpdis->rcItem.right-lpdis->rcItem.left, lpdis->rcItem.bottom-lpdis->rcItem.top );
						}

						if( lpdis->itemState&ODS_SELECTED ) {
							InvertRect( lpdis->hDC, &( lpdis->rcItem ) );
						}

						DrawFocusRect( lpdis->hDC, &( lpdis->rcItem ) );
						DeleteObject( SelectObject( lpdis->hDC, hOldRegion ) );
						break;
					}

					case ODA_FOCUS:
						break;
				}
			}
			return ( TRUE ) ;

		case WM_MEASUREITEM:
			if( wParam == OVER_LIST ) {
				( (LPMEASUREITEMSTRUCT)lParam )->itemWidth = 128;
				( (LPMEASUREITEMSTRUCT)lParam )->itemHeight = 128+12;
			}
			break;

		case WM_DELETEITEM:
			if( wParam == OVER_LIST ) {
				OVERVIEW_FILE *pItem = (OVERVIEW_FILE*)( ( (LPDELETEITEMSTRUCT)lParam )->itemData );
				if( pItem != NULL ) {
					MemFree( pItem );
				}
				return ( TRUE );
			}
			break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) ) {
				case OVER_CHANGE_DIR:
				{
					CHAR buffer[MAX_PATH];
					GetDlgItemText( hdlg, OVER_DIR, buffer, MAX_PATH );
					if( CMUGetFolderName( hdlg, STR_FOLDER, buffer ) ) {
						SetDlgItemText( hdlg, OVER_DIR, buffer );
						AddFilesToListbox( GetDlgItem( hdlg, OVER_LIST ), buffer );
					}
					break;
				}

				case OVER_INVERT:
				case OVER_SMOOTH:
				case OVER_FLATTEN:
				{
					BOOL new_invert = IsDlgButtonChecked( hdlg, OVER_INVERT );
					WORD new_average = SendMessage( hComboAverage, CB_GETCURSEL, 0, 0 );
					WORD new_flatten = SendMessage( hComboFlatten, CB_GETCURSEL, 0, 0 );
					if( new_flatten != flatten  ||  new_invert != invert  ||  new_average != averaging ) {
						CHAR path[MAX_PATH];
						invert = new_invert;
						flatten = new_flatten;
						averaging = new_average;
						GetDlgItemText( hdlg, OVER_DIR, path, 1024 );
						AddFilesToListbox( GetDlgItem( hdlg, OVER_LIST ), path );
					}
					break;
				}

				case IDOK:
				case IDCANCEL:
				{
					BYTE *p, buffer[MAX_PATH];      // pointer to filename
					HANDLE hListBox = GetDlgItem( hdlg, OVER_LIST );
					LONG i = SendMessage( hListBox, LB_GETCOUNT, 0, 0 );    // number of images

					GetDlgItemText( hdlg, OVER_DIR, buffer, MAX_PATH );
					strcat( buffer, "\\" );
					p = buffer+strlen( buffer );
					while( i-- > 0 ) {
						// is this selected?
						if( wParam == IDOK  &&  SendMessage( hListBox, LB_GETSEL, i, 0 ) ) {
							LPBMPDATA pBmp;
							strcpy( p, ( (OVERVIEW_FILE*)SendMessage( hListBox, LB_GETITEMDATA, i, 0 ) )->strName );
							pBmp = OpenCreateWindow( buffer );
							if( pBmp == NULL ) {
								continue;
							}
							// post processing?
							if( flatten==1 ) {
								LPSNOMDATA pSnom = pAllocNewSnom( pBmp, TOPO );
								if( pSnom == NULL ) {
									continue;
								}
								MittelFitBild( pBmp, &( pSnom->Topo ), pSnom->w, pSnom->h, 2 );
							}
							if( flatten==2 ) {
								LPSNOMDATA pSnom = pAllocNewSnom( pBmp, TOPO );
								if( pSnom == NULL ) {
									continue;
								}
								BildSteigungY( &( pSnom->Topo ), 1, pSnom->w, pSnom->h );
								pSnom = pAllocNewSnom( pBmp, TOPO );
								if( pSnom == NULL ) {
									continue;
								}
								MittelFit3DBild( pBmp, &( pSnom->Topo ), pSnom->w, pSnom->h, 2, 3 );
							}
							if( invert ) {
								LPSNOMDATA pSnom = pAllocNewSnom( pBmp, TOPO );
								if( pSnom == NULL ) {
									continue;
								}
								BildNegieren( &( pSnom->Topo ), pSnom->w, pSnom->h );
							}
							if( averaging > 0 ) {
								LPSNOMDATA pSnom = pAllocNewSnom( pBmp, TOPO );
								if( pSnom == NULL ) {
									continue;
								}
								if( averaging <= 3 ) {
									BildMedianSpalten( pSnom->Topo.puDaten, pSnom->w, pSnom->h, 1+( averaging*2 ) );
								}
								else {
									BildMedian( pSnom->Topo.puDaten, pSnom->w, pSnom->h, 1+( ( averaging-4 )*2 ) );
								}
							}
							RecalcCache( pBmp, TRUE, TRUE );
							// pos processing end
						}
						SendMessage( hListBox, LB_DELETESTRING, i, 0 );
					}
					MemFree( pDib );
					DeleteObject( hPal );
					EndDialog( hdlg, TRUE );
				}
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/*****************************************************************************************
****	Importiert Rohdaten in ein WORD-Feld ... ****
****	Die nötigen Parameter gibt es aus einer Struktur im Header-File (snom-dlg.h)
*****************************************************************************************/
BOOL WINAPI RohImportDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;

	switch( message ) {
		case WM_INITDIALOG:
			SetDlgItemInt( hdlg, ROH_WEITE, (UINT)512, FALSE );
			SetDlgItemInt( hdlg, ROH_OFFSET, 0, FALSE );
			CheckDlgButton( hdlg, ROH_INTEL, TRUE );        // Leider ist dieses verkorkste Speicherformat standard
			CheckDlgButton( hdlg, ROH_UNSIGNED, TRUE );     // Standard: Vorzeichenlos
			CheckRadioButton( hdlg, ROH_8BIT, ROH_16BIT, ROH_16BIT );
			pBmp = (LPBMPDATA)lParam;
			EnableWindow( GetDlgItem( hdlg, DLG_ERROR ), pBmp->pSnom[pBmp->iAktuell].Error.Typ == NONE );
			EnableWindow( GetDlgItem( hdlg, DLG_LUMI ), pBmp->pSnom[pBmp->iAktuell].Lumi.Typ == NONE );
			EnableWindow( GetDlgItem( hdlg, DLG_TOPO ), pBmp->pSnom[pBmp->iAktuell].Topo.Typ == NONE );
			return ( FALSE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDOK:
				{
					LPUWORD	pPtr;
					LONG lDateiOffset, len, ww, w, h, i;
					WORD Bit = 16;
					BOOLEAN	IntelCode = IsDlgButtonChecked( hdlg, ROH_INTEL );
					BOOLEAN	ret;
					LONG lOffset = 32768ul;
					HFILE hf;
					WORKMODE pDestTyp;

					// Gibt es die Daten noch nicht?
					if( IsDlgButtonChecked( hdlg, DLG_TOPO ) ) {
						pDestTyp = TOPO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_ERROR ) ) {
						pDestTyp = ERRO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_LUMI ) ) {
						pDestTyp = LUMI;
					}
					if( pDestTyp == NONE ) {
						FehlerRsc( W_NIX );
						EndDialog( hdlg, FALSE );
						return ( FALSE );
					}
					if( IsDlgButtonChecked( hdlg, ROH_UNSIGNED ) ) {
						lOffset = 0;
					}
					if( IsDlgButtonChecked( hdlg, ROH_8BIT ) ) {
						Bit = 8;
					}
					if( IsDlgButtonChecked( hdlg, ROH_10BIT ) ) {
						Bit = 10;
					}
					if( IsDlgButtonChecked( hdlg, ROH_12BIT ) ) {
						Bit = 12;
					}
					if( IsDlgButtonChecked( hdlg, ROH_16BIT ) ) {
						Bit = 16;
					}
					lDateiOffset = GetDlgItemInt( hdlg, ROH_OFFSET, NULL, TRUE );
					w = GetDlgItemInt( hdlg, ROH_WEITE, NULL, TRUE );
					h = GetDlgItemInt( hdlg, ROH_HOEHE, NULL, TRUE );
					if( w <= 0  ||  ( hf = _lopen( pBmp->szName, OF_READ | OF_SHARE_DENY_WRITE ) ) == HFILE_ERROR )	{
						FehlerRsc( E_IMPORT );
						EndDialog( hdlg, FALSE );
						return ( FALSE );
					}

					// Ok, dann kan es ja endlich losgehen
					len = _llseek( hf, 0l, 2 )-lDateiOffset;

					ww = ( w*Bit+7 )/8;
					if( h == 0 ) {
						h = len/ww;
					}
					if( h <= 0  ||  len <= 0 ) {
						_lclose( hf );
						FehlerRsc( E_IMPORT );
						EndDialog( hdlg, FALSE );
						return ( FALSE );
					}

					pPtr = (LPWORD)pMalloc( w*h*sizeof( WORD ) );
					if( pPtr == NULL ) {
						// Fatal Error => Abort!
						StatusLineRsc( E_MEMORY );
						_lclose( hf );
						EndDialog( hdlg, FALSE );
						return ( FALSE );
					}

					WarteMaus();
					i = _llseek( hf, lDateiOffset, 0 );
					i = _hread( hf, (LPVOID)pPtr, ( w*h*Bit )/8l );
					_lclose( hf );
					if( i < ( w*h*Bit )/8l ) {
						NormalMaus();
						MemFree( pPtr );
						FehlerRsc( E_IMPORT );
						EndDialog( hdlg, FALSE );
						return ( FALSE );
					}

					// Evt. Intel nach Motorola (bzw. vise versa
					i = ( i*8+7 )/Bit;
					if( !IntelCode ) {
						while( i-- > 0 )
							pPtr[i] = Big2Little( pPtr[i] );
					}

					// Dann noch einkopieren ...
					ret = LadeBlock( pBmp, pPtr, w, w, h, Bit, pDestTyp, lOffset, TRUE );
					MemFree( pPtr );
					NormalMaus();
					EndDialog( hdlg, ret );
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/
/****	Ab hier alles für die Farben ...              ****/

CHOOSECOLOR cc;
DWORD cColors[16];

BOOL no_update = FALSE;

// set this to zero for LUT update only
#define FULL_IMG_UPDATE (1)

/* Verwaltet Falschfarben-Dialog */
DWORD WINAPI FarbenDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static LPSNOMDATA pSnom;
	static LPBILD pBild;
	static WORKMODE	Modes[4];
	static HWND hwnd, h[3], hfStart, hfEnd, hTab;
	static HBRUSH hBrush[3];
	static WORKMODE	IsMode = NONE;
	static HFONT hSetFont;
	BYTE buffer[128];
	WORD i = 0;

	i = 0;
	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pBmp == NULL ) {
				EndDialog( hdlg, wParam != IDOK );
			}
			// Die Daten werden Online verändert!
			if( ( pSnom = pAllocNewSnom( pBmp, 0 ) ) == NULL ) {
				StatusLineRsc( E_MEMORY );
				EndDialog( hdlg, FALSE );
				return ( FALSE );
			}

			// Ok, valid pointers ...
			pSnom = pBmp->pSnom+pBmp->iAktuell;
			hBrush[0] = hBrush[1] = hBrush[2] = NULL;

			// Alles Initialisieren
			hfStart = GetDlgItem( hdlg, FARB_START );
			SetScrollRange( hfStart, SB_CTL, 0, 990, FALSE );
			hfEnd = GetDlgItem( hdlg, FARB_WEITE );
			SetScrollRange( hfEnd, SB_CTL, 0, 1000, FALSE );

			if( ( pSnom->Topo.Typ == TOPO  &&  ( pSnom->Topo.fEnde-pSnom->Topo.fStart ) < 100.0 )
			    ||  ( pSnom->Error.Typ == ERRO  &&  ( pSnom->Error.fEnde-pSnom->Error.fStart ) < 100.0 )
			    ||  ( pSnom->Lumi.Typ == LUMI  &&  ( pSnom->Lumi.fEnde-pSnom->Lumi.fStart ) < 100.0 ) ) {
				RecalcCache( pBmp, TRUE, FALSE );
			}

			// nötige Handels holen (ich liebe Windows, ich würde auch Gates 2x erschiessen, um sicher zu gehen ...
			h[0] = GetDlgItem( hdlg, FARB_SHOW1 );
			h[1] = GetDlgItem( hdlg, FARB_SHOW2 );
			h[2] = GetDlgItem( hdlg, FARB_SHOW3 );

			// Farbendialog initialisieren
			cc.lStructSize = sizeof( CHOOSECOLOR );
			cc.hwndOwner = hdlg;
			cc.hInstance = hInst;
			cc.Flags = CC_RGBINIT|CC_FULLOPEN;
			cc.lCustData = 0L;
			cc.lpfnHook = NULL;
			cc.lpTemplateName = NULL;

			// Und die richtige Bearbeitung einschalten ...
			pBild = NULL;
			no_update = FALSE;

			// Den Wechselknop vorbereiten
			{
				RECT rect;
				TC_ITEM	tci;
				NMHDR nm;

				GetClientRect( hdlg, &rect );
				hTab = CreateWindow( WC_TABCONTROL, "", WS_VISIBLE|WS_TABSTOP|WS_CHILD, 0, 0, rect.right, rect.bottom, hdlg, NULL, hInst, NULL );
				SendMessage( hTab, WM_SETFONT, (WPARAM)hSetFont, 0 );
				tci.mask = TCIF_TEXT;
				tci.iImage = -1;
				i = 0;
				if( pSnom->Topo.Typ != NONE ) {
					pBild = &( pSnom->Topo );
					tci.pszText = STR_TOPO;
					SendMessage( hTab, TCM_INSERTITEM, TOPO, (LPARAM)&tci );
					Modes[i++] = TOPO;
				}
				if( pSnom->Lumi.Typ != NONE ) {
					pBild = &( pSnom->Lumi );
					tci.pszText = STR_LUMI;
					SendMessage( hTab, TCM_INSERTITEM, LUMI, (LPARAM)&tci );
					Modes[i++] = LUMI;
				}
				if( pSnom->Error.Typ != NONE ) {
					pBild = &( pSnom->Error );
					tci.pszText = STR_ERROR;
					SendMessage( hTab, TCM_INSERTITEM, ERRO, (LPARAM)&tci );
					Modes[i++] = ERRO;
				}
				if( pBild == NULL ) {
					EndDialog( hdlg, wParam != IDOK );
				}
				SendMessage( hTab, TCM_SETCURSEL, 0, 0l );
				nm.hwndFrom = hTab;
				nm.code = TCN_SELCHANGE;
				hBrush[0] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[0] );
				hBrush[1] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[1] );
				hBrush[2] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[2] );
				pBild = 0;
				SendMessage( hdlg, WM_NOTIFY, 0, (LPARAM)&nm );
			}
			return ( TRUE ) ;

		case WM_SETFONT:
			hSetFont = (HANDLE)wParam;
			break;

		case WM_NOTIFY:
		{
			NMHDR *nm = (LPNMHDR)lParam;
			if( pBild != NULL ) {
				GetDlgItemText( hdlg, FARB_KONT_DIST, buffer, 15 );
				pBild->uKontur = (WORD)( atof( buffer )/pBild->fSkal+0.5 );
				GetDlgItemText( hdlg, FARB_MOD_DIST, buffer, 15 );
				pBild->uModulo = (WORD)( atof( buffer )/pBild->fSkal+0.5 );
				pBild->uKonturToleranz = GetDlgItemInt( hdlg, FARB_KONT_TOL, NULL, FALSE );
				pBild->bSpecialZUnit = IsDlgButtonChecked( hdlg, FARB_SPEZ_Z );
				pBild->bShowNoZ = !IsDlgButtonChecked( hdlg, FARB_Z_ACHSE );
				GetDlgItemText( hdlg, FARB_Z_UNIT, pBild->strZUnit, 8 );
				GetDlgItemText( hdlg, FARB_TITEL, pBild->strTitel, 32 );
				GetDlgItemText( hdlg, FARB_START_ZAHL, buffer, 128 );
				pBild->fStart = 100.000000001*atof( buffer )/(pBild->fSkal*pBild->uMaxDaten);
				GetDlgItemText( hdlg, FARB_WEITE_ZAHL, buffer, 128 );
				pBild->fEnde = 100.000000001*atof( buffer )/(pBild->fSkal*pBild->uMaxDaten) ;
			}
			if( nm->code != TCN_SELCHANGE ) {
				break;
			}
			IsMode = SendMessage( nm->hwndFrom, TCM_GETCURSEL, 0, 0l );
			IsMode = Modes[IsMode];
		}
		if( IsMode == TOPO ) {
			pBild = &( pSnom->Topo );
		}
		if( IsMode == ERRO ) {
			pBild = &( pSnom->Error );
		}
		if( IsMode == LUMI ) {
			pBild = &( pSnom->Lumi );
		}
		if( pBild == NULL ) {
			break;
		}
		SetDlgItemText( hdlg, FARB_TITEL, pBild->strTitel );
		CheckDlgButton( hdlg, FARB_LUT, !( pBild->bNoLUT ) );
		if( pBild->iNumColors == 0 ) {
			EnableWindow( GetDlgItem( hdlg, FARB_LUT ), FALSE );
		}
		CheckDlgButton( hdlg, FARB_3D, pBild->bPseudo3D );
		CheckDlgButton( hdlg, FARB_KONT, pBild->bKonturen );
		CheckDlgButton( hdlg, FARB_SPEZ_Z, pBild->bSpecialZUnit );
		SetDlgItemText( hdlg, FARB_Z_UNIT, pBild->strZUnit );
		CheckDlgButton( hdlg, FARB_Z_ACHSE, !pBild->bShowNoZ );
		SetDlgItemText(	hdlg, FARB_MOD_DIST, gcvt( pBild->uModulo*pBild->fSkal, 5, buffer ) );
		SetDlgItemText(	hdlg, FARB_KONT_DIST, gcvt( pBild->uKontur*pBild->fSkal, 5, buffer ) );
		SetDlgItemInt(	hdlg, FARB_KONT_TOL, pBild->uKonturToleranz, FALSE );
		SetScrollPos( hfStart, SB_CTL, (WORD)( pBild->fStart*10.0+0.5 ), TRUE );
		SetScrollPos( hfEnd, SB_CTL, (WORD)( pBild->fEnde*10.0+0.5 ), TRUE );
		sprintf( buffer, "%.3lg %s", pBild->uMaxDaten*pBild->fStart*pBild->fSkal/100.0, (pBild->bSpecialZUnit ? pBild->strZUnit : "nm") );
		SetDlgItemText(	hdlg, FARB_START_ZAHL, buffer );
		sprintf( buffer, "%.3lg %s", pBild->uMaxDaten*pBild->fEnde*pBild->fSkal/100.0, (pBild->bSpecialZUnit ? pBild->strZUnit : "nm") );
		SetDlgItemText(	hdlg, FARB_WEITE_ZAHL, buffer );
		if( hBrush[0] != NULL )	{
			DeleteObject( hBrush[0] );
			DeleteObject( hBrush[1] );
			DeleteObject( hBrush[2] );
		}
		hBrush[0] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[0] );
		hBrush[1] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[1] );
		hBrush[2] = CreateSolidBrush( ( (COLORREF HUGE*)pBild->Farben )[2] );
		InvalidateRect( hdlg, NULL, FALSE );
		break;

#ifdef	BIT32
		case WM_CTLCOLORSTATIC:
		{
			for( i = 0; i < 3; i++ ) {
				if( (HWND)lParam == h[i] ) {
					SelectObject( (HDC)wParam, hBrush[i] );
					return ( (DWORD)hBrush[i] );
				}
			}
			break;
		}

#else
		case WM_CTLCOLOR:
			for( i = 0; i < 3; i++ ) {
				if( (HWND)( lParam ) == h[i] ) {
					return ( hBrush[i] );
				}
			}
			break;
#endif

		// Draw Colortable as Spectrum ...
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT) lParam;

			if( pDIS->itemAction == ODA_DRAWENTIRE  ||  pDIS->itemAction == ODA_SELECT ) {
				LPBITMAPINFO pDib;
				WORD i;
				BYTE *p;

				// Clear Background
				BitBlt( pDIS->hDC, pDIS->rcItem.left, pDIS->rcItem.top, pDIS->rcItem.right-pDIS->rcItem.left, pDIS->rcItem.bottom-pDIS->rcItem.top, NULL, 0, 0, WHITENESS );

				// Create DIB
				pDib = (LPBITMAPINFO)pMalloc( sizeof( BITMAPINFOHEADER )+sizeof( RGBQUAD )*256+64 );
				pDib->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
				pDib->bmiHeader.biWidth = 64;
				pDib->bmiHeader.biHeight = 1;
				pDib->bmiHeader.biPlanes = 1;
				pDib->bmiHeader.biBitCount = 8;
				pDib->bmiHeader.biCompression = BI_RGB;
				pDib->bmiHeader.biXPelsPerMeter = 0;
				pDib->bmiHeader.biYPelsPerMeter = 0;
				pDib->bmiHeader.biClrUsed = 64;
				pDib->bmiHeader.biClrImportant = 0;
				if( pDib == NULL ) {
					break;
				}
				p = (BYTE *)pDib;
				p += sizeof( BITMAPINFOHEADER );
				SetDibPaletteColors( pDib, pBild->Farben, pBild, 0, 64, 0, 64 );
				// Set Bitmap Data
				p = (BYTE *)pDib;
				p += sizeof( BITMAPINFOHEADER )+sizeof( RGBQUAD )*256l;
				for( i = 0;  i < 64;  i++ ) {
					p[i] = i;
				}

				SetStretchBltMode( pDIS->hDC, COLORONCOLOR );
				StretchDIBits(	pDIS->hDC,
				                pDIS->rcItem.left, pDIS->rcItem.top, pDIS->rcItem.right-pDIS->rcItem.left, pDIS->rcItem.bottom-pDIS->rcItem.top,
				                0, 0, 63, 1,
				                (LPSTR) p, (LPBITMAPINFO) pDib,
				                DIB_RGB_COLORS, SRCCOPY ) ;
			}
			break;
		}


		case WM_HSCROLL:
		{
			int iOffset = GetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL );
			switch( LOWORD( wParam ) ) {
				case SB_LINEDOWN:
					iOffset++;
					break;

				case SB_LINEUP:
					iOffset--;
					break;

				case SB_PAGEDOWN:
					iOffset += 16;
					break;

				case SB_PAGEUP:
					iOffset -= 16;
					break;

				case SB_BOTTOM:
					iOffset = 256;
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
			if(  LOWORD( wParam ) != SB_ENDSCROLL  ) {
				// not end of scrolling, further action needed
				SetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL, (WORD)iOffset, TRUE );
				pBild->fStart = GetScrollPos( hfStart, SB_CTL )/10.0;
				sprintf( buffer, "%.3lg %s", pBild->uMaxDaten*pBild->fStart*pBild->fSkal/100.0, (pBild->bSpecialZUnit ? pBild->strZUnit : "nm") );
				SetDlgItemText(	hdlg, FARB_START_ZAHL, buffer );
				pBild->fEnde = GetScrollPos( hfEnd, SB_CTL )/10.0;
				sprintf( buffer, "%.3lg %s", pBild->uMaxDaten*pBild->fEnde*pBild->fSkal/100.0, (pBild->bSpecialZUnit ? pBild->strZUnit : "nm") );
				SetDlgItemText(	hdlg, FARB_WEITE_ZAHL, buffer );
				// Bitmap updaten!
				RecalcCache( pBmp, FULL_IMG_UPDATE, FULL_IMG_UPDATE );
				InvalidateRect( hwnd, NULL, FALSE );
				InvalidateRect( GetDlgItem( hdlg, FARB_SHOW_LUT ), NULL, FALSE );
			}
			break;
		}

		case WM_COMMAND:
			switch( LOWORD( wParam ) ) {

				case FARB_START_ZAHL:
				case FARB_WEITE_ZAHL:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						BOOL update = FALSE;
						double newstart = pBild->fStart, newend = pBild->fEnde;

						if(  LOWORD( wParam ) == FARB_START_ZAHL  ) {
							GetDlgItemText( hdlg, FARB_START_ZAHL, buffer, 128 );
							newstart = 100.0*atof( buffer )/(pBild->fSkal*pBild->uMaxDaten);
						}
						if(  LOWORD( wParam ) == FARB_WEITE_ZAHL  ) {
							GetDlgItemText( hdlg, FARB_WEITE_ZAHL, buffer, 128 );
							newend = 100.0*atof( buffer )/(pBild->fSkal*pBild->uMaxDaten) ;
						}

						if(  fabs(newstart-pBild->fStart)>.5  ) {
							pBild->fStart = newstart;
							SetScrollPos( hfStart, SB_CTL, (WORD)( newstart*10.0+0.5 ), TRUE );
							update = TRUE;
						}
						if(  fabs(newend-pBild->fEnde)>.5  ) {
							pBild->fEnde = newend;
							SetScrollPos( hfEnd, SB_CTL, (WORD)( newend*10.0+0.5 ), TRUE );
							update = TRUE;
						}
						if(  update  ) {
							// Bitmap updaten!
							RecalcCache( pBmp, FULL_IMG_UPDATE, FULL_IMG_UPDATE );
							InvalidateRect( hwnd, NULL, FALSE );
							InvalidateRect( GetDlgItem( hdlg, FARB_SHOW_LUT ), NULL, FALSE );
						}
					}
					break;

				case FARB_LOAD_LUT:
				{       //Loading new LUT
					TCHAR datei[MAX_PATH];
					if( CMUFileOpen( hwnd, STR_OPEN_FILE, datei, STR_FILE_LUT ) ) {
						HFILE hFile;
						int i;
						UCHAR pPtr[256*3], *pC = pPtr;

						hFile = _lopen( datei, OF_READ|OF_SHARE_DENY_WRITE );
						if( hFile == HFILE_ERROR ) {
							break;
						}
						_hread( hFile, pPtr, 3*256 );
						_lclose( hFile );
						for( i = 0;  i < 256;  i++, pC += 3 ) {
							pBild->LUT[i] = RGB( pC[0], pC[1], pC[2] );
						}
						pBild->iNumColors = 256;
						EnableWindow( GetDlgItem( hdlg, FARB_LUT ), TRUE );
						CheckDlgButton( hdlg, FARB_LUT, !pBild->bNoLUT );
					}
				}

				case FARB_LUT:
					pBild->bNoLUT = !IsDlgButtonChecked( hdlg, FARB_LUT );
					// Bitmap updaten!
					RecalcCache( pBmp, FULL_IMG_UPDATE, FULL_IMG_UPDATE );
					InvalidateRect( GetDlgItem( hdlg, FARB_SHOW_LUT ), NULL, FALSE );
					InvalidateRect( hwnd, NULL, FALSE );
					break;

				// Farbe einstellen
				case FARB_3:
					i++;

				case FARB_2:
					i++;

				case FARB_1:
					cc.rgbResult = ( (COLORREF HUGE*)pBild->Farben )[i];
					cc.lpCustColors = (LPDWORD)cColors;
					if( !ChooseColor( &cc ) ) {
						break;
					}
					DeleteObject( hBrush[i] );
					( (COLORREF HUGE*)pBild->Farben )[i] = cc.rgbResult;
					hBrush[i] = CreateSolidBrush( cc.rgbResult );
					InvalidateRect( h[i], NULL, FALSE );
					// Bitmap updaten!
					RecalcCache( pBmp, FULL_IMG_UPDATE, FULL_IMG_UPDATE );
					InvalidateRect( hwnd, NULL, FALSE );
					InvalidateRect( GetDlgItem( hdlg, FARB_SHOW_LUT ), NULL, FALSE );
					break;

				case FARB_MOD:
				case FARB_KONT:
				case FARB_3D:
				case FARB_PREVIEW:
					pBild->bPseudo3D = IsDlgButtonChecked( hdlg, FARB_3D );
					pBild->bKonturen = IsDlgButtonChecked( hdlg, FARB_KONT );
					pBild->bModuloKonturen = IsDlgButtonChecked( hdlg, FARB_MOD );
					pBild->bSpecialZUnit = IsDlgButtonChecked( hdlg, FARB_SPEZ_Z );
					pBild->bShowNoZ = !IsDlgButtonChecked( hdlg, FARB_Z_ACHSE );
					GetDlgItemText( hdlg, FARB_Z_UNIT, pBild->strZUnit, 8 );
					// Bitmap komplett updaten!
					GetDlgItemText( hdlg, FARB_KONT_DIST, buffer, 15 );
					pBild->uKontur = (WORD)( atof( buffer )/pBild->fSkal+0.5 );
					pBild->uKonturToleranz = (WORD)( (LONG)GetDlgItemInt( hdlg, FARB_KONT_TOL, NULL, FALSE )*pBild->uKontur/100l );
					GetDlgItemText( hdlg, FARB_MOD_DIST, buffer, 15 );
					pBild->uModulo = (WORD)( atof( buffer )/pBild->fSkal+0.5 );
					GetDlgItemText( hdlg, FARB_TITEL, pBild->strTitel, 32 );
					if( pBild->uKontur == 0  &&  pBild->bKonturen )	{
						pBild->bKonturen = FALSE;
						CheckDlgButton( hdlg, FARB_KONT, FALSE );
					}
					if( pBild->uModulo == 0  &&  pBild->bModuloKonturen ) {
						pBild->bModuloKonturen = FALSE;
						CheckDlgButton( hdlg, FARB_MOD, FALSE );
					}
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					break;

				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_FARBEN );
					break;

				case IDCANCEL:
					// Neue Bitmap wieder löschen
					pBmp->iAktuell--;
					pBmp->iMax--;
					RecalcCache( pBmp, TRUE, TRUE );
					for( i = 0; i < 3; i++ ) {
						DeleteObject( hBrush[i] );
					}
					InvalidateRect( hwnd, NULL, FALSE );
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;

				case IDOK:
				{
					NMHDR nm;
					// Update der Parameter erzwingen ...
					nm.hwndFrom = hTab;
					nm.code = TCN_SELCHANGING;
					SendMessage( hdlg, WM_NOTIFY, 0, (LPARAM)&nm );
					EndDialog( hdlg, TRUE );
					// Alte Palette wieder herstellen
					for( i = 0; i < 3; i++ ) {
						DeleteObject( hBrush[i] );
					}
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					return ( TRUE ) ;
				}
			}
	}
	return ( FALSE );
}


/***************************************************************************************/

/* 3D-Ansicht-Dialog */
int WINAPI Ansicht3DDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hAnsicht, h[4], hwnd;
	static float fZAngle, fXYAngle, fZSkal;
	static HGDIOBJ brush[4];
	static COLORREF color[4];
	double fXW, fYW, fZW;
	int i, j;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );

			hAnsicht = GetDlgItem( hdlg, ANSICHT_PREVIEW );
#ifdef _WIN32
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, ANSICHT_XY_EDIT, hInst, GetDlgItem( hdlg, ANSICHT_XY_EDIT ), 90, 0, 90.0*f3DXYWinkel );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, ANSICHT_Z_EDIT, hInst, GetDlgItem( hdlg, ANSICHT_Z_EDIT ), 90, 0, 90.0*f3DZWinkel );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, ANSICHT_HOEHE_EDIT, hInst, GetDlgItem( hdlg, ANSICHT_HOEHE_EDIT ), 500, 0, 100.0*f3DZSkal );
#else
			fZAngle = f3DZWinkel;
			fXYAngle = f3DXYWinkel;
			fZSkal = f3DZSkal;
#endif
			if( w3DZoom > 1 ) {
				CheckDlgButton( hdlg, ANSICHT_DRAFT, 1 );
			}
			CheckDlgButton( hdlg, ANSICHT_PLOTPOS, PlotsUnten );

			// Die Boxen zur Ansicht füllen
			SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_RESETCONTENT, 0, 0 );
			SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_ADDSTRING, 0, (LPARAM)STR_PROFIL );
			SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_ADDSTRING, 0, (LPARAM)STR_AUTOKORR );
			SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_ADDSTRING, 0, (LPARAM)STR_PSD );
			for( j = 0, i = 1<<3;  i <= P_INT_HIST;  i *= 2, j++ ) {
				if( wProfilShowFlags&i ) {
					SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_SETCURSEL, j, 0 );
					break;
				}
			}

			SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_RESETCONTENT, 0, 0 );
			SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_RESETCONTENT, 0, 0 );
			SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_ADDSTRING, 0, (LPARAM)STR_NIX );
			SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_ADDSTRING, 0, (LPARAM)STR_NIX );
			SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)STR_NIX );
			SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)STR_NIX );
			if( pBmp != NULL ) {
				SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_TOPO );
				SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_TOPO );
				SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_ERROR );
				SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_ERROR );
				SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_LUMI );
				SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)STR_LUMI );
				switch( pBmp->Links ) {
					case TOPO:
						SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_TOPO );
						break;

					case ERRO:
						SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_ERROR );
						break;

					case LUMI:
						SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_LUMI );
						break;
				}
				switch( pBmp->Rechts ) {
					case TOPO:
						SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_TOPO );
						break;

					case ERRO:
						SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_ERROR );
						break;

					case LUMI:
						SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPSTR)STR_LUMI );
						break;
				}
			}

			// nötige Handels holen
			h[0] = GetDlgItem( hdlg, ANSICHT_SHOW1 );
			h[1] = GetDlgItem( hdlg, ANSICHT_SHOW2 );
			h[2] = GetDlgItem( hdlg, ANSICHT_SHOW3 );
			h[3] = GetDlgItem( hdlg, ANSICHT_SHOW4 );
			// Farbeinstellung vorbereiten
			color[0] = cVorn;
			brush[0] = CreateSolidBrush( color[0] );
			color[1] = cMarkierungLinks;
			brush[1] = CreateSolidBrush( color[1] );
			color[2] = cMarkierungRechts;
			brush[2] = CreateSolidBrush( color[2] );
			color[3] = cHinten;
			brush[3] = CreateSolidBrush( color[3] );
			cc.lStructSize = sizeof( CHOOSECOLOR );
			cc.hwndOwner = hdlg;
			cc.hInstance = hInst;
			cc.Flags = CC_RGBINIT|CC_FULLOPEN;
			cc.lCustData = 0L;
			cc.lpfnHook = NULL;
			cc.lpTemplateName = NULL;
			return ( TRUE ) ;

#ifdef	BIT32
		case WM_CTLCOLORSTATIC:
		{
			for( i = 0; i < 4; i++ ) {
				if( (HWND)lParam == h[i] ) {
					return ( (int)brush[i] );
				}
			}
			break;
		}

#else
		// Makierung für die Buttons ...
		case WM_CTLCOLOR:
			for( i = 0; i < 4; i++ ) {
				if( (HWND)lParam == h[i] ) {
					return ( brush[i] );
				}
			}
			break;
#endif

		case WM_PAINT:
Redraw3D:
			{
				HANDLE hBrush, hOld;
				HDC hDC;
				RECT rect;
				HRGN hRgn;
				WORD dx, dy; // Offsets zum Zentrieren

				InvalidateRect( hAnsicht, NULL, FALSE );
				UpdateWindow( hAnsicht );
				hDC = GetDC( hAnsicht );
				GetClientRect( hAnsicht, &rect );
				hRgn = CreateRectRgnIndirect( &rect );
				hOld = SelectObject( hDC, brush[3] );
				SelectObject( hDC, hRgn );
				PaintRgn( hDC, hRgn );
				hOld = SelectObject( hDC, hOld );
				hBrush = CreatePen( PS_INSIDEFRAME, 2, color[0] );
				hOld = SelectObject( hDC, hBrush );

				rect.bottom -= 1;
				fYW = fXYAngle;
				fXW = ( 1.0-fYW );
				fZW = fZSkal*( 1.0-fZAngle );

				dx = ( rect.right-( 40.0*fXW+60.0*fYW ) )/2;
				dy = ( rect.bottom + ( 60.0*fXW + 40.0*fYW )*fZAngle+12.0*fZW )/2;

				MoveToEx( hDC, dx, dy-40.0*fZAngle*fYW, NULL );
				LineTo( hDC, dx+40.0*fXW, dy );
				LineTo( hDC, dx+( 40.0*fXW+60.0*fYW ), dy-60.0*fZAngle*fXW );
				LineTo( hDC, dx+( 40.0*fXW+60.0*fYW ), dy-60.0*fZAngle*fXW-12.0*fZW );
				LineTo( hDC, dx+40.0*fXW, dy-12.0*fZW );
				LineTo( hDC, dx, dy-40.0*fZAngle*fYW-12.0*fZW );
				LineTo( hDC, dx, dy-40.0*fZAngle*fYW );
				MoveToEx( hDC, dx+40.0*fXW, dy, NULL );
				LineTo( hDC, dx+40.0*fXW, dy-12.0*fZW );
				MoveToEx( hDC, dx, dy-40.0*fYW*fZAngle-12.0*fZW, NULL );
				LineTo( hDC, dx+60.0*fYW, dy-( 60.0*fXW + 40.0*fYW )*fZAngle-12.0*fZW );
				LineTo( hDC, dx+( 60.0*fYW+40.0*fXW ), dy-60.0*fXW*fZAngle-12.0*fZW );

				SelectObject( hDC, hOld );
				DeleteObject( hBrush );

				DeleteRgn( hRgn );
				ReleaseDC( hAnsicht, hDC );
			}
			break;

		case WM_COMMAND:
			i = 0;
			switch( LOWORD( wParam ) ) {
				case ANSICHT_XY_EDIT:
				case ANSICHT_Z_EDIT:
				case ANSICHT_HOEHE_EDIT:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						fXYAngle = GetDlgItemInt( hdlg, ANSICHT_XY_EDIT, NULL, FALSE )/90.0;
						fZAngle = GetDlgItemInt( hdlg, ANSICHT_Z_EDIT, NULL, FALSE )/90.0;
						fZSkal = GetDlgItemInt( hdlg, ANSICHT_HOEHE_EDIT, NULL, FALSE )/100.0;
						goto Redraw3D;
					}
					break;

				case ANSICHT_HINTEN:
					i++;

				case ANSICHT_MARK_RECHTS:
					i++;

				case ANSICHT_MARK_LINKS:
					i++;

				case ANSICHT_VORNE:
					cc.rgbResult = color[i];
					cc.lpCustColors = (LPDWORD)cColors;
					if( !ChooseColor( &cc ) ) {
						break;
					}
					color[i] = cc.rgbResult;
					DeleteObject( brush[i] );
					brush[i] = CreateSolidBrush( color[i] );
					InvalidateRect( h[i], NULL, FALSE );
					InvalidateRect( hAnsicht, NULL, FALSE );
					break;

				case ANSICHT_ZEICHENSATZ:
					ChooseFont( &CFont );
					break;

				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_VIEW );
					break;

				case IDOK:
					f3DZWinkel = fZAngle;
					f3DXYWinkel = fXYAngle;
					f3DZSkal = fZSkal;
					w3DZoom = 1;
					if( IsDlgButtonChecked( hdlg, ANSICHT_DRAFT ) ) {
						w3DZoom = 2;
					}
					cVorn = color[0];
					cMarkierungLinks = color[1];
					cMarkierungRechts = color[2];
					cHinten = color[3];
					DeleteObject( brush[0] );
					DeleteObject( brush[1] );
					DeleteObject( brush[2] );
					DeleteObject( brush[3] );
					PlotsUnten = IsDlgButtonChecked( hdlg, ANSICHT_PLOTPOS );
					wProfilShowFlags = P_DIST|( 1<<( SendDlgItemMessage( hdlg, ANSICHT_DATEN, CB_GETCURSEL, 0, 0l )+3 ) );
					if( pBmp ) {
						pBmp->Links = 1<<( SendDlgItemMessage( hdlg, ANSICHT_LINKS, CB_GETCURSEL, 0, 0l )-1 );
						pBmp->Rechts = 1<<( SendDlgItemMessage( hdlg, ANSICHT_RECHTS, CB_GETCURSEL, 0, 0l )-1 );
						pBmp->bPlotUnten = PlotsUnten;
					}
					RedrawAll( CLEAR3D|UPDATE|SLIDER );

				case IDCANCEL:
					EndDialog( hdlg, wParam != IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/


// Sucht Namen aller Bitmaps ... (z.B. für die Mathematik)
LPBMPDATA MakeHwndName( HWND hwnd )
{
	LPBMPDATA pBmp;

	if( !IsChild( hwndClient, hwnd )  ||  !IsWindowVisible( hwnd )  ||  hwnd == NULL
	    ||  ( pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 ) ) == NULL
	    ||  IsBadHugeWritePtr( pBmp, 20 )  ||  *(LPLONG)pBmp == 0l ) {
		return ( NULL );
	}
	return ( pBmp );
}


//**** Callback für das Neuzeichnen der Fenster
BOOL CALLBACK EnumSetComboNames( HWND hwnd, LPARAM lparm )
{
	LPBMPDATA pBmp;
	WORD i;
	BYTE str[256];

	if( (HWND)HIWORD( lparm ) != hwnd ) {
		pBmp = MakeHwndName( hwnd );
		if( pBmp != NULL ) {
#if 0
			if( pBmp->pSnom[pBmp->iAktuell].Topo.Typ == TOPO  &&  modus&TOPO ) {
				lstrcpy( str, (LPCSTR)"t " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)( (long)hwnd|( (long)TOPO<<16l ) ) ) == CB_ERR ) {
					return ( FALSE );
				}
			}
			if( pBmp->pSnom[pBmp->iAktuell].Error.Typ == ERRO  &&  modus&ERRO ) {
				lstrcpy( str, (LPCSTR)"e " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)( (long)hwnd|( (long)ERRO<<16l ) ) ) == CB_ERR ) {
					return ( FALSE );
				}
			}
			if( pBmp->pSnom[pBmp->iAktuell].Lumi.Typ == LUMI  &&  modus&LUMI ) {
				lstrcpy( str, (LPCSTR)"l " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)( (long)hwnd|( (long)LUMI<<16l ) ) ) == CB_ERR ) {
					return ( FALSE );
				}
			}
#else
			if( pBmp->pSnom[pBmp->iAktuell].Topo.Typ == TOPO  &&  modus&TOPO ) {
				lstrcpy( str, (LPCSTR)"t " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)hwnd ) == CB_ERR ) {
					return ( FALSE );
				}
			}
			if( pBmp->pSnom[pBmp->iAktuell].Error.Typ == ERRO  &&  modus&ERRO ) {
				lstrcpy( str, (LPCSTR)"e " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)hwnd ) == CB_ERR ) {
					return ( FALSE );
				}
			}
			if( pBmp->pSnom[pBmp->iAktuell].Lumi.Typ == LUMI  &&  modus&LUMI ) {
				lstrcpy( str, (LPCSTR)"l " );
				lstrcat( str, (LPCSTR)pBmp->szName+pBmp->wKurzname );
				i = (WORD)SendMessage( (HWND)lparm, CB_ADDSTRING, 0, (LPARAM)(LPSTR)str );
				if( SendMessage( (HWND)lparm, CB_SETITEMDATA, i, (LPARAM)hwnd ) == CB_ERR ) {
					return ( FALSE );
				}
			}
#endif
		}
	}
	return ( TRUE );
}


/***********************************************************************************************/
/* Verwaltet Mathe-Dialog */
BOOL WINAPI MatheDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	BYTE str[256];
	static HWND hwnd, hComboSrc;
	static WORD op, overflow, fest;
	static double wert;
	WORD i;

	switch( message ) {
		case WM_INITDIALOG:
		{
			LPBMPDATA pSrc;

			hwnd = (HWND)lParam;
			pSrc = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pSrc == NULL ) {
				return ( FALSE );   // Das war kein gültiger Pointer ...
			}
			// ComboBoxen initialisieren
			hComboSrc = GetDlgItem( hdlg, MATHE_QUELLE );
			SendMessage( hComboSrc, CB_RESETCONTENT, 0, 0 );
			EnumChildWindows( hwndClient, EnumSetComboNames, (LPARAM)MAKELONG( hComboSrc, NULL ) );

			// und Button polieren: Zuerst Quelle und Ziel
			EnableWindow( GetDlgItem( hdlg, DLG_ERROR ), pSrc->pSnom[pSrc->iAktuell].Error.Typ == ERRO );
			EnableWindow( GetDlgItem( hdlg, DLG_LUMI ), pSrc->pSnom[pSrc->iAktuell].Lumi.Typ == LUMI );
			EnableWindow( GetDlgItem( hdlg, DLG_TOPO ), pSrc->pSnom[pSrc->iAktuell].Topo.Typ == TOPO );

			if( op > 0 ) {
				CheckRadioButton( hdlg, 30, 130, op );
			}
			if( overflow > 0 ) {
				CheckDlgButton( hdlg, MATHE_OVERFLOW, 1 );
			}
			if( fest ) {
				CheckDlgButton( hdlg, MATHE_FEST, 1 );
				gcvt( wert, 5, str );
				SetDlgItemText( hdlg, MATHE_WERT, str );
			}
		}
			return ( TRUE ) ;

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_MATH );
					break;

				case IDOK:
				{
					HWND DestHwnd = hwnd;
					LONG SrcHwnd;
					LPBMPDATA pSrc, pDest;
					LPSNOMDATA pSnom;
					WORKMODE pSrcTyp = NONE, pDestTyp = NONE; //

					op = 0;
					i = (WORD)SendMessage( hComboSrc, CB_GETCURSEL, 0, 0L );
					SrcHwnd = (LONG)SendMessage( hComboSrc, CB_GETITEMDATA, i, 0L );
					pDest = (LPBMPDATA)GetWindowLong( hwnd, 0 );
					if( IsDlgButtonChecked( hdlg, DLG_TOPO ) ) {
						pDestTyp = TOPO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_ERROR ) ) {
						pDestTyp = ERRO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_LUMI ) ) {
						pDestTyp = LUMI;
					}
					pSrc = (LPBMPDATA)GetWindowLong( (HWND)SrcHwnd, 0 );
					pSrcTyp = ( SrcHwnd>>16 );
					fest = IsDlgButtonChecked( hdlg, MATHE_FEST );
					GetDlgItemText( hdlg, MATHE_WERT, str, 256 );
					wert = atof( str );

					// Ok, nun Operation feststellen
					if( IsDlgButtonChecked( hdlg, MATHE_PLUS ) ) {
						op = MATHE_PLUS;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_MINUS ) ) {
						op = MATHE_MINUS;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_MAL ) ) {
						op = MATHE_MAL;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_GETEILT ) ) {
						op = MATHE_GETEILT;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_MODULO ) ) {
						op = MATHE_MODULO;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_OR ) ) {
						op = MATHE_OR;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_AND ) ) {
						op = MATHE_AND;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_XOR ) ) {
						op = MATHE_XOR;
					}
					if( IsDlgButtonChecked( hdlg, MATHE_LOG ) ) {
						op = MATHE_LOG;
					}
					overflow = IsDlgButtonChecked( hdlg, MATHE_OVERFLOW );

					// Alle möglichen Fehler abfangen
					if( op == 0 ) {
						// Keinen gültigen Operator ausgewählt
						goto MatheFehler;
					}
					if( fest  &&  wert == 0.0  &&  op == MATHE_GETEILT ) {
						// Division durch Null
						goto MatheFehler;
					}
					if( !fest  &&  ( pSrc == NULL || pSrcTyp == 0 ) ) {
						// Keine gültigen 2. Operanden ausgewählt!
						goto MatheFehler;
					}
					if( pDest == NULL  ||  pDestTyp == NONE  ||  ( pSnom = pAllocNewSnom( pDest, pDestTyp ) ) == NULL ) {
						// Keine gültigen Zieldaten ausgewählt!
						goto MatheFehler;
					}
					if( fest ) {
						BildCalcConst( GetBildPointer( pDest, pDestTyp ), pSnom->w, pSnom->h, op, wert, overflow );
					}
					else {
						BildCalcBild( pDest, pDestTyp, op, pSrc, pSrcTyp, overflow );
					}

					RecalcCache( pDest, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, TRUE );
// Hier bei schweren Fehlern bei Dialog ausfüllen
MatheFehler:
					InvalidateRect( DestHwnd, NULL, TRUE );
					EndDialog( hdlg, TRUE );
					return ( TRUE ) ;
				}
				// Sonst konnte man halt nix ausrichten ...

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/*************************************************************************************************/
/* Verwaltet Einheiten-Dialog */


BOOL WINAPI UnitDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static char str[256];
	static LPBMPDATA pBmp;

	switch( message ) {
		case WM_INITDIALOG:
			pBmp = (LPBMPDATA)lParam;
			// Deafult in nm
			if( pBmp == NULL ) {
				EndDialog( hdlg, FALSE );
				return ( TRUE );
#if 0
				CheckRadioButton( hdlg, HUB_AA, HUB_KEINE, HUB_NM );
				SetDlgItemText( hdlg, HUB_X, gcvt( fPiezoSkalX, 5, str ) );
				SetDlgItemText( hdlg, HUB_Y, gcvt( fPiezoSkalY, 5, str ) );
				SetDlgItemText( hdlg, HUB_Z, gcvt( fPiezoSkalZ, 8, str ) );
				SetDlgItemText( hdlg, HUB_LUMI, gcvt( fIntens, 8, str ) );
				SetDlgItemText( hdlg, HUB_Z_UNIT, STR_TOPO_UNIT );
				SetDlgItemText( hdlg, HUB_LUMI_UNIT, STR_LUMI_UNIT );
				SetDlgItemText( hdlg, HUB_X_OFF, "0.0" );
				SetDlgItemText( hdlg, HUB_Y_OFF, "0.0" );
#endif
			}
			else {
				char txt[1024]; // Largest for Windows ...

				// Sonderbehandlung für z-Skalierung ...
				SetDlgItemText( hdlg, HUB_Z_UNIT, pBmp->pSnom[pBmp->iAktuell].Topo.strZUnit );
				SetDlgItemText( hdlg, HUB_LUMI_UNIT, pBmp->pSnom[pBmp->iAktuell].Lumi.strZUnit );
				SetDlgItemText( hdlg, HUB_Z, gcvt( pBmp->pSnom[pBmp->iAktuell].Topo.fSkal, 8, str ) );
				SetDlgItemText( hdlg, HUB_LUMI, gcvt( pBmp->pSnom[pBmp->iAktuell].Lumi.fSkal, 8, str ) );

				// Nun die XY Einheit und Offsets ...
				CheckRadioButton( hdlg, HUB_AA, HUB_KEINE, HUB_NM );
				if( pBmp->pPsi.cNoUnits ) {
					CheckRadioButton( hdlg, HUB_AA, HUB_KEINE, HUB_KEINE );
				}

				SetDlgItemText( hdlg, HUB_X, gcvt( pBmp->pSnom[pBmp->iAktuell].fX, 5, str ) );
				SetDlgItemText( hdlg, HUB_Y, gcvt( pBmp->pSnom[pBmp->iAktuell].fY, 5, str ) );
				CheckDlgButton( hdlg, HUB_SHOW_OFF, pBmp->pPsi.cShowOffset );
				SetDlgItemText( hdlg, HUB_X_OFF, gcvt( pBmp->pSnom[pBmp->iAktuell].fXOff, 7, str ) );
				SetDlgItemText( hdlg, HUB_Y_OFF, gcvt( pBmp->pSnom[pBmp->iAktuell].fYOff, 7, str ) );

				sprintf( txt, "MaxTopo:\t%u\r\nMaxLumi:\t%u\r\n"
				         "Width:\t%0g\r\nHeight:\t%0g\r\n"
				         "Line/s:\t%0g\r\nAngle:\t%0g\r\n"
				         "ZGain:\t%0g\r\nZGainUnit:\t%s\r\n",
				         pBmp->pSnom[pBmp->iAktuell].Topo.uMaxDaten, pBmp->pSnom[pBmp->iAktuell].Lumi.uMaxDaten,
				         pBmp->pPsi.fW, pBmp->pPsi.fH, pBmp->pPsi.fLinePerSec, pBmp->pPsi.fRot,	pBmp->pPsi.fZGain, pBmp->pPsi.cZGainUnit );
				if( pBmp->Typ == SNOMPUTZ ) {
					char extra_txt[256];
					sprintf( extra_txt, "- Snomputz -\r\nProp: %g\r\nInte: %g\r\nDiff: %g\r\nFreq: %gkHz\r\nSetp: %i\r\nCycl: %i",
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->fPid,
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->fInt,
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->fDiff,
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->lFreq/1000.0,
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->iSoll,
					         ( (SNOMPUTZ_SCANDATA*)( pBmp->pExtra ) )->lZyklus );
					lstrcat( txt, extra_txt );
				}
				SetDlgItemText( hdlg, HUB_INFO, (LPSTR)txt );
				SetDlgItemText( hdlg, HUB_KOMM, pBmp->pKommentar );
			}
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_MASZE );
					break;

				case IDOK:
				{
					double faktor = 1.0, fSkalX, fSkalY, fSkalZ;
					LPSNOMDATA pSnom;

					if( IsDlgButtonChecked( hdlg, HUB_AA ) ) {
						faktor = 1e-1;
					}
					if( IsDlgButtonChecked( hdlg, HUB_MKM ) ) {
						faktor = 1e3;
					}
					GetDlgItemText( hdlg, HUB_X, str, 256 );
					fSkalX = atof( str )*faktor;
					GetDlgItemText( hdlg, HUB_Y, str, 256 );
					fSkalY = atof( str )*faktor;
					GetDlgItemText( hdlg, HUB_Z, str, 256 );
					fSkalZ = atof( str )*faktor;

					if( pBmp != NULL  &&  ( pSnom = pAllocNewSnom( pBmp, NONE ) ) != NULL )	{
						pSnom->fX = fSkalX;
						pSnom->fY = fSkalY;
						if( !pSnom->Topo.bSpecialZUnit ) {
							pSnom->Topo.fSkal = fSkalZ;
						}
						else {
							pSnom->Topo.fSkal = fSkalZ/faktor;
						}
						if( !pSnom->Error.bSpecialZUnit ) {
							pSnom->Error.fSkal = fSkalZ;
						}
						else {
							pSnom->Error.fSkal = fSkalZ/faktor;
						}
						GetDlgItemText( hdlg, HUB_LUMI, str, 256 );
						pSnom->Lumi.fSkal = atof( str );
						GetDlgItemText( hdlg, HUB_KOMM, (LPSTR)pBmp->pKommentar, 1024 );
						pBmp->pPsi.cNoUnits = IsDlgButtonChecked( hdlg, HUB_KEINE );
						pBmp->pPsi.cShowOffset = IsDlgButtonChecked( hdlg, HUB_SHOW_OFF );
						GetDlgItemText( hdlg, HUB_X_OFF, str, 256 );
						pSnom->fXOff = atof( str );
						GetDlgItemText( hdlg, HUB_Y_OFF, str, 256 );
						pSnom->fYOff = atof( str );
						GetDlgItemText( hdlg, HUB_Z_UNIT, pSnom->Topo.strZUnit, 8 );
						InvalidateRect( pBmp->hwnd, NULL, TRUE );
						RecalcCache( pBmp, TRUE, TRUE );
					}
					else {
						StatusLineRsc( E_MEMORY );
					}
				}

				case IDCANCEL:
					EndDialog( hdlg, wParam == IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/************************************************************************************/


/* Zeilenmittelung-Dialog */
BOOL WINAPI MittelDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static HWND hwnd;
	static LPBMPDATA pBmp;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pBmp == NULL ) {
				EndDialog( hdlg, wParam != IDOK );
			}

			if( !WhatToDo( pBmp, modus ) ) {
				StatusLineRsc( W_NIX );
				EndDialog( hdlg, wParam != IDOK );
			}
			StatusLineRsc( I_ZEILENMITTEL );
			CheckRadioButton( hdlg, MITTEL_0, MITTEL_FIT, MITTEL_FIT );
			SetDlgItemInt( hdlg, MITTEL_N, 2, FALSE );
			break;

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_LINES );
					break;

				case IDOK:
				{
					LPSNOMDATA pSnom;
					WORD n = 0, err = TRUE;
					HCURSOR	hCurSave;

					// Neue Bitmaps erstellen
					if( ( pSnom = pAllocNewSnom( pBmp, modus ) ) == NULL ) {
						EndDialog( hdlg, FALSE );
						return ( FALSE ) ;
					}
					EndDialog( hdlg, TRUE );
					hCurSave = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

					// Ordnung feststellen
					if( IsDlgButtonChecked( hdlg, MITTEL_0 ) ) {
						n = 1;
					}
					if( IsDlgButtonChecked( hdlg, MITTEL_1 ) ) {
						n = 2;
					}
					if( IsDlgButtonChecked( hdlg, MITTEL_2 ) ) {
						n = 3;
					}

					if( n > 0 ) {
						// Topografie mitteln?
						if( (LONG)pSnom->Topo.puDaten > 256 ) {
							err = MittelBild( pBmp, &( pSnom->Topo ), pSnom->w, pSnom->h, n );
						}
						// Fehler mitteln?
						if( err  &&  (LONG)pSnom->Error.puDaten > 256 ) {
							err = MittelBild( pBmp, &( pSnom->Error ), pSnom->w, pSnom->h, n );
						}
						// Lumineszenz mitteln?
						if( err  &&  (LONG)pSnom->Lumi.puDaten > 256 ) {
							err = MittelBild( pBmp, &( pSnom->Lumi ), pSnom->w, pSnom->h, n );
						}

						if( !err ) {
							FehlerRsc( E_NOT_FITTED );
							FreeBmp( pBmp, pBmp->iAktuell );
						}

						InvalidateRect( hwnd, NULL, FALSE );
						RecalcCache( pBmp, TRUE, TRUE );
					}
					else {
						// Hier Mittelung mit Polynom n-ter Ordnung (n<10)
						n = GetDlgItemInt( hdlg, MITTEL_N, NULL, FALSE );
						if( n > 0  &&  n < 10  &&  IsDlgButtonChecked( hdlg, MITTEL_FIT ) ) {
							// Topografie mitteln?
							if( (LONG)pSnom->Topo.puDaten > 256 ) {
								err = MittelFitBild( pBmp, &( pSnom->Topo ), pSnom->w, pSnom->h, n );
							}
							// Fehler mitteln?
							if( err  &&  (LONG)pSnom->Error.puDaten > 256 ) {
								err = MittelFitBild( pBmp, &( pSnom->Error ), pSnom->w, pSnom->h, n );
							}
							// Lumineszenz mitteln?
							if( err  &&  (LONG)pSnom->Lumi.puDaten > 256 ) {
								err = MittelFitBild( pBmp, &( pSnom->Lumi ), pSnom->w, pSnom->h, n );
							}

							if( !err ) {
								FehlerRsc( E_NOT_FITTED );
								FreeBmp( pBmp, pBmp->iAktuell );
							}
							InvalidateRect( hwnd, NULL, FALSE );
							RecalcCache( pBmp, TRUE, TRUE );
						}
					}
					SetCursor( hCurSave );
				}

					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, wParam != IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/************************************************************************************/


/* Verwaltet Rauhigkeitskorrelationsdialog */
BOOL WINAPI FractalDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static HWND hwnd;
	static LPBMPDATA pBmp;
	static BOOL Is2D, Is3D;

	switch( message ) {
		case WM_INITDIALOG:
			if( Is2D  &&  Is3D ) {
				EndDialog( hdlg, FALSE );
			}
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pBmp == NULL ) {
				EndDialog( hdlg, FALSE );
			}
			if( pBmp->pSnom[pBmp->iAktuell].Topo.puDaten == NULL ) {
				EndDialog( hdlg, FALSE );
			}
			SetDlgItemInt( hdlg, ALPHA_START, 2, FALSE );
			SetDlgItemInt( hdlg, ALPHA_ENDE, (UINT)( ( pBmp->pSnom[pBmp->iAktuell].w*2 )/3 ), FALSE );
			SetDlgItemInt( hdlg, ALPHA_WINKEL, 0, FALSE );
			CheckDlgButton( hdlg, ALPHA_2D, Is3D );
			EnableWindow( GetDlgItem( hdlg, ALPHA_2D ), !Is2D );
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_FRAC );
					break;

				case IDOK:
				{
					BYTE szDatei[256];
					LONG lStart, lEnde, lWinkel;
					HFILE hFile;
					OFSTRUCT of;

					lStart = GetDlgItemInt( hdlg, ALPHA_START, NULL, FALSE );
					lEnde = GetDlgItemInt( hdlg, ALPHA_ENDE, NULL, FALSE );
					lWinkel = GetDlgItemInt( hdlg, ALPHA_WINKEL, NULL, FALSE );

					// Erst hier werden Fehler abgefangen
					szFselHelpStr = STR_HELP_FRAC;
					lstrcpy( szDatei, pBmp->szName );
					ChangeExt( szDatei, ".kor" );
					if( lStart == 0  ||  lEnde < lStart  ||  !CMUFileSave( hwndFrame, STR_FILE_FRAC, szDatei, STR_FILE_ASCII, NULL )
					    ||  ( hFile = OpenFile( szDatei, &of, OF_CREATE ) ) == HFILE_ERROR ) {
						EndDialog( hdlg, FALSE );
						return ( TRUE ) ;
					}

					if( IsDlgButtonChecked( hdlg, ALPHA_2D ) ) {
						//  Winkel-Scan
						double dMean, dRMS, dAnzahl, dTempMean, dTempRMS, fSkal, dm, fX;
						BYTE str[128];
						WORD iAkt = pBmp->iAktuell;
						LPUWORD	pDaten;
						LONG ww, w, h, x, y, tx, ty;
						WORD i;
						HFILE hlFile = hFile;

						// Zuerst den richtigen Quadranten bestimmen
						while( lWinkel < 0 )
							lWinkel += 360;
						while( lWinkel >= 360 )
							lWinkel -= 360;
						if( lWinkel >= 180 ) {
							lWinkel -= 180;
						}
						// Fehler bei 90� abfangen
						if( lWinkel == 90 ) {
							dm = 30000.0;
						}
						else {
							dm = tan( lWinkel*M_PI/180.0 );
						}

						fSkal = pBmp->pSnom[iAkt].Topo.fSkal;
						if( (LONG)pBmp->pSnom[iAkt].Topo.puDaten < 256l ) {
							iAkt = (WORD)pBmp->pSnom[iAkt].Topo.puDaten-1;
						}
						w = pBmp->pSnom[iAkt].w;
						h = pBmp->pSnom[iAkt].h;
						// Get current data
						pDaten = (LPUWORD)pMalloc( w*h*sizeof( UWORD ) );
						if( pDaten == NULL ) {
							StatusLineRsc( E_MEMORY );
							EndDialog( hdlg, FALSE );
							return ( FALSE ) ;
						}
						MemMove( pDaten, pBmp->pSnom[iAkt].Topo.puDaten, w*h*sizeof( UWORD ) );
						if( pDaten == NULL ) {
							StatusLineRsc( E_MEMORY );
							EndDialog( hdlg, FALSE );
							return ( FALSE ) ;
						}
						if( fabs( dm ) <= 1.0 ) {
							fX = pBmp->pSnom[pBmp->iAktuell].fX;
						}
						else {
							fX = pBmp->pSnom[pBmp->iAktuell].fY;
						}

						Is2D = TRUE;
						EndDialog( hdlg, TRUE );
						for( ww = lStart;  ww < lEnde; ww++ ) {
							sprintf( str, GetStringRsc( STR_FRAC_LINE ), ww, lWinkel );
							StatusLine( str );
							dMean = dRMS = dAnzahl = 0.0;
							if( fabs( dm ) <= 1.0 )	{
								for( x = 0;  x < w;        x += ww/2 ) {
									tx = x;
									if( tx >= w-ww ) {
										tx = w-ww-1;
									}
									for( y = 0;  y < h;  y++ ) {
										RMSLine( pDaten, tx, y, w, h, ww, dm, fSkal, (double far*)&dTempMean, (double far*)&dTempRMS );
										dMean += dTempRMS;
										dRMS += ( dTempRMS*dTempRMS );
										dAnzahl += 1.0;
									}
									YieldApp( FALSE );
								}
							}
							else {
								for( y = 0;  y < h;        y += ww/2 ) {
									ty = y;
									if( y >= h-ww ) {
										ty = h-ww-1;
									}
									for( x = 0;  x < w;  x++ ) {
										RMSLine( pDaten, x, ty, w, h, ww, dm, fSkal, &dTempMean, &dTempRMS );
										dMean += dTempRMS;
										dRMS += ( dTempRMS*dTempRMS );
										dAnzahl += 1.0;
									}
									YieldApp( FALSE );
								}
							}
							dMean /= dAnzahl;
							dTempRMS = dMean*dMean*dAnzahl;
							if( dRMS < dTempRMS ) {
								dRMS = 0.0;
							}
							else {
								dRMS = sqrt( ( dRMS-dTempRMS )/( dAnzahl-1.0 ) );
							}
							// Alles mal 1000, da Ergebnisse besser in nm!
							i = sprintf( str, "%lf %lf %lf\xD\xA", ww*fX*1000.0, dMean*1000.0, dRMS*1000.0 );
							_lwrite( hFile, str, i );
						}
						MemFree( pDaten );
						_lclose( hlFile );
						Is2D = FALSE;
					}
					else {
						// Flächenmittelung
						double dMean, dRMS, dAnzahl, dTempMean, dTempRMS, fSkal, fX;
						LPFLOAT	pQuadrate;
						BYTE str[128];
						WORD iAkt = pBmp->iAktuell;
						LPUWORD	pDaten;
						LONG ww, w, h, x, y, tx, ty;
						WORD i;
						HFILE hlFile = hFile;

						fSkal = pBmp->pSnom[iAkt].Topo.fSkal;
						fX = pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].fY;
						if( (LONG)pBmp->pSnom[iAkt].Topo.puDaten < 256l ) {
							iAkt = (WORD)pBmp->pSnom[iAkt].Topo.puDaten-1;
						}
						w = pBmp->pSnom[iAkt].w;
						h = pBmp->pSnom[iAkt].h;
//					if(  (pDaten = (LPUWORD)GlobalLock( GlobalPtrHandle( pBmp->pSnom[iAkt].Topo.puDaten ) ))==NULL  )
						// Get current data
						pDaten = (LPUWORD)pMalloc( w*h*sizeof( UWORD ) );
						if( pDaten == NULL ) {
							StatusLineRsc( E_MEMORY );
							EndDialog( hdlg, FALSE );
							return ( FALSE ) ;
						}
						MemMove( pDaten, pBmp->pSnom[iAkt].Topo.puDaten, w*h*sizeof( UWORD ) );
						pQuadrate = (LPFLOAT)pMalloc( sizeof( float )*w*h );
						if( pQuadrate != NULL )	{ // Ein float-Array mit den Quadraten f�r die RMS-Berechnung bereitstellen
							unsigned long k;

							for( x = 0;  x < w*h;  x++ ) {
								k = pDaten[x];
								pQuadrate[x] = (float)( k*k );
							}
						}

						Is3D = TRUE;
						EndDialog( hdlg, TRUE );
						if( lEnde > w ) {
							lEnde = w;
						}
						if( lEnde > h ) {
							lEnde = h;
						}
						for( ww = lStart;  ww < lEnde; ww++ ) {
							sprintf( str, GetStringRsc( STR_FRAC_RECT ), ww, ww );
							StatusLine( str );
							dMean = dRMS = dAnzahl = 0.0;
							for( y = 0;  y < h;  y += ww ) {
								ty = y;
								if( y+ww >= h ) {
									ty = h-ww-1;
								}
								for( x = 0;  x < w;  x += ww ) {
									if( x+ww >= w ) {
										tx = w-ww-1;
									}
									tx = x;
									RMSArea( pDaten, w, tx, ty, ww, ww, fSkal, &dTempMean, &dTempRMS, pQuadrate );
									dMean += dTempRMS;
									dRMS += ( dTempRMS*dTempRMS );
									dAnzahl += 1.0;
								}
								YieldApp( FALSE );
							}
							dMean /= dAnzahl;
							dTempRMS = dMean*dMean*dAnzahl;
							if( dRMS < dTempRMS ) {
								dRMS = 0.0;
							}
							else {
								dRMS = sqrt( ( dRMS-dTempRMS )/( dAnzahl-1.0 ) );
							}
							// Alles mal 1000, da Ergebnisse besser in nm!
							i = sprintf( str, "%lf %lf %lf\xD\xA", sqrt( ww*ww*fX ), dMean, dRMS );
							_lwrite( hFile, str, i );
						}
						if( pQuadrate ) {
							MemFree( pQuadrate );
						}
						MemFree( pDaten );
						Is3D = FALSE;
						_lclose( hlFile );
					}
					ClearStatusLine();
					return ( TRUE ) ;
				}

				case IDCANCEL:
					EndDialog( hdlg, wParam == IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/


/* Verwaltet Despike-Dialog */
BOOL WINAPI SpikeDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hwnd, hTab;
	static WORKMODE	SpikeModus;
	static BOOL bX[3], bY[3], bLin[3], bUp[3], bLow[3];
	static WORD uPts[3], uUp[3], uLow[3];

	LPSNOMDATA pSnom;
	int i;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			pSnom = pBmp->pSnom+pBmp->iAktuell;
			// Als Windows-Control
			{
				RECT rect;
				TC_ITEM	tci;
				NMHDR nm;
				HWND hTab;

				GetClientRect( hdlg, &rect );
				hTab = CreateWindow( WC_TABCONTROL, "", WS_VISIBLE|WS_TABSTOP|WS_CHILD, 0, 0, rect.right, rect.bottom, hdlg, NULL, hInst, NULL );
				tci.mask = TCIF_TEXT;
				tci.iImage = -1;
				i = 0;
				if( pSnom->Topo.Typ != NONE ) {
					tci.pszText = STR_TOPO;
					SendMessage( hTab, TCM_INSERTITEM, TOPO, (LPARAM)&tci );
					SpikeModus |= TOPO;
					uUp[i++] = pSnom->Topo.uMaxDaten;
				}
				if( pSnom->Lumi.Typ != NONE ) {
					tci.pszText = STR_LUMI;
					SendMessage( hTab, TCM_INSERTITEM, LUMI, (LPARAM)&tci );
					SpikeModus |= LUMI;
					uUp[i++] = pSnom->Lumi.uMaxDaten;
				}
				if( pSnom->Error.Typ != NONE ) {
					tci.pszText = STR_ERROR;
					SendMessage( hTab, TCM_INSERTITEM, ERRO, (LPARAM)&tci );
					SpikeModus |= ERRO;
					uUp[i++] = pSnom->Error.uMaxDaten;
				}
				SendMessage( hTab, TCM_SETCURSEL, 0, 0l );
				nm.hwndFrom = hTab;
				nm.code = TCN_SELCHANGE;
				SendMessage( hdlg, WM_NOTIFY, 0, (LPARAM)&nm );
			}
			// Is überhaupt was zu tun? (Sollte immer wahr sein!)
			if( SpikeModus == NONE ) {
				EndDialog( hdlg, TRUE );
			}
			for( i = 0;  i < 3;  i++ ) {
				bX[i] = bY[i] = bLin[3] = TRUE;
				bUp[i] = bLow[i] = FALSE;
				uPts[i] = 1;
				uLow[i] = 0;
			}
			return ( TRUE );

		case WM_NOTIFY:
		{
			NMHDR *nm = (LPNMHDR)lParam;
			if( nm->code == TCN_SELCHANGING ) {
				break;
				lParam = SendMessage( nm->hwndFrom, TCM_GETCURSEL, 0, 0l );
				bX[lParam] = IsDlgButtonChecked( hdlg, SPIKE_X );
				bY[lParam] = IsDlgButtonChecked( hdlg, SPIKE_Y );
				uPts[lParam] = GetDlgItemInt( hdlg, SPIKE_WEITE, NULL, FALSE );
				bLow[lParam] = IsDlgButtonChecked( hdlg, SPIKE_LOW_OK );
				uLow[lParam] = GetDlgItemInt( hdlg, SPIKE_LOW, NULL, FALSE );
				bUp[lParam] = IsDlgButtonChecked( hdlg, SPIKE_UP_OK );
				uUp[lParam] = GetDlgItemInt( hdlg, SPIKE_UP, NULL, FALSE );
				bLin[lParam] = IsDlgButtonChecked( hdlg, SPIKE_INTERPOL );
			}
			if( nm->code != TCN_SELCHANGE ) {
				break;
			}
			wParam = SendMessage( nm->hwndFrom, TCM_GETCURSEL, 0, 0l );
		}
			CheckDlgButton( hdlg, SPIKE_X, bX[wParam] );
			CheckDlgButton( hdlg, SPIKE_Y, bY[wParam] );
			SetDlgItemInt(	hdlg, SPIKE_WEITE, uPts[wParam], FALSE );
			CheckDlgButton( hdlg, SPIKE_LOW_OK, bLow[wParam] );
			SetDlgItemInt(	hdlg, SPIKE_LOW, uLow[wParam], FALSE );
			CheckDlgButton( hdlg, SPIKE_UP_OK, bUp[wParam] );
			SetDlgItemInt(	hdlg, SPIKE_UP, uUp[wParam], FALSE );
			CheckDlgButton( hdlg, SPIKE_INTERPOL, bLin[wParam] );
			break;

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_SPIKE );
					break;

				// Nun Despiken ...
				case IDOK:
				{
					NMHDR nm;
					// Update der Parameter erzwingen ...
					nm.hwndFrom = hTab;
					nm.code = TCN_SELCHANGING;
					SendMessage( hdlg, WM_NOTIFY, 0, (LPARAM)&nm );
					EndDialog( hdlg, TRUE );
					if( ( pSnom = pAllocNewSnom( pBmp, modus ) ) == NULL ) {
						StatusLineRsc( E_MEMORY );
						return ( FALSE ) ;
					}
					WarteMaus();
					i = 0;
					StatusLineRsc( I_DESPIKE );
					if( SpikeModus&TOPO ) {
						BildDespike( &( pSnom->Topo ), pSnom->w, pSnom->h, uPts[i], uPts[i], uLow[i], uUp[i],
						             bX[i], bY[i], bLow[i], bUp[i], bLin[i] );
						i++;
					}
					if( SpikeModus&LUMI ) {
						BildDespike( &( pSnom->Lumi ), pSnom->w, pSnom->h, uPts[i], uPts[i], uLow[i], uUp[i],
						             bX[i], bY[i], bLow[i], bUp[i], bLin[i] );
						i++;
					}
					if( SpikeModus&ERRO ) {
						BildDespike( &( pSnom->Error ), pSnom->w, pSnom->h, uPts[i], uPts[i], uLow[i], uUp[i],
						             bX[i], bY[i], bLow[i], bUp[i], bLin[i] );
						i++;
					}
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					NormalMaus();
					ClearStatusLine();
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/


/* Verwaltet Median-Dialog */
BOOL WINAPI MedianDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hwnd;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			// Was Mitteln?
			if( !WhatToDo( pBmp, modus ) ) {
				StatusLineRsc( W_NIX );
				EndDialog( hdlg, wParam != IDOK );
			}
			// und Button polieren: Zuerst Quelle und Ziel
			CheckRadioButton( hdlg, MEDIAN_3ZEILEN, MEDIAN_5X5, MEDIAN_3ZEILEN );
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_SPIKE );
					break;

				// Nun Despiken ...
				case IDOK:
				{
					LPSNOMDATA pSnom;
					WORD zeilen;
					HCURSOR	hCurSave;

					EndDialog( hdlg, TRUE );
					if( ( pSnom = pAllocNewSnom( pBmp, modus ) ) == NULL ) {
						StatusLineRsc( E_MEMORY );
						return ( FALSE ) ;
					}

					hCurSave = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

					zeilen = IsDlgButtonChecked( hdlg, MEDIAN_3ZEILEN )*3 +
					         IsDlgButtonChecked( hdlg, MEDIAN_5ZEILEN )*5 +
					         IsDlgButtonChecked( hdlg, MEDIAN_7ZEILEN )*7;
					if( zeilen > 0 ) {
						if( (LONG)pSnom->Topo.puDaten > 256 ) {
							BildMedianSpalten( pSnom->Topo.puDaten, pSnom->w, pSnom->h, zeilen );
						}
						if( (LONG)pSnom->Error.puDaten > 256 ) {
							BildMedianSpalten( pSnom->Error.puDaten, pSnom->w, pSnom->h, zeilen );
						}
						if( (LONG)pSnom->Lumi.puDaten > 256 ) {
							BildMedianSpalten( pSnom->Lumi.puDaten, pSnom->w, pSnom->h, zeilen );
						}
					}

					zeilen = IsDlgButtonChecked( hdlg, MEDIAN_2X2 )*1 +
					         IsDlgButtonChecked( hdlg, MEDIAN_3X3 )*3 +
					         IsDlgButtonChecked( hdlg, MEDIAN_5X5 )*5;
					if( zeilen > 0 ) {
						if( (LONG)pSnom->Topo.puDaten > 256 ) {
							BildMedian( pSnom->Topo.puDaten, pSnom->w, pSnom->h, zeilen );
						}
						if( (LONG)pSnom->Error.puDaten > 256 ) {
							BildMedian( pSnom->Error.puDaten, pSnom->w, pSnom->h, zeilen );
						}
						if( (LONG)pSnom->Lumi.puDaten > 256 ) {
							BildMedian( pSnom->Lumi.puDaten, pSnom->w, pSnom->h, zeilen );
						}
					}

					if( (LONG)pSnom->Topo.puDaten > 256 ) {
						BildMax( &( pSnom->Topo ), pSnom->w, pSnom->h );
					}
					if( (LONG)pSnom->Error.puDaten > 256 ) {
						BildMax( &( pSnom->Error ), pSnom->w, pSnom->h );
					}
					if( (LONG)pSnom->Lumi.puDaten > 256 ) {
						BildMax( &( pSnom->Lumi ), pSnom->w, pSnom->h );
					}
					RecalcCache( pBmp, TRUE, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					SetCursor( hCurSave );
					ClearStatusLine();
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/


/* Winkelverteilung */
BOOL WINAPI WinkelDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_INITDIALOG:
			// Verwaltet auch Dialog mit Zahlen!
			if( (long)lParam < 0 ) {
				CheckDlgButton( hdlg, WINKEL_LOGARITHMUS, TRUE );
			}
			SetDlgItemInt( hdlg, IDD_ZAHL, 90, FALSE );
#ifdef _WIN32
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, -1, hInst, GetDlgItem( hdlg, IDD_ZAHL ), 90, 5, 90 );
#endif
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDOK:
				{
					int i;

					i = GetDlgItemInt( hdlg, IDD_ZAHL, NULL, FALSE );
					if( IsDlgButtonChecked( hdlg, WINKEL_LOGARITHMUS ) ) {
						i = -i;
					}
					EndDialog( hdlg, i );
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, 0 );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


BOOL WINAPI GrossDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hwnd;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			SetDlgItemInt( hdlg, GROSS_W, pBmp->pSnom[pBmp->iAktuell].w, FALSE );
			SetDlgItemInt( hdlg, GROSS_H, pBmp->pSnom[pBmp->iAktuell].h, FALSE );
			SetDlgItemText( hdlg, GROSS_FAKTOR, "1.0" );
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDOK:
				{
					LONG NeuW, NeuH = 0;
					char str[16];
					LPSNOMDATA pSnom;

					if( IsDlgButtonChecked( hdlg, GROSS_X_Y ) ) {
						NeuW = GetDlgItemInt( hdlg, GROSS_W, NULL, FALSE );
						NeuH = GetDlgItemInt( hdlg, GROSS_H, NULL, FALSE );
					}
					if( IsDlgButtonChecked( hdlg, GROSS_FAKTOR_OK ) ) {
						GetDlgItemText( hdlg, GROSS_FAKTOR, str, 16 );
						NeuW = atof( str )*pBmp->pSnom[pBmp->iAktuell].w+0.5;
						NeuH = atof( str )*pBmp->pSnom[pBmp->iAktuell].h+0.5;
					}
					if( NeuW > 0  &&  NeuH > 0  &&  ( pSnom = pAllocNewSnom( pBmp, TOPO|ERRO|LUMI ) ) != NULL ) {
						StatusLineRsc( I_GROSS );

						if( pSnom->Topo.Typ == TOPO ) {
							BildResize(  &( pSnom )->Topo, pSnom->w, pSnom->h, NeuW, NeuH );
						}
						if( pSnom->Error.Typ == ERRO ) {
							BildResize(  &( pSnom->Error ), pSnom->w, pSnom->h, NeuW, NeuH );
						}
						if( pSnom->Lumi.Typ == LUMI ) {
							BildResize(  &( pSnom->Lumi ), pSnom->w, pSnom->h, NeuW, NeuH );
						}
						pSnom->fX /= (double)NeuW/(double)pSnom->w;
						pSnom->fY /= (double)NeuH/(double)pSnom->h;
						pSnom->w = NeuW;
						pSnom->h = NeuH;

						// Um ungültige Pointer zu vermeiden => Scanline auf Null
						pBmp->bIsScanLine = FALSE;
						pBmp->lMaxScan = 0;
						pBmp->rectScan[0].left = pBmp->rectScan[0].right = 0;
						pBmp->rectScan[0].top =	pBmp->rectScan[0].bottom = 0;

						// Maske wird gelöscht ...
						if( pBmp->pMaske ) {
							MemFree( pBmp->pMaske );
							pBmp->pMaske = NULL;
							pBmp->wMaskeW = 0;
						}
						RecalcCache( pBmp, TRUE, TRUE );
					}
					EndDialog( hdlg, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					ClearStatusLine();
				}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


// 4.7.98


/* Tutor Dialoge */
BOOL WINAPI TutorDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static HWND hwnd;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_LINES );
					break;

				case IDOK:
					EndDialog( hdlg, TRUE );
					if( IsDlgButtonChecked( hdlg, IDM_ZEILENMITTEL ) ) {
						SendMessage( hwnd, WM_COMMAND, IDM_ZEILENMITTEL, 0l );
					}
					if( IsDlgButtonChecked( hdlg, IDM_DIFF_NULL ) ) {
						SendMessage( hwnd, WM_COMMAND, IDM_DIFF_NULL, 0l );
					}
					if( IsDlgButtonChecked( hdlg, IDM_NULL ) ) {
						SendMessage( hwnd, WM_COMMAND, IDM_NULL, 0l );
					}
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/* Verwaltet Höhenprofildialog: Was speichern? */
BOOL WINAPI ProfilDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_INITDIALOG:
			CheckDlgButton( hdlg, DLG_TOPO, ( wProfilMode&TOPO ) != 0 );
			CheckDlgButton( hdlg, DLG_ERROR, ( wProfilMode&ERRO ) != 0 );
			CheckDlgButton( hdlg, DLG_LUMI, ( wProfilMode&LUMI ) != 0 );
			CheckDlgButton( hdlg, PROFIL_DISTANZ, ( wProfilFlags&P_DIST ) != 0 );
			CheckDlgButton( hdlg, PROFIL_X, ( wProfilFlags&P_X ) != 0 );
			CheckDlgButton( hdlg, PROFIL_Y, ( wProfilFlags&P_Y ) != 0 );
			CheckDlgButton( hdlg, PROFIL_WERTE, ( wProfilFlags&P_Z ) != 0 );
			CheckDlgButton( hdlg, PROFIL_AUTOKORRELATION, ( wProfilFlags&P_AUTOKORRELATION ) != 0 );
			CheckDlgButton( hdlg, PROFIL_AMPLITUDE, ( wProfilFlags&P_PSD ) != 0 );
			return ( TRUE );

		case WM_COMMAND:
			switch( wParam ) {
				case IDHELP:
				{
					BYTE str[256];
					if( GetDlgItemText( hdlg, HILFETEXT, str, 256 ) > 0 ) {
						WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)str );
					}
					break;
				}

				case IDOK:
					wProfilMode = NONE;
					if( IsDlgButtonChecked( hdlg, DLG_TOPO ) ) {
						wProfilMode = TOPO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_ERROR ) ) {
						wProfilMode |= ERRO;
					}
					if( IsDlgButtonChecked( hdlg, DLG_LUMI ) ) {
						wProfilMode |= LUMI;
					}
					wProfilFlags = NONE;
					if( IsDlgButtonChecked( hdlg, PROFIL_DISTANZ ) ) {
						wProfilFlags = P_DIST;
					}
					if( IsDlgButtonChecked( hdlg, PROFIL_Y ) ) {
						wProfilFlags |= P_X;
					}
					if( IsDlgButtonChecked( hdlg, PROFIL_X ) ) {
						wProfilFlags |= P_Y;
					}
					if( IsDlgButtonChecked( hdlg, PROFIL_WERTE ) ) {
						wProfilFlags |= P_Z;
					}
					if( IsDlgButtonChecked( hdlg, PROFIL_AUTOKORRELATION ) ) {
						wProfilFlags |= P_AUTOKORRELATION;
					}
					if( IsDlgButtonChecked( hdlg, PROFIL_AMPLITUDE ) ) {
						wProfilFlags |= P_PSD;
					}
					EndDialog( hdlg, TRUE );
					return ( TRUE ) ;

				case IDCANCEL:
					EndDialog( hdlg, FALSE );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/

// berechnet die Filtercurve für die FFT-Filterung ...
void CalcFFTResponse( float *pf, int iAnzahl, int iLowF, int iLowW, int iHighF, int iHighW )
{
	int i, j;

	if( iLowF >= iHighF ) {
		// Durchlassen
		for( i = 0;  i < iAnzahl  &&  i < iHighF-iHighW;  i++ ) {
			pf[i] = 1.0;
		}
		// Fallende Flanke
		j = i;
		for(  ; i < iAnzahl  &&  i < iHighF+iHighW;  i++ ) {
			pf[i] = 1.0-(float)( i-j )/(float)( 2*iHighW );
		}
		// Null
		for(  ; i < iAnzahl  &&  i < iLowF-iLowW;  i++ ) {
			pf[i] = 0.0;
		}
		// Steigende Flanke
		j = i;
		for(  ; i < iAnzahl  &&  i < iLowF+iLowW;  i++ ) {
			pf[i] = (float)( i-j )/(float)( 2*iLowW );
		}
		// Durchlassen
		for(  ; i < iAnzahl;  i++ ) {
			pf[i] = 1.0;
		}
	}
	else {
		// Null
		for( i = 0; i < iAnzahl  &&  i < iLowF-iLowW;  i++ ) {
			pf[i] = 0.0;
		}
		// Steigende Flanke
		j = i;
		for(  ; i < iAnzahl  &&  i < iLowF+iLowW;  i++ ) {
			pf[i] = (float)( i-j )/(float)( 2*iLowW );
		}
		// Durchlassen
		for(  ;  i < iAnzahl  &&  i < iHighF-iHighW;  i++ ) {
			pf[i] = 1.0;
		}
		// Fallende Flanke
		j = i;
		for(  ; i < iAnzahl  &&  i < iHighF+iHighW;  i++ ) {
			pf[i] = 1.0-(float)( i-j )/(float)( 2*iHighW );
		}
		// Null
		for(  ; i < iAnzahl;  i++ ) {
			pf[i] = 0.0;
		}
	}
}


/* 3D-Ansicht-Dialog */
int WINAPI FFTDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hAnsicht, hLowF, hHighF, hwnd;
	static double fFSpan;
	static BYTE str[16];

	static LPFLOAT fPts;
	static int iSize = 1;
	static long lW;

	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			lW = pBmp->pSnom[pBmp->iAktuell].w;
			while( iSize < lW )
				iSize *= 2;
			fFSpan = pBmp->pSnom[0].w*pBmp->pPsi.fLinePerSec;       // Obere Grenzfrequenz *2.0, da hin +r�ck, /2.0, da FFT nur bis halbe Frequenz ...
			if( fFSpan == 0 ) {
				fFSpan = lW;
			}
			fPts = pMalloc( sizeof( float )*iSize );

			hAnsicht = GetDlgItem( hdlg, FFT_FKT );
			sprintf( str, "%8.2f", fFSpan );
			SetDlgItemText( hdlg, FFT_LOW_FREQ, str );
			SetDlgItemInt( hdlg, FFT_LOW_W, 0, FALSE );
			sprintf( str, "%8.2f", fFSpan*0.9 );
			SetDlgItemText( hdlg, FFT_HIGH_FREQ, str );
			SetDlgItemInt( hdlg, FFT_HIGH_W, 1, FALSE );
#ifdef _WIN32
			hLowF = CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                             hdlg, FFT_LOW_FREQ, hInst, GetDlgItem( hdlg, FFT_LOW_FREQ ), 1024, 0, 1024 );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, FFT_LOW_W, hInst, GetDlgItem( hdlg, FFT_LOW_W ), 100, 0, 0 );
			hHighF = CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                              hdlg, FFT_HIGH_FREQ, hInst, GetDlgItem( hdlg, FFT_HIGH_FREQ ), 1024, 0, 922 );
			CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                     hdlg, FFT_HIGH_W, hInst, GetDlgItem( hdlg, FFT_HIGH_W ), 100, 0, 1 );
#endif
			return ( TRUE ) ;

		case WM_PAINT:
			{
				SIZE size;
				HANDLE hOld;
				HDC hDC;
				RECT rect;
				HRGN hRgn;
				int i, x, y, w;      // Offsets zum Zentrieren
				int iLow, iLW, iHigh, iHW;

				GetDlgItemText( hdlg, FFT_LOW_FREQ, str, 16 );
				iLow = (int)( iSize*atof( str )/fFSpan+0.5 );
				iLW = ( iSize*GetDlgItemInt( hdlg, FFT_LOW_W, NULL, FALSE ) )/100;
				GetDlgItemText( hdlg, FFT_HIGH_FREQ, str, 16 );
				iHigh = (int)( iSize*atof( str )/fFSpan+0.5 );
				iHW = ( iSize*GetDlgItemInt( hdlg, FFT_HIGH_W, NULL, FALSE ) )/100;
				CalcFFTResponse( fPts, iSize, iLow, iLW, iHigh, iHW );

				InvalidateRect( hAnsicht, NULL, TRUE );
				UpdateWindow( hAnsicht );
				hDC = GetDC( hAnsicht );
				GetClientRect( hAnsicht, &rect );
				GetTextExtentPoint( hDC, "Ty", 2, &size );
				x = rect.left+5;
				y = rect.bottom-size.cy*3;
				w = rect.right-rect.left-10;
				hRgn = CreateRectRgnIndirect( &rect );
				hOld = SelectObject( hDC, GetStockObject( WHITE_BRUSH ) );
				SelectObject( hDC, hRgn );
				PaintRgn( hDC, hRgn );
				hOld = SelectObject( hDC, hOld );
				SelectClipRgn( hDC, hRgn );

				hOld = SelectObject( hDC, GetStockObject( BLACK_PEN ) );
				DrawHorizontalAxis( hDC, x, y, w, 2, 4, fFSpan, 0, STR_F_UNIT, NULL );

				MoveToEx( hDC, x, y, NULL );
				LineTo( hDC, x+w, y );
				MoveToEx( hDC, x, y-fPts[0]*size.cy, NULL );
				for( i = 1;  i < iSize;  i++ ) {
					LineTo(  hDC, x+( i*w )/iSize, y-fPts[i]*size.cy );
				}

				SelectObject( hDC, hOld );
				DeleteRgn( hRgn );
				ReleaseDC( hAnsicht, hDC );
			}
			break;

		case WM_VSCROLL:
			if( hLowF == (HWND)lParam ) {
				sprintf( str, "%8.2f", fFSpan*LOWORD( SendMessage( hLowF, UDM_GETPOS, 0, 0 ) )/1024.0 );
				SetDlgItemText( hdlg, FFT_LOW_FREQ, str );
			}
			if( hHighF == (HWND)lParam ) {
				sprintf( str, "%8.2f", fFSpan*LOWORD( SendMessage( hHighF, UDM_GETPOS, 0, 0 ) )/1024.0 );
				SetDlgItemText( hdlg, FFT_HIGH_FREQ, str );
			}
			break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) ) {
				case FFT_LOW_FREQ:
				case FFT_LOW_W:
				case FFT_HIGH_FREQ:
				case FFT_HIGH_W:
#ifdef BIT32
					if( HIWORD( wParam ) == EN_CHANGE )
#else
					if( HIWORD( lParam ) == EN_CHANGE )
#endif
					{
						SendMessage( hdlg, WM_PAINT, 0, 0 );
					}
					break;

				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_FFT );
					break;

				case IDOK:
				{
					LPSNOMDATA pSnom;

					// Filterarry fPts ist gefüllt, ansonsten einfach:
					// SendMessage( hdlg, WM_PAINT, 0, 0 );
					// FFT-Filterung
					if( ( pSnom = pAllocNewSnom( pBmp, modus ) ) == NULL ) {
						FehlerRsc( E_MEMORY );
					}
					else {
						StatusLineRsc( I_FFT );
						WarteMaus();
						if( (LONG)pSnom->Topo.puDaten > 256 ) {
							BildFFTFilter( &( pSnom->Topo ), pSnom->w, pSnom->h, fPts, iSize );
						}
						if( (LONG)pSnom->Error.puDaten > 256 ) {
							BildFFTFilter( &( pSnom->Error ), pSnom->w, pSnom->h, fPts, iSize );
						}
						if( (LONG)pSnom->Lumi.puDaten > 256 ) {
							BildFFTFilter( &( pSnom->Lumi ), pSnom->w, pSnom->h, fPts, iSize );
						}
						NormalMaus();
						RecalcCache( pBmp, TRUE, TRUE );
						InvalidateRect( hwnd, NULL, FALSE );
						ClearStatusLine();
					}
				}

				case IDCANCEL:
					MemFree( fPts );
					EndDialog( hdlg, wParam != IDOK );
					return ( TRUE ) ;
			}
	}
	return ( FALSE );
}


/* 3D-Ansicht-Dialog */
int WINAPI VolumeDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static HWND hTolerance, hResult, hwnd;
	static LPBYTE pMaske;
	extern HWND hModeLess;

	switch( message ) {
		case WM_INITDIALOG:
			pMaske = NULL;
			if( hModeLess == NULL ) {
				hModeLess = hdlg;
			}
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			SetDlgItemInt( hdlg, VOLUME_TOLERANCE, 5, FALSE );
#ifdef _WIN32
			hTolerance = CreateUpDownControl( WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 10, 10,
			                                  hdlg, 0, hInst, GetDlgItem( hdlg, VOLUME_TOLERANCE ), 100, 0, 5 );
#endif
			return ( TRUE ) ;

		case WM_CLOSE:
		case WM_DESTROY:
			EndDialog( hdlg, 0 );
			if( hModeLess == hdlg ) {
				hModeLess = NULL;
			}
			// free any memory here ...
			break;

		case WM_VSCROLL:
			if( hTolerance == (HWND)lParam ) {
				SetDlgItemInt( hdlg, VOLUME_TOLERANCE, LOWORD( SendMessage( hTolerance, UDM_GETPOS, 0, 0 ) ), FALSE );
			}
			break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) ) {
				case VOLUME_TOLERANCE:
#ifdef BIT32
					if( HIWORD( wParam ) != EN_CHANGE )
#else
					if( HIWORD( lParam ) != EN_CHANGE )
#endif
					{
						break;
					}
					break;

				case VOLUME_RECALC:
					// Recalc volume and area
					pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
					if( pBmp->lMaxScan < 1 ) {
						; //	SetDlgItemText( hdlg, VOLUME_RESULT, STR_ERROR_NO_SCANLINE );
					}
					else if( modus&TOPO  &&  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten ) {
						LONG w = pBmp->pSnom[pBmp->iAktuell].w, h = pBmp->pSnom[pBmp->iAktuell].h;
						LPUWORD	puData;
						UWORD iLow, iHigh;
						// Get valid pointer to Bitmap, save Mask pointer
						if( pBmp->pMaske == NULL ) {
							pBmp->pMaske = pMalloc( ( ( w+7 )/8 )*h );
							pBmp->wMaskeW = ( w+7 )/8;
						}
						puData = pBmp->pSnom[pBmp->iAktuell].Topo.puDaten;
						if( (LONG)puData < 256 ) {
							puData = pBmp->pSnom[(WORD)pBmp->pSnom[pBmp->iAktuell].Topo.puDaten-1].Topo.puDaten;
						}
						iLow = puData[w*( pBmp->rectScan[0].top )+pBmp->rectScan[0].left];
						iHigh = puData[w*( pBmp->rectScan[0].bottom )+pBmp->rectScan[0].right];
						iHigh += (UWORD)( ( ( (LONG)iLow-(LONG)iHigh )*GetDlgItemInt( hdlg, VOLUME_TOLERANCE, NULL, FALSE ) )/100l );
						MarkUpperArea( puData, w, h, pBmp->pMaske, pBmp->wMaskeW, pBmp->rectScan[0].left, pBmp->rectScan[0].top, iHigh );
						InvalidateRect( hwnd, NULL, FALSE );
						RecalcCache( pBmp, TRUE, TRUE );
					}
					break;
			}
	}
	return ( FALSE );
}


/***************************************************************************************/


/* Verwaltet QD-Dialog */
DWORD WINAPI QDDialog( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPBMPDATA pBmp;
	static LPSNOMDATA pSnom;
	static HWND hwnd, hfStart;
	static BOOL bCenter = TRUE;
	BYTE str[128];
	static char unit_str[128], result_str[128];
	WORD i = 0;

	i = 0;
	switch( message ) {
		case WM_INITDIALOG:
			hwnd = (HWND)lParam;
			pBmp = (LPBMPDATA)GetWindowLong( hwnd, 0 );
			if( pBmp == NULL  &&  pBmp->pSnom[0].Topo.puDaten == 0 ) {
				EndDialog( hdlg, wParam != IDOK );
			}
			// Ok, valid pointers ...
			pSnom = pBmp->pSnom+pBmp->iAktuell;
			// init controls
			CheckDlgButton( hdlg, QD_CENTER_ON_DOT, bCenter );
			// Scrolly Initialisieren
			hfStart = GetDlgItem( hdlg, QD_SCROLL_TOLERANCE );
			SetScrollRange( hfStart, SB_CTL, 1, pSnom->Topo.uMaxDaten/4, FALSE );
			SetScrollPos( hfStart, SB_CTL, pBmp->dot_radius, TRUE );
			if(  pBmp->dot_radius==0  &&  pBmp->dot_number==0  ) {
				SetScrollPos( hfStart, SB_CTL, pSnom->Topo.uMaxDaten/16+1, TRUE );
				goto RecalcDot;
			}
			else {
				sprintf( unit_str, "%lf.4 %s", pSnom->Topo.fSkal*GetScrollPos( hfStart, SB_CTL ), pSnom->Topo.strZUnit ? pSnom->Topo.strZUnit : "nm" );
				SetDlgItemText( hdlg, QD_EDIT_TOLERANCE, unit_str );
				sprintf( result_str, "Count %i  density %.3e/cm²", pBmp->dot_number, (double)pBmp->dot_number*1e14/( pSnom->fX*pSnom->w*pSnom->fY*pSnom->h ) );
				SetDlgItemText( hdlg, QD_RESULT, result_str );
			}
			return ( TRUE );


		case WM_HSCROLL:
		{
			int iOffset = GetScrollPos( GET_SCROLL_HANDLE( lParam ), SB_CTL );
			switch( LOWORD( wParam ) ) {
				case SB_LINEDOWN:
					iOffset++;
					break;

				case SB_LINEUP:
					iOffset--;
					break;

				case SB_PAGEDOWN:
					iOffset += 16;
					break;

				case SB_PAGEUP:
					iOffset -= 16;
					break;

				case SB_BOTTOM:
					iOffset = pSnom->Topo.uMaxDaten/4;
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
			SendMessage( hwnd, WM_NOTIFY, 0, 0 );
			goto RecalcDot;
		}
		break;

		case WM_NOTIFY:

		case WM_COMMAND:
			switch( wParam ) {
				case QD_CENTER_ON_DOT:
					goto RecalcDot;
		
				case IDHELP:
					WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)STR_HELP_FARBEN );
					break;

				case IDOK:
				{
					EndDialog( hdlg, TRUE );
					InvalidateRect( hwnd, NULL, FALSE );
					return ( TRUE ) ;
				}
			}
	}
	return ( FALSE );

RecalcDot:
	{
		int iOffset = GetScrollPos( hfStart, SB_CTL );
		BOOL new_center = IsDlgButtonChecked( hdlg, QD_CENTER_ON_DOT );
		if( iOffset != pBmp->dot_radius  ||  new_center != bCenter  ) {
			double Median, MedianRMS;
			LPUWORD pData = GetDataPointer( pBmp, TOPO );

			bCenter = new_center;
			pBmp->dot_radius = iOffset;

			MeadianArea( pData, pSnom->w, 0, 0, pSnom->w, pSnom->h, 1.0, pSnom->Topo.uMaxDaten, &Median, &MedianRMS );
			pBmp->dot_mean_level = Median;
			pBmp->dot_quantisation = MedianRMS;
			if( pBmp->dot_number > 0 ) {
				MemFree( pBmp->dot_histogramm );
			}
			pBmp->dot_number = 0;
			pBmp->dot_histogramm_count = 0;
			pBmp->dot_histogramm = NULL;
			pBmp->dot_number = ListOfMaxima( pData, pSnom->w, pSnom->h, pSnom->Topo.uMaxDaten, pBmp->dot_radius, &( pBmp->dot_histogramm ) );
			pBmp->dot_histogramm_count = pBmp->dot_number;
			CalcDotRadius( pData, pSnom->w, pSnom->h, pBmp->dot_mean_level, DOT_AVERAGE, pBmp->dot_number, pBmp->dot_histogramm, pBmp->dot_quantisation, bCenter );
			sprintf( unit_str, "%lf.4 %s", pSnom->Topo.fSkal*pBmp->dot_radius, pSnom->Topo.strZUnit ? pSnom->Topo.strZUnit : "nm" );
			SetDlgItemText( hdlg, QD_EDIT_TOLERANCE, unit_str );
			sprintf( result_str, "Count %i  density %.3e/cm²", pBmp->dot_number, (double)pBmp->dot_number*1e14/( pSnom->fX*pSnom->w*pSnom->fY*pSnom->h ) );
			SetDlgItemText( hdlg, QD_RESULT, result_str );
			pBmp->bCountDots = ( pBmp->dot_number > 0 );
#ifdef BIT32
			SendMessage( hwndToolbar, TB_CHECKBUTTON, IDM_DOT_MODE, pBmp->bCountDots );
#endif
			sprintf( str, GetStringRsc( I_DOTS ), pBmp->dot_number, (double)pBmp->dot_number*1e14/( pBmp->pSnom[pBmp->iAktuell].fX*pBmp->pSnom[pBmp->iAktuell].w*pBmp->pSnom[pBmp->iAktuell].fY*pBmp->pSnom[pBmp->iAktuell].h ) );
			StatusLine( str );
			InvalidateRect( hwnd, NULL, FALSE );
			if( pBmp->dot_number > 0 ) {
				EnableMenuItem( hMenuBmp, IDM_DOTS_REMOVE, MF_BYCOMMAND|MF_ENABLED );
				EnableMenuItem( hMenuBmp, IDM_DOTS_CLEAR, MF_BYCOMMAND|MF_ENABLED );
				EnableMenuItem( hMenuBmp, IDM_DOTS_SAVE, MF_BYCOMMAND|MF_ENABLED );
			}
			else {
				EnableMenuItem( hMenuBmp, IDM_DOTS_REMOVE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );
				EnableMenuItem( hMenuBmp, IDM_DOTS_CLEAR, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );
				EnableMenuItem( hMenuBmp, IDM_DOTS_SAVE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );
			}
			SetScrollPos( hfStart, SB_CTL, (WORD)( pBmp->dot_radius ), TRUE );
			CheckDlgButton( hdlg, QD_CENTER_ON_DOT, bCenter );
		}
	}
	return message==WM_INITDIALOG;
}


/***************************************************************************************/

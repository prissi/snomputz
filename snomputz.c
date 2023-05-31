/*--------------------------------------------------------
        SNOMPUTZ	Routinen zum Filtern vom Snom-Dateien (jaja, das war noch Version 0.3 ...
        --------------------------------------------------------*/


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlobj_core.h>   // für Drag&Drop
#if defined( USE_CTL3D )
#include "ctl3d.h"
#endif  // USE_CTL3D
#if !defined( __WIN32__ ) && !defined( _WIN32 )
#include <shellapi.h>   // für Drag&Drop
#include <shlobj_core.h>   // für Drag&Drop
#include <print.h>
#include <commctrl.h>
#else
#include <commctrl.h>
#endif

#include "myportab.h"
#define __main
#include "snomputz.h"
#include "snom-var.h"
#include "snomlang.h"

#include "filebox.h"
#include "snom-win.h"
#include "snom-mem.h"
#include "snom-dat.h"
#include "snom-dlg.h"
#include "snom-dsp.h"
#include "snom-avi.h"
#include "dsp-mes.h"
#include "snom-prg.h"


// Info-Dialog 15 Sekunden zeigen
#define EVAL_TIME	15

#ifndef LPNMTTDISPINFO
#define LPNMTTDISPINFO LPTOOLTIPTEXT
#define TTN_GETDISPINFO TTN_NEEDTEXT
#endif

#ifdef __WIN32__
#define TBSTYLE_TOOLTIPS        0x0100
#define TTN_FIRST               ( 0U-520U )       // tooltips
#define TTN_NEEDTEXT           ( TTN_FIRST - 0 )
#endif


BOOL WINAPI InfoDlgProc( HWND hdlg, UINT message, UINT wParam, LONG lParam );
long WINAPI FrameWndProc( HWND, UINT, UINT, LONG ) ;
BOOL WINAPI CloseEnumProc( HWND, LONG ) ;
long WINAPI BmpWndProc( HWND, UINT, UINT, LONG ) ;
long WINAPI RectWndProc( HWND, UINT, UINT, LONG ) ;


/**** Die Messroutine (aus DSP-MES.C) ****/
LRESULT WINAPI MessBmpWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


// aus snom-bmp.c
BOOL OpenCreateImage( LPCSTR datei );


// Einige Defines, die *nicht* in der RC-Datei benötigt werden!
#define INIT_MENU_POS	0
// Bitmapnamen werden an dieses Menü gehängt!
#define BITMAP_MENU_POS 7

// Identifier für Tool/Statusbar
#define ID_TOOLBAR	17
#define ID_STATUSBAR 18

#define iMaxTool 26

TBBUTTON pToolbarButtons[iMaxTool] = {
	{ 6, IDM_MESS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 0, IDM_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 1, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 2, IDM_DRUCKEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 9, IDM_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 18, IDM_COPY_SCANLINE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 19, IDM_DOT_MODE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 7, IDM_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },    // Alt: Bild 3
	{ 8, IDM_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },    // Alt: Bild 4
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
#if 0
	{ 15, IDM_SMALLER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 16, IDM_LARGER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
#endif
	{ 10, IDM_DREHEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 11, IDM_ZEILENMITTEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 21, IDM_MEDIAN3, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 22, IDM_MEDIAN5, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 12, IDM_MATHE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 5, IDM_FALSCHFARBEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 17, IDM_MASZE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 13, IDM_3DANSICHT_DEFAULT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
//	{ 14, IDM_MASZE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
};


// Recentdateinamen ab dieser Position ...
#define INIT_MENU_RECENT	5
#define BITMAP_MENU_RECENT 14

#define IDM_FIRSTCHILD		4242

HWND hModeLess = NULL;
HACCEL hAccel;
HMENU hMenuInitRecent, hMenuBmpRecent, hMenuInitWindow, hMenuBmpWindow;

char sRecentFiles[4][1024];
char sIniFileName[] = "Snomputz.ini";


// Für "zufällige Clipboardnamen ...
BYTE ClipBoardNumber = 0;


// Hauptroutine: Alle Init ...
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow )
{
	// Zur initialisierung ...
	CLIENTCREATESTRUCT clientcreate;
	char szBuf[1024];
	DWORD dwVersion;
	MSG msg ;
	WNDCLASS wndclass;
	HKEY hRegKey;
	extern CHAR strLastOpenPath[1024];
	extern CHAR strLastSavePath[1024];
	extern CHAR strDspDllPath[1024];
	extern CHAR strDspPrgPath[1024];
	//INITCOMMONCONTROLSEX	ie={sizeof(ie),ICC_WIN95_CLASSES};

	// file to load
	if (*pCmdLine != 0) {
		HWND hOther = FindWindow(szFrameClass, "SNOM-Putz");
		if(hOther) {
			HGLOBAL hdrop = GlobalAlloc(GHND, sizeof(DROPFILES) + lstrlen(pCmdLine) + 2);
			DROPFILES* df = (DROPFILES*)(GlobalLock(hdrop));
			df->pFiles = sizeof(DROPFILES);
			lstrcpy((char*)(df + 1), pCmdLine);
			GlobalUnlock(hdrop);
			if (!PostMessage(hOther, WM_DROPFILES, (WPARAM)hdrop, 0)) {
				GlobalFree(hdrop);
			}
			return 0;	// use it in other handle
		}
	}




#ifdef _TWO_DIGIT_EXPONENT
	_set_output_format( _TWO_DIGIT_EXPONENT );
#endif

	hInst = hInstance;
	if( hPrevInstance == NULL ) {
		// Fensterklasse für das Rahmenfenster
		wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wndclass.lpfnWndProc   = FrameWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 4;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( hInstance, "SNOMIcon" );
		wndclass.hCursor       = NULL;
		wndclass.hbrBackground = (HBRUSH)( COLOR_APPWORKSPACE+1 );
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = szFrameClass;
		RegisterClass( &wndclass );

		// Fensterklasse für Bitmap-Dokumentenfesnter
		wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
		wndclass.lpfnWndProc   = BmpWndProc ;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 4;
		wndclass.hInstance     = hInstance ;
		// Windows95 braucht Ikonen
		dwVersion = GetVersion();
#ifndef BIT32
		if( LOBYTE( LOWORD( dwVersion ) ) >= 3  &&   HIBYTE( LOWORD( dwVersion ) ) >= 95 )
#else
		if( LOBYTE( LOWORD( dwVersion ) ) > 3 )
#endif
		{
			wndclass.hIcon         = LoadIcon( hInstance, "SNOMIconBild" );
		}
		else {
			wndclass.hIcon         = NULL;
		}
		wndclass.hCursor       = NULL;
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH );
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = (LPSTR)szBmpClass;
		RegisterClass( &wndclass );

#ifdef BIT32
		// Fensterklasse für das Messfenster
		wndclass.style         = CS_OWNDC;
		wndclass.lpfnWndProc   = MessWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( hInstance, "SNOMIcon" );
		wndclass.hCursor       = NULL;
		wndclass.hbrBackground = (HBRUSH)( COLOR_APPWORKSPACE+1 );
		wndclass.lpszMenuName  = "MessMenu";
		wndclass.lpszClassName = szMessClass;
		RegisterClass( &wndclass );

		// Fensterklasse für das Messfenster
		wndclass.style         = CS_OWNDC;
		wndclass.lpfnWndProc   = MovieWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( hInstance, "SNOMIcon" );
		wndclass.hCursor       = NULL;
		wndclass.hbrBackground = (HBRUSH)( COLOR_APPWORKSPACE+1 );
		wndclass.lpszMenuName  = "MovieMenu";
		wndclass.lpszClassName = szMovieClass;
		RegisterClass( &wndclass );

		// Fensterklasse für selbstdefiniertes Objekt "Oszilloskop" definieren ..
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = OsziWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = NULL;
		wndclass.hCursor       = LoadCursor( NULL, IDC_CROSS );
		wndclass.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = (LPSTR)"Oszilloskop";
		RegisterClass( &wndclass );

		// Fensterklasse für Bitmap-Messfenster
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = MessBmpWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 4;
		wndclass.hInstance     = hInstance ;
		// Windows95 braucht Ikonen
		dwVersion = GetVersion();
		wndclass.hIcon         = LoadIcon( hInstance, "SNOMIconBild" );
		wndclass.hCursor       = NULL;
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH );
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = (LPSTR)"MessBmpShow";
		RegisterClass( &wndclass );
#endif

		// Drucker initialisieren
		PDlg.lStructSize = sizeof( PRINTDLG );
		PDlg.hwndOwner = NULL;
		PDlg.hDevMode = NULL;
		PDlg.hDevNames = NULL;
		PDlg.Flags = PD_RETURNDEFAULT;
		PrintDlg( &PDlg );

		CFont.lStructSize = sizeof( CHOOSEFONT );
		CFont.hwndOwner = NULL;
		CFont.lpLogFont = &lf;
		CFont.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_ANSIONLY; // | CF_NOSTYLESEL;

		// Vernünftig Initialisieren
		lf.lfWeight = FW_DONTCARE;
		lf.lfOutPrecision = OUT_TT_PRECIS;
		lf.lfClipPrecision = CLIP_STROKE_PRECIS;
		lf.lfQuality = PROOF_QUALITY;

		WM_FSEL_HELP = RegisterWindowMessage( HELPMSGSTRING );
#ifdef BIT32
#define TBS_TRANSPARENT					0x0800
		InitCommonControls();
#endif
	}

	// Name der Hilfedatei finden
	MakeHelpFileName( hInst, szHilfedatei, "SnomPutz" );

#if defined( USE_CTL3D )
	Ctl3dRegister( hInst );
	Ctl3dAutoSubclass( hInst );
#endif

	// Zwischenspeicher für schnellere Berechnungen des Redraws
	pColorConvert = pMalloc( 65536ul );

	// die zwei Handles für die Menüs
	hMenuInit  = LoadMenu( hInst, "MdiMenuInit" );
	hMenuBmp = LoadMenu( hInst, "MdiMenuBitmap" );

	hMenuInitWindow  = GetSubMenu( hMenuInit, INIT_MENU_POS );
	hMenuBmpWindow = GetSubMenu( hMenuBmp, BITMAP_MENU_POS );

	hMenuInitRecent  = GetSubMenu( hMenuInit, 0 );
	hMenuBmpRecent = GetSubMenu( hMenuBmp, 0 );

#if(WINVER >= 0x0400)
	if( RegCreateKey( HKEY_CURRENT_USER, "Software\\Snomputz", &hRegKey ) == ERROR_SUCCESS ) {
		// Zuletzt benutzt Dateien eintragen ...
		HKEY hSubkey;
		LONG len;
		int i, j;

		for( i = j = 0;  i < 4;  i++ ) {
			wsprintf( (LPSTR)szBuf, "Software\\Snomputz\\Recent\\File %i", i+1 );
			RegCreateKey( HKEY_CURRENT_USER, szBuf, &hSubkey );
			len = 1024;
			if(  RegQueryValue( hSubkey, NULL, sRecentFiles[i], &len )==ERROR_SUCCESS  ||  len>0  ) {
				wsprintf( (LPSTR)szBuf, "%d: %s", i+1, (LPSTR)sRecentFiles[i] );
				InsertMenu( hMenuInitRecent, ( INIT_MENU_RECENT+j ), MF_STRING|MF_BYPOSITION, IDM_RECENT1+j, szBuf );
				InsertMenu( hMenuBmpRecent, ( BITMAP_MENU_RECENT+j ), MF_STRING|MF_BYPOSITION, IDM_RECENT1+j, szBuf );
				j ++;
			}
			RegCloseKey( hSubkey );
		}
		InsertMenu( hMenuInitRecent, INIT_MENU_RECENT+4, MF_BYPOSITION|MF_SEPARATOR, 0xFFFFFFFFul, NULL );
		InsertMenu( hMenuBmpRecent, BITMAP_MENU_RECENT+4, MF_BYPOSITION|MF_SEPARATOR, 0xFFFFFFFFul, NULL );

		// Restliche Initwerte lesen ...
		RegCreateKey( HKEY_CURRENT_USER, "Software\\Snomputz\\Last save", &hSubkey );
		len = 1024;
		RegQueryValue( hSubkey, NULL, strLastSavePath, &len );
		RegCloseKey( hSubkey );
		RegCreateKey( HKEY_CURRENT_USER, "Software\\Snomputz\\Last open", &hSubkey );
		len = 1024;
		RegQueryValue( hSubkey, NULL, strLastOpenPath, &len );
		RegCloseKey( hSubkey );
		RegCreateKey( HKEY_CURRENT_USER, "Software\\Snomputz\\DSP Dll", &hSubkey );
		len = 1024;
		if(  RegQueryValue( hSubkey, NULL, strDspDllPath, &len )!=ERROR_SUCCESS  ||  len==0  ) {
			lstrcpy( strDspDllPath, "PC32DLL.DLL" );
		}
		RegCloseKey( hSubkey );
		RegCreateKey( HKEY_CURRENT_USER, "Software\\Snomputz\\DSP Out", &hSubkey );
		len = 1024;
		if(  RegQueryValue( hSubkey, NULL, strDspPrgPath, &len )!=ERROR_SUCCESS  ||  len==0  ) {
			lstrcpy( strDspPrgPath, "DSP\\SNOMPUTZ.OUT" );
		}
		RegCloseKey( hSubkey );
	}

	GetPrivateProfileString( "3D", "XY angle", "0.5", (LPSTR)szBuf, 80, sIniFileName );
	f3DXYWinkel = atof( szBuf );
	GetPrivateProfileString( "3D", "Z angle", "0.5", (LPSTR)szBuf, 80, sIniFileName );
	f3DZWinkel = atof( szBuf );
	GetPrivateProfileString( "3D", "Z factor", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	f3DZSkal = atof( szBuf );
	w3DZoom = GetPrivateProfileInt( "3D", "Draft", 2, sIniFileName );
	Show3D = GetPrivateProfileInt( "3D", "Show 3D", 0, sIniFileName );

	// Farben
	GetPrivateProfileString( "Colors", "Front", "0", (LPSTR)szBuf, 80, sIniFileName );
	cVorn = atol( szBuf );
	GetPrivateProfileString( "Colors", "Back", "16777215", (LPSTR)szBuf, 80, sIniFileName );
	cHinten = atol( szBuf );
	GetPrivateProfileString( "Colors", "Mark left", "33023", (LPSTR)szBuf, 80, sIniFileName );
	cMarkierungLinks = atol( szBuf );
	GetPrivateProfileString( "Colors", "Mark right", "16744192", (LPSTR)szBuf, 80, sIniFileName );
	cMarkierungRechts = atol( szBuf );

	// Zeichensatz
	GetPrivateProfileString( "Font", "Face", "MS Sans Serif", (LPSTR)lf.lfFaceName, LF_FACESIZE, sIniFileName );
	lf.lfHeight = GetPrivateProfileInt( "Font", "Size", -12, sIniFileName );
	if( abs( lf.lfHeight ) > 50000 ) {
		lf.lfHeight = -12;
	}

	// Skalierung
	GetPrivateProfileString( "Scale", "X", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalX = atof( szBuf );
	GetPrivateProfileString( "Scale", "Y", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalY = atof( szBuf );
	GetPrivateProfileString( "Scale", "Z", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalZ = atof( szBuf );
	GetPrivateProfileString( "Scale", "L", "1.0", (LPSTR)szBuf, 80, sIniFileName );

	// Profildaten: Was kopieren/speichern?
	wProfilMode = GetPrivateProfileInt( "Profil", "Mode", 7, sIniFileName );
	wProfilFlags = GetPrivateProfileInt( "Profil", "Flags", P_DIST|P_Z, sIniFileName );

	// Sonstiges
	fIntens = atof( szBuf );
	PlotsUnten = GetPrivateProfileInt( "Misc", "Plot below", FALSE, sIniFileName );

	// evt. Registry Updaten
	if( GetProfileString( "Extensions", "hdf", "", (LPSTR)szBuf, 80 ) == 0  ||  strstr( szBuf, "SNOMPUTZ" ) == NULL ) {
		lstrcpy((LPSTR)szBuf + GetModuleFileName(hInst, szBuf, 80), " ^.hdf");
		WriteProfileString("Extensions", "hdf", (LPSTR)szBuf);
		lstrcpy((LPSTR)szBuf + GetModuleFileName(hInst, szBuf, 80), " ^.xqd");
		WriteProfileString("Extensions", "xqd", (LPSTR)szBuf);
	}
#else
	// Zuletzt benutzt Dateien eintragen ...
	if( GetPrivateProfileString( "Recent Files", "File 1", "", szBuf, 80, sIniFileName ) ) {
		BYTE prf[] = "File 1";
		int i;

		for( i = 0;  i < 4;  i++ ) {
			prf[5] = '1'+i;

			GetPrivateProfileString( "Recent Files", prf, "", (LPSTR)sRecentFiles[i], 256, sIniFileName );
			wsprintf( (LPSTR)szBuf, "%d: %s", i+1, (LPSTR)sRecentFiles[i] );
			InsertMenu( hMenuInitRecent, ( INIT_MENU_RECENT+i ), MF_STRING|MF_BYPOSITION, IDM_RECENT1+i, szBuf );
			InsertMenu( hMenuBmpRecent, ( BITMAP_MENU_RECENT+i ), MF_STRING|MF_BYPOSITION, IDM_RECENT1+i, szBuf );
		}
		InsertMenu( hMenuInitRecent, INIT_MENU_RECENT+4, MF_BYPOSITION|MF_SEPARATOR, 0xFFFFFFFFul, NULL );
		InsertMenu( hMenuBmpRecent, BITMAP_MENU_RECENT+4, MF_BYPOSITION|MF_SEPARATOR, 0xFFFFFFFFul, NULL );
	}

	// Restliche Initwerte lesen ...
	//Pfade
	GetPrivateProfileString( "Paths", "Last save", "", (LPSTR)strLastSavePath, 256, sIniFileName );
	GetPrivateProfileString( "Paths", "Last open", "", (LPSTR)strLastOpenPath, 256, sIniFileName );
#ifdef BIT32
	GetPrivateProfileString( "Paths", "DSP Dll", "PC32DLL.DLL", (LPSTR)strDspDllPath, 256, sIniFileName );
	GetPrivateProfileString( "Paths", "DSP Out", "DSP\\SNOMPUTZ.OUT", (LPSTR)strDspPrgPath, 256, sIniFileName );
#endif

	GetPrivateProfileString( "3D", "XY angle", "0.5", (LPSTR)szBuf, 80, sIniFileName );
	f3DXYWinkel = atof( szBuf );
	GetPrivateProfileString( "3D", "Z angle", "0.5", (LPSTR)szBuf, 80, sIniFileName );
	f3DZWinkel = atof( szBuf );
	GetPrivateProfileString( "3D", "Z factor", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	f3DZSkal = atof( szBuf );
	w3DZoom = GetPrivateProfileInt( "3D", "Draft", 2, sIniFileName );
	Show3D = GetPrivateProfileInt( "3D", "Show 3D", 0, sIniFileName );

	// Farben
	GetPrivateProfileString( "Colors", "Front", "0", (LPSTR)szBuf, 80, sIniFileName );
	cVorn = atol( szBuf );
	GetPrivateProfileString( "Colors", "Back", "16777215", (LPSTR)szBuf, 80, sIniFileName );
	cHinten = atol( szBuf );
	GetPrivateProfileString( "Colors", "Mark left", "33023", (LPSTR)szBuf, 80, sIniFileName );
	cMarkierungLinks = atol( szBuf );
	GetPrivateProfileString( "Colors", "Mark right", "16744192", (LPSTR)szBuf, 80, sIniFileName );
	cMarkierungRechts = atol( szBuf );

	// Zeichensatz
	GetPrivateProfileString( "Font", "Face", "MS Sans Serif", (LPSTR)lf.lfFaceName, LF_FACESIZE, sIniFileName );
	lf.lfHeight = GetPrivateProfileInt( "Font", "Size", -12, sIniFileName );
	if( abs( lf.lfHeight ) > 50000 ) {
		lf.lfHeight = -12;
	}

	// Skalierung
	GetPrivateProfileString( "Scale", "X", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalX = atof( szBuf );
	GetPrivateProfileString( "Scale", "Y", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalY = atof( szBuf );
	GetPrivateProfileString( "Scale", "Z", "1.0", (LPSTR)szBuf, 80, sIniFileName );
	fPiezoSkalZ = atof( szBuf );
	GetPrivateProfileString( "Scale", "L", "1.0", (LPSTR)szBuf, 80, sIniFileName );

	// Profildaten: Was kopieren/speichern?
	wProfilMode = GetPrivateProfileInt( "Profil", "Mode", 7, sIniFileName );
	wProfilFlags = GetPrivateProfileInt( "Profil", "Flags", P_DIST|P_Z, sIniFileName );

	// Sonstiges
	fIntens = atof( szBuf );
	PlotsUnten = GetPrivateProfileInt( "Misc", "Plot below", FALSE, sIniFileName );

	// evt. Registry Updaten
	if( GetProfileString( "Extensions", "hdf", "", (LPSTR)szBuf, 80 ) == 0  ||  strstr( szBuf, "SNOMPUTZ" ) == NULL ) {
		lstrcpy( (LPSTR)szBuf+GetModuleFileName( hInst, szBuf, 80 ), " ^.hdf" );
		WriteProfileString( "Extensions", "hdf", (LPSTR)szBuf );
	}
#endif

	// Schlüssel gibt es schon?
	if( RegOpenKey( HKEY_CLASSES_ROOT, ".hdf", &hRegKey ) == ERROR_SUCCESS ) {
		LONG i;

		// Soll der aktuelle Schlüssel evt ersetzt werden?
		i = 80;
		RegQueryValue( HKEY_CLASSES_ROOT, ".hdf", (LPSTR)szBuf, &i );
		i = IDCANCEL;
		if( lstrcmpi( (LPSTR)szBuf, (LPSTR)"SPM.Image" ) != 0
		    &&  GetPrivateProfileString( "Misc", "CheckReg", "Yes", (LPSTR)szBuf, 80, sIniFileName ) > 0 ) {
			i = IDNO;
			if( szBuf[0] != 'N' ) {
				i = MessageBox( hwndFrame, GetStringRsc( I_SNOM_HDF ), STR_HDF_SNOM, MB_ICONQUESTION|MB_YESNOCANCEL );
			}
		}
		if( i == IDYES ) {
			RegCreateKey( HKEY_CLASSES_ROOT, ".hdf", &hRegKey );
			RegSetValue( HKEY_CLASSES_ROOT, ".hdf", REG_SZ, "SPM.Image", 9 );
			RegCreateKey( HKEY_CLASSES_ROOT, ".hdz", &hRegKey );
			RegSetValue( HKEY_CLASSES_ROOT, ".hdz", REG_SZ, "SPM.Image", 9 );
			RegCreateKey(HKEY_CLASSES_ROOT, ".xqd", &hRegKey);
			RegSetValue(HKEY_CLASSES_ROOT, ".xdq", REG_SZ, "SPM.Image", 9);
			RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image", &hRegKey );
			RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)"HDF SPM Data", lstrlen( "HDF SPM Data" ) );
			RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image\\DefaultIcon", &hRegKey );
			lstrcpy( (LPSTR)szBuf+GetModuleFileName( hInst, szBuf, 80 ), ",0" );
			RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)szBuf, lstrlen( szBuf ) );
			RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image\\DefaultIcon", &hRegKey );
			RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image\\shell\\open\\command", &hRegKey );
			lstrcpy( (LPSTR)szBuf+GetModuleFileName( hInst, szBuf, 80 ), " %1" );
			RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)szBuf, lstrlen( szBuf ) );
			WritePrivateProfileString( "Misc", "CheckReg", (LPSTR)"Yes", sIniFileName );
		}
		if( i == IDNO ) {
			WritePrivateProfileString( "Misc", "CheckReg", (LPSTR)"No", sIniFileName );
		}
	}
	else {
		RegCreateKey( HKEY_CLASSES_ROOT, ".hdz", &hRegKey );
		RegSetValue( HKEY_CLASSES_ROOT, ".hdz", REG_SZ, "SPM.Image", 9 );
		RegCreateKey(HKEY_CLASSES_ROOT, ".hdf", &hRegKey);
		RegSetValue(HKEY_CLASSES_ROOT, ".hdf", REG_SZ, "SPM.Image", 9);
		RegCreateKey(HKEY_CLASSES_ROOT, ".xqd", &hRegKey);
		RegSetValue(HKEY_CLASSES_ROOT, ".xdq", REG_SZ, "SPM.Image", 9);
		RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image", &hRegKey );
		RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)"HDF SPM Data", lstrlen( "HDF SPM Data" ) );
		RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image\\DefaultIcon", &hRegKey );
		lstrcpy( (LPSTR)szBuf+GetModuleFileName( hInst, szBuf, 80 ), ",0" );
		RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)szBuf, lstrlen( szBuf ) );
		RegCreateKey( HKEY_CLASSES_ROOT, "SPM.Image\\shell\\open\\command", &hRegKey );
		lstrcpy( (LPSTR)szBuf+GetModuleFileName( hInst, szBuf, 80 ), " %1" );
		RegSetValue( hRegKey, NULL, REG_SZ, (LPSTR)szBuf, lstrlen( szBuf ) );
	}

	// Tabelle mit Abkürzungsbefehlen laden
	hAccel = LoadAccelerators( hInst, "BitmapAccel" ) ;

	// Rahmenfenster erzeugen
#ifdef BIT32
	hwndFrame = CreateWindowEx( WS_EX_ACCEPTFILES, szFrameClass, "SNOM-Putz",
#else
	hwndFrame = CreateWindowEx( WS_EX_ACCEPTFILES, szFrameClass, "SNOM-Putz",
#endif
	                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	                            CW_USEDEFAULT, CW_USEDEFAULT,
	                            CW_USEDEFAULT, CW_USEDEFAULT,
	                            HWND_DESKTOP, hMenuInit, hInstance, NULL ) ;
	DragAcceptFiles( hwndFrame, TRUE );
#ifdef BIT32
	// Toolbar
#ifdef _WIN32
	hwndToolbar = CreateToolbarEx( hwndFrame, WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_ALTDRAG, ID_TOOLBAR, iMaxTool, hInst, TOOLBARBMP, pToolbarButtons, iMaxTool, 16, 16, 16, 16, sizeof( TBBUTTON ) );
#elif defined( __WIN32__ )
	hwndToolbar = CreateToolbar( hwndFrame, WS_CHILD|WS_BORDER|WS_VISIBLE|TBSTYLE_TOOLTIPS, ID_TOOLBAR, iMaxTool, hInst, TOOLBARBMP, pToolbarButtons, iMaxTool );
#endif
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_SAVE, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DRUCKEN, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_UNDO, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_REDO, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_DREHEN, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_ZEILENMITTEL, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MATHE, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_FALSCHFARBEN, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY_SCANLINE, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MASZE, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MEDIAN3, FALSE );
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MEDIAN5, FALSE );
	// StatusBar
	hwndInfo = CreateStatusWindow( WS_CHILD|WS_BORDER|WS_VISIBLE, NULL, hwndFrame, ID_STATUSBAR );
#endif
	// und die MDI-Fenster anlegen ...
	clientcreate.hWindowMenu  = hMenuInitWindow ;
	clientcreate.idFirstChild = IDM_FIRSTCHILD ;
	hwndClient = CreateWindow( "MDICLIENT", "Client",
	                           WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
	                           0, 0, 0, 0, hwndFrame, (HMENU)17, hInst,
	                           (LPSTR)&clientcreate );
	ShowWindow( hwndFrame, nCmdShow ) ;
	UpdateWindow( hwndFrame ) ;
	CreateDialogParam( hInst, "AboutDialog", hwndFrame, InfoDialog, EVAL_TIME );

#ifndef __WIN32__
	if( lstrlen( pCmdLine ) != 0 ) {
		// Datei laden
		LPBMPDATA pBmp;
		BOOLEAN	bFinish;                // Letzter String?
		UCHAR buffer[_MAX_PATH];
		LPSTR c, c_start = pCmdLine;

		do {
			LPSTR b = buffer;
			// seperates by either quotes or spaces
			BOOLEAN	parse_quotations = *c_start=='\"';
			c = c_start + 1;
			if (parse_quotations) {
				c_start++;
				while (*c != '\"' && *c != 0)
					*b++ = *c++;
				if(*c=='\"') {
					// skip last quote
					c++;
				}
			}
			else {
				while (*c != ' ' && *c != 0)
					*b++ = *c++;
			}
			*b = 0;
			bFinish = *c == 0;

			// Beim ersten Mal wird's schon nicht schiefgehen ...
			pBmp = (LPBMPDATA )pMalloc( sizeof( BMPDATA ) );
			pBmp->pSnom[0].fX = 1.0;
			pBmp->pSnom[0].fY = 1.0;
			pBmp->Links = pBmp->Rechts = NONE;
			pBmp->pSnom[0].Topo.fSkal = 1.0;
			pBmp->pSnom[0].Error.fSkal = 1.0;
			pBmp->pSnom[0].Lumi.fSkal = 1.0;
			pBmp->hwnd = hwndFrame; // Für Import-Dialog, damit es einen Vater gibt ...
			if( !ReadAll(buffer, pBmp ) ) {
				MemFree( pBmp );
			}
			else {
				MDICREATESTRUCT mdicreate;

				// Defaultskalierung anwählen
				mdicreate.szClass = (LPSTR)szBmpClass ;
				mdicreate.szTitle = buffer;
				mdicreate.hOwner  = hInst;
				mdicreate.x       = CW_USEDEFAULT;
				mdicreate.y       = CW_USEDEFAULT;
				mdicreate.cx      = CW_USEDEFAULT;
				mdicreate.cy      = CW_USEDEFAULT;
				mdicreate.style   = WS_CHILD|WS_BORDER;
				mdicreate.lParam  = (LONG)(LPBMPDATA)pBmp;
				UpdateRecent(buffer, pBmp );
				SendMessage( hwndClient, WM_MDICREATE, 0, (long) (LPMDICREATESTRUCT) &mdicreate ) ;
			}
			c_start = c++;
		} while( !bFinish );
	}
#endif

	// für MDI veränderte Ereignis-Warteschleife
	while( GetMessage( &msg, NULL, 0, 0 ) )	{
		if( hwndEdit != NULL  &&  msg.hwnd == hwndEdit ) {
			TranslateMessage( &msg ) ;
			DispatchMessage( &msg ) ;
		}
		else if( ( hModeLess == NULL  ||  !IsDialogMessage( hModeLess, &msg ) )  &&
//				 (hwndEdit==NULL  ||  !TranslateAccelerator(hwndEdit,NULL,&msg))  &&
		         !TranslateMDISysAccel( hwndClient, &msg ) &&
		         !TranslateAccelerator( hwndFrame, hAccel, &msg )
		         ) {
			TranslateMessage( &msg ) ;
			DispatchMessage( &msg ) ;
		}
	}

	// DSP DLL entladen (fehlt)

	// Aufräumen der momentan nicht eingesetzten Menüs
#if defined( USE_CTL3D )
	Ctl3dUnregister( hInst );
#endif
	DestroyMenu( hMenuBmp );

#if(WINVER >= 0x0400)
	// Einstellung in INI-Datei speichern
	// Zuerst: benutze Dateien
	{
		// Zuletzt benutzt Dateien eintragen ...
		BYTE str[256];
		int i;

		for( i = 0;  i < 4;  i++ ) {
			wsprintf( (LPSTR)szBuf, "Software\\Snomputz\\Recent\\File %i", i+1 );
			RegSetValue( HKEY_CURRENT_USER, szBuf, REG_SZ, sRecentFiles[i], 0 );
		}

		// Write paths
		RegSetValue( HKEY_CURRENT_USER, "Software\\Snomputz\\Last save", REG_SZ, strLastSavePath, 0 );
		RegSetValue( HKEY_CURRENT_USER, "Software\\Snomputz\\Last open", REG_SZ, strLastOpenPath, 0 );
		RegSetValue( HKEY_CURRENT_USER, "Software\\Snomputz\\DSP DLL", REG_SZ, strDspDllPath, 0 );
		RegSetValue( HKEY_CURRENT_USER, "Software\\Snomputz\\DSP Out", REG_SZ, strDspPrgPath, 0 );

		// Winkel
		gcvt( f3DXYWinkel, 4, str );
		WritePrivateProfileString( "3D", "XY angle", (LPSTR)str, sIniFileName );
		gcvt( f3DZWinkel, 4, str );
		WritePrivateProfileString( "3D", "Z angle", (LPSTR)str, sIniFileName );
		gcvt( f3DZSkal, 4, str );
		WritePrivateProfileString( "3D", "Z factor", (LPSTR)str, sIniFileName );
		wsprintf( str, "%d", w3DZoom );
		WritePrivateProfileString( "3D", "Draft", (LPSTR)str, sIniFileName );
		wsprintf( str, "%d", Show3D );
		WritePrivateProfileString( "3D", "Show 3D", (LPSTR)str, sIniFileName );

		// Farben
		wsprintf( str, "%ld", cVorn );
		WritePrivateProfileString( "Colors", "Front", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cHinten );
		WritePrivateProfileString( "Colors", "Back", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cMarkierungLinks );
		WritePrivateProfileString( "Colors", "Mark left", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cMarkierungRechts );
		WritePrivateProfileString( "Colors", "Mark right", (LPSTR)str, sIniFileName );

		// Profildaten: Was kopieren/speichern?
		wsprintf( str, "%ld", (ULONG)wProfilMode );
		WritePrivateProfileString( "Profil", "Mode", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", (ULONG)wProfilFlags );
		WritePrivateProfileString( "Profil", "Flags", (LPSTR)str, sIniFileName );

		// Zeichensatz
		WritePrivateProfileString( "Font", "Face", (LPSTR)lf.lfFaceName, sIniFileName );
		wsprintf( str, "%ld", lf.lfHeight );
		WritePrivateProfileString( "Font", "Size", (LPSTR)str, sIniFileName );

		// Skalierung
		gcvt( fPiezoSkalX, 10, str );
		WritePrivateProfileString( "Scale", "X", (LPSTR)str, sIniFileName );
		gcvt( fPiezoSkalY, 10, str );
		WritePrivateProfileString( "Scale", "Y", (LPSTR)str, sIniFileName );
		gcvt( fPiezoSkalZ, 10, str );
		WritePrivateProfileString( "Scale", "Z", (LPSTR)str, sIniFileName );
		gcvt( fIntens, 10, str );
		WritePrivateProfileString( "Scale", "L", (LPSTR)str, sIniFileName );

		// Sonstiges
		wsprintf( str, "%d", PlotsUnten );
		WritePrivateProfileString( "Misc", "Plot below", (LPSTR)str, sIniFileName );
	}
#else
	// Einstellung in INI-Datei speichern
	// Zuerst: benutze Dateien
	{
		BYTE str[256] = "File 1";
		int i;

#ifdef BIT32
		//Pfade
		WritePrivateProfileString( "Paths", "Last save", (LPSTR)strLastSavePath, sIniFileName );
		WritePrivateProfileString( "Paths", "Last open", (LPSTR)strLastOpenPath, sIniFileName );
		WritePrivateProfileString( "Paths", "DSP Dll", (LPSTR)strDspDllPath, sIniFileName );
		WritePrivateProfileString( "Paths", "DSP Out", (LPSTR)strDspPrgPath, sIniFileName );
#endif

		for( i = 0;  i < 4;  i++ ) {
			str[5] = '1'+i;
			WritePrivateProfileString( "Recent Files", (LPSTR)str, (LPSTR)sRecentFiles[i], sIniFileName );
		}

		// Winkel
		gcvt( f3DXYWinkel, 4, str );
		WritePrivateProfileString( "3D", "XY angle", (LPSTR)str, sIniFileName );
		gcvt( f3DZWinkel, 4, str );
		WritePrivateProfileString( "3D", "Z angle", (LPSTR)str, sIniFileName );
		gcvt( f3DZSkal, 4, str );
		WritePrivateProfileString( "3D", "Z factor", (LPSTR)str, sIniFileName );
		wsprintf( str, "%d", w3DZoom );
		WritePrivateProfileString( "3D", "Draft", (LPSTR)str, sIniFileName );
		wsprintf( str, "%d", Show3D );
		WritePrivateProfileString( "3D", "Show 3D", (LPSTR)str, sIniFileName );

		// Farben
		wsprintf( str, "%ld", cVorn );
		WritePrivateProfileString( "Colors", "Front", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cHinten );
		WritePrivateProfileString( "Colors", "Back", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cMarkierungLinks );
		WritePrivateProfileString( "Colors", "Mark left", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", cMarkierungRechts );
		WritePrivateProfileString( "Colors", "Mark right", (LPSTR)str, sIniFileName );

		// Profildaten: Was kopieren/speichern?
		wsprintf( str, "%ld", (ULONG)wProfilMode );
		WritePrivateProfileString( "Profil", "Mode", (LPSTR)str, sIniFileName );
		wsprintf( str, "%ld", (ULONG)wProfilFlags );
		WritePrivateProfileString( "Profil", "Flags", (LPSTR)str, sIniFileName );

		// Zeichensatz
		WritePrivateProfileString( "Font", "Face", (LPSTR)lf.lfFaceName, sIniFileName );
		wsprintf( str, "%ld", lf.lfHeight );
		WritePrivateProfileString( "Font", "Size", (LPSTR)str, sIniFileName );

		// Skalierung
		gcvt( fPiezoSkalX, 10, str );
		WritePrivateProfileString( "Scale", "X", (LPSTR)str, sIniFileName );
		gcvt( fPiezoSkalY, 10, str );
		WritePrivateProfileString( "Scale", "Y", (LPSTR)str, sIniFileName );
		gcvt( fPiezoSkalZ, 10, str );
		WritePrivateProfileString( "Scale", "Z", (LPSTR)str, sIniFileName );
		gcvt( fIntens, 10, str );
		WritePrivateProfileString( "Scale", "L", (LPSTR)str, sIniFileName );

		// Sonstiges
		wsprintf( str, "%d", PlotsUnten );
		WritePrivateProfileString( "Misc", "Plot below", (LPSTR)str, sIniFileName );
	}
#endif
	MemFree( pColorConvert );
	return ( msg.wParam );
}


// Gibt Rechenzeit an andere ab
VOID YieldApp( BOOL Wait )
{
//	extern HANDLE	hAccel;
	BOOL DoIt = FALSE;
	MSG mesg;

	// Damit die anderen auch noch etwas tun können
	if( Wait ) {
		DoIt = GetMessage( &mesg, NULL, 0, 0 );
	}
	else {
		DoIt = PeekMessage( &mesg, NULL, 0, 0, PM_REMOVE );
	}

	// Ist denn etwas zu tun?
	if( DoIt ) {
		if( ( hModeLess == NULL  ||  !IsDialogMessage( hModeLess, &mesg ) )  &&
		    !TranslateMDISysAccel( hwndClient, &mesg )  &&
		    !TranslateAccelerator( hwndFrame, hAccel, &mesg ) )	{
			TranslateMessage( &mesg ) ;
			DispatchMessage( &mesg ) ;
		}
	}
}


// 27.9.97


/***************************************************************************************/
// Ändert den Status der Menüeinträge von UNDO/REDO
void EnableUndoRedo( BOOL bUndo, BOOL bRedo )
{
	HMENU hMenu = GetMenu( hwndFrame );

#ifdef BIT32
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_UNDO, bUndo );
#endif
	if( bUndo ) {
		EnableMenuItem( hMenu, IDM_UNDO, MF_ENABLED );
	}
	else {
		EnableMenuItem( hMenu, IDM_UNDO, MF_DISABLED|MF_GRAYED );
	}
#ifdef BIT32
	SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_REDO, bRedo );
#endif
	if( bRedo ) {
		EnableMenuItem( hMenu, IDM_REDO, MF_ENABLED );
	}
	else {
		EnableMenuItem( hMenu, IDM_REDO, MF_DISABLED|MF_GRAYED );
	}
}


// 26.7,97


// Erneuert die Informationen der zuletzt geoeffneten Dateien
void UpdateRecent( LPSTR sFile, LPBMPDATA pBmp )
{
	char sBuf[256];
	int i, found = 3;

	// Schon einmal geladen?
	for( i = 0;  i < 3 && found == 3;  i++ ) {
		if( lstrcmpi( (LPSTR)sRecentFiles[i], sFile ) == 0 ) {
			found = i;
		}
	}

	// Neue Datei eintragen:
	// Alle kleiner "found" einen nach hinten
	if( found == -1 ) {
		found = 3;
	}
	for( i = found;  i > 0;  i-- ) {
		lstrcpy( (LPSTR)sRecentFiles[i], (LPSTR)sRecentFiles[i-1] );
	}
	lstrcpy( (LPSTR)sRecentFiles[0], sFile );

	// Und Menü updaten ...
	for( i = 0;  i < 4;  i++ ) {
		wsprintf( (LPSTR)sBuf, "%d: %s", i+1, (LPSTR)sRecentFiles[i] );
		ModifyMenu( hMenuInitRecent, IDM_RECENT1+i, MF_STRING|MF_BYCOMMAND, IDM_RECENT1+i, (LPSTR)sBuf );
		ModifyMenu( hMenuBmpRecent, IDM_RECENT1+i, MF_STRING|MF_BYCOMMAND, IDM_RECENT1+i, (LPSTR)sBuf );
	}
	if( hMenuBmp ) {
		// and finally update file display options
		pBmp->Links = pBmp->Rechts = 0;
		if( pBmp->pSnom[0].Topo.Typ == TOPO  &&  GetMenuState( hMenuBmp, IDM_SHOW_TOPO, MF_BYCOMMAND )&MF_CHECKED ) {
			pBmp->Links = TOPO;
		}
		if( pBmp->pSnom[0].Error.Typ == ERRO  &&  GetMenuState( hMenuBmp, IDM_SHOW_ERROR, MF_BYCOMMAND )&MF_CHECKED ) {
			if( pBmp->Links == 0 ) {
				pBmp->Links = ERRO;
			}
			else if( pBmp->Rechts == 0 ) {
				pBmp->Rechts = ERRO;
			}
		}
		if( pBmp->pSnom[0].Lumi.Typ == LUMI  &&  GetMenuState( hMenuBmp, IDM_SHOW_LUMI, MF_BYCOMMAND )&MF_CHECKED ) {
			if( pBmp->Links == 0 ) {
				pBmp->Links = ERRO;
			}
			else if( pBmp->Rechts == 0 ) {
				pBmp->Rechts = ERRO;
			}
		}
	}
}


// 4.7.98


// create/openh new window
LPBMPDATA OpenCreateWindow( LPCSTR datei )
{
	LPBMPDATA pBmp = (LPBMPDATA)pMalloc( sizeof( BMPDATA ) );
	MDICREATESTRUCT mdicreate;
	if( pBmp == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( NULL );
	}
	pBmp->pSnom[0].fX = 1.0;
	pBmp->pSnom[0].fY = 1.0;
	pBmp->Links = pBmp->Rechts = NONE;
	pBmp->pSnom[0].Topo.fSkal = 1.0;
	pBmp->pSnom[0].Error.fSkal = 1.0;
	pBmp->pSnom[0].Lumi.fSkal = 1.0;
	pBmp->hwnd = hwndFrame; // Für Import-Dialog, damit es einen Vater gibt ...
	if( !ReadAll( datei, pBmp ) ) {
		MemFree( pBmp );
		return ( NULL );
	}
	UpdateRecent( datei, pBmp );
	mdicreate.szClass = (LPSTR)szBmpClass ;
	mdicreate.szTitle = datei;
	mdicreate.hOwner  = hInst;
	mdicreate.x       = CW_USEDEFAULT;
	mdicreate.y       = CW_USEDEFAULT;
	mdicreate.cx      = CW_USEDEFAULT;
	mdicreate.cy      = CW_USEDEFAULT;
	mdicreate.style   = WS_CHILD|WS_BORDER;
	mdicreate.lParam  = (LONG)(LPBMPDATA)pBmp;
	// not needed to assing: hwndChild = (HWND)
	SendMessage( hwndClient, WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT) &mdicreate ) ;
	return ( pBmp );
}


// 24.1.2011


// String aus Resource
LPSTR GetStringRsc( UINT h )
{
	static char strBuffer[512];
	int iLen = LoadString( hInst, h, strBuffer, 512 );
	if( iLen > 0 ) {
		return ( strBuffer );
	}
	return ( NULL );
}


// Schreibt neue Informationen in die Statusleiste ...
void ClearStatusLine( void )
{
#ifdef BIT32
	SendMessage( hwndInfo, SB_SETTEXT, 0, (LPARAM)"" );
#endif
}


// Schreibt neue Informationen in die Statusleiste ...
void StatusLine( LPSTR str )
{
#ifdef BIT32
	SendMessage( hwndInfo, SB_SETTEXT, 0, (LPARAM)str );
#endif
}


// Schreibt neue Informationen aus der Stringressource h in die Statusleiste ...
void StatusLineRsc( UINT h )
{
	StatusLine( GetStringRsc( h ) );
}


// Fehlermeldung aus der Stringressource
void Fehler( LPSTR str )
{
	StatusLine( str );
#ifndef BIT32
	MessageBox( hwndFrame, str, NULL, MB_ICONSTOP|MB_OK );
#else
	MessageBoxEx( hwndFrame, str, NULL, MB_ICONSTOP|MB_OK, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ) );
#endif
	ClearStatusLine();
}


// 28.10.98


// Fehlermeldung aus der Stringressource
void FehlerRsc( UINT h )
{
	Fehler( GetStringRsc( h ) );
}


// 28.10.98


// Fehlermeldung aus der Stringressource
void Warnung( LPSTR str )
{
	if( hwndFrame == NULL ) {
		return;
	}
	StatusLine( str );
#ifndef BIT32
	MessageBox( hwndFrame, str, NULL, MB_ICONEXCLAMATION|MB_OK );
#else
	MessageBoxEx( hwndFrame, str, NULL, MB_ICONEXCLAMATION|MB_OK, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ) );
#endif
	ClearStatusLine();
}


// 28.10.98


// Fehlermeldung aus der Stringressource
void WarnungRsc( UINT h )
{
	Warnung( GetStringRsc( h ) );
}


// 28.10.98


// Mauszeiger variieren
HCURSOR	hOldMouse = NULL;

void WarteMaus( void )
{
	if( hOldMouse != NULL ) {
		SetCursor( hOldMouse );
	}
	hOldMouse = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
}


void NormalMaus( void )
{
	if( hOldMouse != NULL ) {
		SetCursor( hOldMouse );
	}
	hOldMouse = NULL;
}


// 30.10.98


/* Verwaltet das Init-Menü */
long WINAPI FrameWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
	LPBMPDATA pBmp;
	HWND hwndChild;
	MDICREATESTRUCT mdicreate;
	char datei[256];

	switch( message ) {
		case WM_CREATE:
			return ( 0 );

		case WM_INITMENU:
			//  Clipboardstatus evt. ändern
			if( IsClipboardFormatAvailable( CF_DIB ) ) {
				EnableMenuItem( hMenuInit, IDM_PASTE, MF_ENABLED );
				EnableMenuItem( hMenuBmp, IDM_PASTE, MF_ENABLED );
			}
			else {
				EnableMenuItem( hMenuInit, IDM_PASTE, MF_GRAYED|MF_DISABLED );
				EnableMenuItem( hMenuBmp, IDM_PASTE, MF_GRAYED|MF_DISABLED );
			}
			break;

		case WM_SIZE:
#ifdef BIT32
			if( hwnd == hwndFrame )	{
				int y, dh, cx, cy;
				RECT xywh;

				SendMessage( hwndToolbar, WM_SIZE, wParam, lParam );
				SendMessage( hwndInfo, WM_SIZE, wParam, lParam );
				cx = LOWORD( lParam );
				cy = HIWORD( lParam );
				GetWindowRect( hwndInfo, &xywh );
				dh = xywh.bottom-xywh.top;
				GetWindowRect( hwndToolbar, &xywh );
				y = xywh.bottom-xywh.top;
				dh += y;
				SetWindowPos( hwndClient, NULL, 0, y, cx, cy-dh, SWP_NOZORDER );
				return ( 0 ) ;
			}
#endif
			break;

#ifdef _WIN32
		case WM_NOTIFY:
		{
			LPNMTTDISPINFO pTTText = (LPNMTTDISPINFO)lParam;

			if( pTTText->hdr.code == TTN_GETDISPINFO ) {
				pTTText->lpszText = GetStringRsc( pTTText->hdr.idFrom );
			}
			break;
		}
#endif

		case WM_DROPFILES:
		{
			CHAR datei[1024];
			WORD i, max;

			max = DragQueryFile( (HDROP)wParam, (WPARAM)-1, datei, 1024 );
			for( i = 0;  i < max;  i++ ) {
				if( ( pBmp = (LPBMPDATA)pMalloc( sizeof( BMPDATA ) ) ) != NULL ) {
					DragQueryFile( (HDROP)wParam, i, datei, 1024 );
					CHAR *fname = datei;
					if (*fname == '\"') {
						// eventually remove quotation marks
						fname++;
						fname[lstrlen(fname)-1] = 0;
					}
					pBmp->pSnom[0].fX = 1.0;
					pBmp->pSnom[0].fY = 1.0;
					pBmp->Links = pBmp->Rechts = NONE;
					pBmp->pSnom[0].Topo.fSkal = 1.0;
					pBmp->pSnom[0].Error.fSkal = 1.0;
					pBmp->pSnom[0].Lumi.fSkal = 1.0;
					pBmp->hwnd = hwndFrame; // Für Import-Dialog, damit es einen Vater gibt ...
					if( !ReadAll( fname, pBmp ) ) {
						MemFree( pBmp );
						continue;
					}
					UpdateRecent( fname, pBmp );
					mdicreate.szClass = (LPSTR)szBmpClass ;
					mdicreate.szTitle = fname;
					mdicreate.hOwner  = hInst;
					mdicreate.x       = CW_USEDEFAULT;
					mdicreate.y       = CW_USEDEFAULT;
					mdicreate.cx      = CW_USEDEFAULT;
					mdicreate.cy      = CW_USEDEFAULT;
					mdicreate.style   = WS_CHILD|WS_BORDER;
					mdicreate.lParam  = (LONG)(LPBMPDATA)pBmp;
					SendMessage( hwndClient, WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT) &mdicreate ) ;
					ClearStatusLine();
				}
				else {
					FehlerRsc( E_MEMORY );
				}
			}

			DragFinish( (HDROP)wParam );
			break;
		}

		// Farbenmanagment
		case WM_PALETTECHANGED:
			if( hwnd == (HWND)wParam ) {
				break;
			}

		case WM_QUERYNEWPALETTE:
		{
			HDC hDC = GetDC( hwnd );
			HPALETTE hOldPal = SelectPalette( hDC, hCurrentPal, FALSE );
			UINT i = RealizePalette( hDC );

			if( i != 0 ) {
				RedrawAll( UPDATE );
			}
			SelectPalette( hDC, hOldPal, TRUE );
			RealizePalette( hDC );
			ReleaseDC( hwnd, hDC );
			return ( i );
		}

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) ) {
				case IDM_OPEN_FROM_STRING:
					OpenCreateWindow( (LPSTR)lParam );
					break;

				case IDM_RECENT1:
				case IDM_RECENT2:
				case IDM_RECENT3:
				case IDM_RECENT4:
					OpenCreateWindow( (LPSTR)sRecentFiles[wParam-IDM_RECENT1] );
					break;

				case IDM_OPEN:
					lstrcpy( datei, "*.*" );
					szFselHelpStr = STR_HFILE_OPEN;
					if( CMUFileOpen( hwnd, STR_OPEN_FILE, datei, STR_FILE_AFM ) ) {
						OpenCreateWindow( (LPSTR)datei );
					}
					break;

				case IDM_BROWSE:
				{
					lstrcpy( datei, sRecentFiles[0] );
					if( CMUGetFolderName( hwnd, STR_FOLDER, datei ) ) {
						LONG params[2];
						params[0] = (LONG)hwnd;
						params[1] = (LONG)datei;
						CreateDialogParam( hInst, "OverviewDlg", hwnd, HandleBrowseDialog, (LPARAM)params  );
					}
					break;
				}

#ifdef BIT32
				case IDM_MESS:
					CreateWindow( szMessClass, STR_SNOM_MESS, WS_OVERLAPPED|WS_CAPTION|WS_MINIMIZEBOX|WS_VISIBLE|WS_SYSMENU,
					              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, (LPVOID)hwndFrame );
					break;

				case IDM_MOVIE_MAKER:
					CreateWindow( szMovieClass, STR_SNOM_MESS, WS_OVERLAPPED|WS_CAPTION|WS_MINIMIZEBOX|WS_VISIBLE|WS_SYSMENU,
					              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, (LPVOID)hwndFrame );
					break;
#endif

				case IDM_PROFIL:
					// Was soll bei einer Höhenlinie gespeichert werden?
					DialogBoxParam( hInst, "ProfilDialog", hwnd, (DLGPROC)ProfilDialog, 0 );
					break;

				case IDM_3DANSICHT_DEFAULT:
					if( GetMenu( hwnd ) == hMenuInit ) {
						DialogBoxParam( hInst, "Ansicht3DDialog", hwnd, Ansicht3DDialog, 0L );
					}
					else {
						SendMessage( hwnd, WM_COMMAND, IDM_3DANSICHT, 0L );
					}
					break;

				// Vergrößen auf Fenstergröße ...
				case IDM_FIT_TO_WINDOW:
				case IDM_2ZU1:
				case IDM_1ZU1:
				case IDM_1ZU2:
				case IDM_1ZU3:
				case IDM_1ZU4:
					wZoomFaktor = wParam-IDM_FIT_TO_WINDOW;
					CheckMenuItem( hMenuBmp, IDM_FIT_TO_WINDOW, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, IDM_2ZU1, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, IDM_1ZU1, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, IDM_1ZU2, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, IDM_1ZU3, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, IDM_1ZU4, MF_UNCHECKED );
					CheckMenuItem( hMenuBmp, wParam, MF_CHECKED );
					RedrawAll( UPDATE|SLIDER );
					break;

				case IDM_CLOSE:       // aktives Dokumentenfenster schließen
					hwndChild = (HWND)SendMessage( hwndClient, WM_MDIGETACTIVE, 0, 0L );
					if( SendMessage( hwndChild, WM_QUERYENDSESSION, 0, 0L ) ) {
						SendMessage( hwndClient, WM_MDIDESTROY, (WPARAM)hwndChild, 0L ) ;
					}
					return ( 0 );

				case IDM_DRUCKEREINSTELLEN:
					PDlg.hwndOwner = hwndFrame;
					PDlg.Flags = PD_PRINTSETUP;
					PrintDlg( &PDlg );
					break;

				case IDM_EXIT:           // Programm beenden
					SendMessage( hwnd, WM_CLOSE, 0, 0L ) ;
					return ( 0 );

				// Anordnung der Dokumentenfenster
				case IDM_TILE:
					SendMessage( hwndClient, WM_MDITILE, 0, 0L ) ;
					return ( 0 );

				case IDM_CASCADE:
					SendMessage( hwndClient, WM_MDICASCADE, 0, 0L ) ;
					return ( 0 );

				case IDM_ARRANGE:
					SendMessage( hwndClient, WM_MDIICONARRANGE, 0, 0L ) ;
					return ( 0 );

				case IDM_CLOSEALL:     // Schließen aller Dokumentenfenster
					EnumChildWindows( hwndClient, (FARPROC)CloseEnumProc, 0L ) ;
					return ( 0 );

				case IDM_PROGRAMM:
				{
					hwndEdit = CreateWindowEx( WS_EX_ACCEPTFILES|WS_EX_APPWINDOW, "edit", STR_SNOM_PROG,
					                           WS_OVERLAPPEDWINDOW|WS_CAPTION|WS_VISIBLE|WS_SYSMENU|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_LEFT|ES_WANTRETURN,
					                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwndFrame, LoadMenu( hInst, "ProgrammMenu" ), hInst, NULL );
					pOldProgEditProc = (WNDPROC)SetWindowLong( hwndEdit, GWL_WNDPROC, (LONG)(WNDPROC)ProgWndProc );
					SetClassLong( hwndEdit, GCL_HICON, (LONG)LoadIcon( hInst, "SnomIcon" ) );
					break;
				}

				// Hilfemenue
				case IDM_ABOUT:
					DialogBoxParam( hInst, "AboutDialog", hwnd, InfoDialog, 0 );
					break;

				case IDM_HILFESUCHEN:
					WinHelp( hwndFrame, szHilfedatei, HELP_PARTIALKEY, 0L );
					break;

				case IDM_HILFEINHALT:
					WinHelp( hwndFrame, szHilfedatei, HELP_INDEX, 0L );
					break;

				// SNOM-Putz für Doofe ...
				case IDM_HILFETUTOR:
				{
					WORKMODE OldModus = modus;

					MessageBox( hwnd, STR_TUTOR, "", IDOK );
					pBmp = (LPBMPDATA)pMalloc( sizeof( BMPDATA ) );
					if( pBmp == NULL ) {
						StatusLineRsc( E_MEMORY );
						break;
					}
					pBmp->pSnom[0].fX = 1.0;
					pBmp->pSnom[0].fY = 1.0;
					pBmp->Links = pBmp->Rechts = NONE;
					pBmp->pSnom[0].Topo.fSkal = 1.0;
					pBmp->pSnom[0].Error.fSkal = 1.0;
					pBmp->pSnom[0].Lumi.fSkal = 1.0;
					lstrcpy( datei, "*.*" );
					szFselHelpStr = STR_HFILE_TUTOR;
					if( !CMUFileOpen( hwnd, STR_OPEN_FILE, datei, STR_FILE_AFM )
					    ||  !ReadAll( datei, pBmp ) ) {
						MemFree( pBmp );
						MessageBox( hwnd, STR_T_NO_OPEN, STR_HFILE_TUTOR, MB_OK );
						break;
					}
					UpdateRecent( datei, pBmp );
					wZoomFaktor = 0;
					mdicreate.szClass = (LPSTR)szBmpClass ;
					mdicreate.szTitle = datei;
					mdicreate.hOwner  = hInst;
					mdicreate.x       = CW_USEDEFAULT;
					mdicreate.y       = CW_USEDEFAULT;
					mdicreate.cx      = CW_USEDEFAULT;
					mdicreate.cy      = CW_USEDEFAULT;
					mdicreate.style   = WS_CHILD|WS_BORDER|WS_MAXIMIZE;
					mdicreate.lParam  = (LONG)(LPBMPDATA)pBmp;
					hwndChild = (HWND) SendMessage( hwndClient, WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT) &mdicreate );

					// Evt. Fehlerdaten rausschmeißen
					if( pBmp->pSnom[0].Error.Typ != NONE  &&  MessageBox( hwndClient, STR_T_ERROR, STR_HFILE_TUTOR, MB_YESNO ) == IDYES ) {
						pBmp->Rechts = NONE;
						RecalcCache( pBmp, TRUE, TRUE );
						InvalidateRect( hwndChild, NULL, FALSE );
					}

					// Evt. Rechtsdrehen
					modus = TOPO|ERRO|LUMI;
					if( MessageBox( hwndClient, STR_T_TURN, STR_HFILE_TUTOR, MB_YESNO ) == IDYES ) {
						SendMessage( hwndChild, WM_COMMAND, IDM_DREHEN, 0L );
					}

					// Evt. Mitteln
					if( !DialogBoxParam( hInst, "TutorDialogNeigung", hwnd, TutorDialog, (LPARAM)hwndChild ) ) {
						modus = OldModus;
						break;
					}

					// Farben einstellen
					if( !DialogBoxParam( hInst, "FarbenDialog", hwnd, (DLGPROC)FarbenDialog, (LPARAM)hwndChild ) ) {
						modus = OldModus;
						break;
					}

					// 3D Parameter einstellen
					if( pBmp->pSnom[pBmp->iAktuell].Topo.bPseudo3D  ||  pBmp->pSnom[pBmp->iAktuell].Error.bPseudo3D  ||  pBmp->pSnom[pBmp->iAktuell].Lumi.bPseudo3D ) {
						DialogBoxParam( hInst, "Ansicht3DDialog", hwnd, Ansicht3DDialog, (LPARAM)hwndChild );
					}

					modus = OldModus;
					break;
				}

				default:            // Weitergabe an aktives Dokumentenf.
					hwndChild = (HWND)SendMessage( hwndClient, WM_MDIGETACTIVE, 0, 0L );
					if( IsWindow( hwndChild ) ) {
						SendMessage( hwndChild, WM_COMMAND, wParam, lParam ) ;
					}
					break;        // und danach an DefFrameProc
			}
			break;
		}

		case WM_QUERYENDSESSION:
		case WM_CLOSE:                     // alle Dokumentenfenster schließen
			SendMessage( hwnd, WM_COMMAND, IDM_CLOSEALL, 0L ) ;
			if( NULL != GetWindow( hwndClient, GW_CHILD ) ) {
				return ( 0 );
			}
			break ; // -> weiter mit DefFrameProc

		case WM_DESTROY:
		{
			HMENU hCurrentMenu = GetMenu( hwnd );
			extern HWND hModeLess;
			DragAcceptFiles( hwndFrame, FALSE );
			if( hMenuInit != hCurrentMenu ) {
				DestroyMenu( hMenuInit );
			}
			if( hMenuBmp != hCurrentMenu ) {
				DestroyMenu( hMenuBmp );
			}
			PostQuitMessage( 0 ) ;
			return ( 0 );
		}

		default:
			if( message == WM_FSEL_HELP ) {
				WinHelp( hwndFrame, szHilfedatei, HELP_KEY, (DWORD)(LPSTR)szFselHelpStr );
			}
	}
	// Weitergabe an DefFrameProc (ersetzt DefWindowProc)
	return ( DefFrameProc( hwnd, hwndClient, message, wParam, lParam ) );
}


BOOL WINAPI CloseEnumProc( HWND hwnd, LONG lParam )
{
	if( GetWindow( hwnd, GW_OWNER ) ) {       // Client-Fenster?
		return ( 1 );              // ja - nicht abbauen!
	}
	// Botschaft an das Client-Fenster: Dokument zurück auf Originalgröße
	SendMessage( GetParent( hwnd ), WM_MDIRESTORE, (WPARAM)hwnd, 0L ) ;

	// Botschaft an das Dokumentenfenster: Schließen OK?
	if( !SendMessage( hwnd, WM_QUERYENDSESSION, 0, 0L ) ) {
		return ( 0 );
	}

	// OK. Botschaft an das Client-Fenster: Dokumentenfenster abbauen
	SendMessage( GetParent( hwnd ), WM_MDIDESTROY, (WPARAM)hwnd, 0L ) ;
	return ( 1 );
}



// Globale Variablen
//
// werden initialisiert, wenn "#define __main" im Header
//

#include "snom-def.h"
#include "snomputz.h"
#include "snom-typ.h"

#ifndef __SNOM_VAR
#define __SNOM_VAR

#ifdef __main

// COMMON-Dialog-Variablen
PRINTDLG PDlg;
CHOOSEFONT CFont;
LOGFONT	lf;

// Farbmanagment
HWND			hCurrentPalHwnd=NULL;
HPALETTE	hCurrentPal=NULL;

// Windowkram
HANDLE hInst;
HMENU  hMenuInit, hMenuBmp;
HWND  hwndFrame, hwndClient, hwndInfo;
HWND	hwndToolbar;
char  szFrameClass [] = "SnomFrame" ;
char  szBmpClass [] = "SnomBitmapChild" ;
char  szMessClass [] = "SnomBitmapMess" ;
char  szMovieClass [] = "SnomBitmapMovie" ;
char  szMessSeriellClass [] = "SnomMessSeriell";
char  szHilfedatei[256];
LPSTR	szFselHelpStr;

// Hilfedatei wurde vom Fileselector aus aufgerufen
UINT	 WM_FSEL_HELP;

// Darstellungsparameter
BOOL	Show3D = FALSE, PlotsUnten=FALSE;
double	fPiezoSkalX, fPiezoSkalY, fPiezoSkalZ, fIntens;
WORD	wZoomFaktor=1;

// Startparameter für 3D-Ansicht: 45°, 45° 1:1, Draft-Modus
float	f3DXYWinkel, f3DZWinkel, f3DZSkal;
WORD	w3DZoom;	// 2=Draft
// Startfarben: Schwarz Weiß Orange
COLORREF	cVorn, cHinten, cMarkierungLinks, cMarkierungRechts;

// Was bearbeiten?
WORKMODE	modus=LUMI|ERRO|TOPO;
WORKMODE	show=LUMI|ERRO|TOPO;

// Berechnungs-Cache
LPUCHAR	pColorConvert=NULL;

// Alles für die Höhenlinien ...
WORKMODE		wProfilMode=TOPO|ERRO|LUMI;
PROFILFLAGS	wProfilFlags=P_DIST|P_X|P_Y|P_Z|P_AUTOKORRELATION;
PROFILFLAGS	wProfilShowFlags=P_DIST|P_Z;

// Für "Unbenannt"-Bilder
WORD	uUnbenannt=0;

#else																																																		 // Aus SNOM-WRK.C

// COMMON-Dialog-Variablen
extern PRINTDLG PDlg;
extern CHOOSEFONT CFont;
extern LOGFONT	lf;

// Handle des Fensters mit der aktuellen Farbpalette
extern HWND			hCurrentPalHwnd;
extern HPALETTE	hCurrentPal;

// Zur Zeichensatzauswahl
extern LOGFONT	lf;

// Hilfedatei wurde vom Fileselector aus aufgerufen
extern UINT	 WM_FSEL_HELP;

// Window-Stuff
extern HANDLE hInst;
extern HMENU  hMenuInit, hMenuBmp;
extern HWND  hwndFrame, hwndClient, hwndInfo;
extern HWND	hwndToolbar;
extern char  szFrameClass [];
extern char  szBmpClass[];
extern char  szHilfedatei[256];
extern LPSTR	szFselHelpStr;

// Zeichenparamenter
extern double	fPiezoSkalX, fPiezoSkalY, fPiezoSkalZ, fIntens;
//extern double	fSkalX, fSkalY, fSkalZ, fIntens;
extern WORD		wZoomFaktor;

// Parameter für 3D-Ansicht: 45°, 45° 1:1
extern WORD		w3DZoom;
extern BOOL		Show3D, PlotsUnten;
extern float	f3DXYWinkel, f3DZWinkel, f3DZSkal;
extern COLORREF	cVorn, cHinten, cMarkierungLinks, cMarkierungRechts;

// Was bearbeiten?
extern WORKMODE	modus, show;

// Berechnungs-Cache
extern LPUCHAR pColorConvert;

// Alles für die Höhenlinien ...
extern WORKMODE		wProfilMode;
extern PROFILFLAGS	wProfilFlags;
extern PROFILFLAGS	wProfilShowFlags;

// Für "Unbenannt"-Bilder
extern WORD	uUnbenannt;


#endif
#endif

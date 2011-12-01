// Typen werden definiert
#ifndef __SNOM_TYP
#define __SNOM_TYP

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include "psi-hdf.h"
#include "snom-def.h"

/********************************	Typdefinition ********************************/

#include "myportab.h"

#define MAX_SCANLINE 4

typedef enum { NONE=0, IDM_NM, IDM_AA, IDM_MKM } EINHEIT;
typedef enum { DIGITAL=1, PSI, HDF, SNOMPUTZ, SM2, ECS, OMICRON, HITACHI, WSxM, SEIKO } EXTRADATA;
typedef enum { P_DIST=1, P_X=2, P_Y=4, P_Z=8, P_AUTOKORRELATION=16, P_PSD=32, P_HISTOGRAMM=64, P_INT_HIST=128 } PROFILFLAGS;

// Falls enum 2 Byte ...
#if 0
typedef enum { TOPO=1, ERRO=2, LUMI=4, TRACK_ALL=256 } WORKMODE;
#else	// ansonsten eben so ...
#define TOPO (1)
#define ERRO (2)
#define LUMI (4)
#define TRACK_ALL (256)
typedef UWORD WORKMODE;
#endif

// Sonst stimmt die Größe der Strukturen nicht mehr ...
#define FLAG UWORD




/********************************	Strukturen ********************************/

#pragma pack(2)	// Wortweise packen

// Struktur, die (fast) alle relevanten Daten zur Darstellung hat
typedef struct
{
	LPUWORD		puDaten;					// Daten, Wert <=256 Pointer auf letzte geänderte Daten
	UWORD			uMaxDaten;				// größter Datenwert (Kleinster==0)
	WORKMODE	Typ;							// Datentyp, NONE=ungenutzt
	CHAR			strTitel[32];
	double		fSkal;						// Skalierungsfaktor
	UWORD			uKontur;
	UWORD			uKonturToleranz;	// Für "Höhenlinien"
	UWORD			uModulo;
	FLAG 	bNoLUT;			// Use Look up table
	FLAG 	bPseudo3D;
	FLAG 	bKonturen;
	FLAG 	bModuloKonturen;
	COLORREF	Farben[3];
	float			fStart;
	float			fEnde;							// Zu zeigender Farbbereich in Prozent
	float			fXYWinkel3D, fZWinkel3D, fZSkal3D;			// Darstellungsparameter
	FLAG			bSpecialZUnit;			// Nicht umskalieren!
	FLAG			bShowNoZ;						// Z-Achse nicht zeigen
	CHAR			strZUnit[16];				// Z Einheitsbeschriftung
	UWORD			iNumColors;
	COLORREF	LUT[256];
} BILD;


#define EXPONENTIAL_PID	1
#define PIEZO_IS_INVERT 2

typedef struct
{
	float	fPid, fInt, fDiff;	// PID-Parameter
	long	lFreq, lZyklus;			// Geschwindigkeit
	long		iSoll;							// Sollwert
	double	fW, fH;						// Größe des Scan (in nm)
	double	fX, fY;						// Offset (in nm)
	double	fRot;							// Drehwinkel (zur Zeit imm Null)
	short		iWPts, iHPts, iVersatz;	// Punkte in X-Richtung, Y-Rtg, Versatz
	short		iModus, iRichtung;	// Modus (TOOP,ERRO,LUMI) und Richtung (1=Vor, 2=Zurück, 3=Beide)
	double	fPiezoX, fPiezoY, fPiezoZ;
	short		bFlags;						// exponentiell, invert, ...
} SNOMPUTZ_SCANDATA;

// Struktur, die bei jeder Änderung neu angelegt wird
typedef struct
{
	LONG		w, h;				 					// Breite und Höhe der Bitmap
	double	fX, fY, fXOff, fYOff;	// Skalierung
	BILD		Topo, Error, Lumi;
} SNOMDATA;


typedef struct {
	UWORD x;
	UWORD y;
	UWORD hgt;
	UWORD radius;
} XYZ_COORD;


typedef struct {
	UWORD x;
	UWORD y;
} XY_COORD;


// Struktur, die einem Fenster zugeordnet wird, und in der ALLES zur Darstellung versammelt ist
typedef struct
{
	char		szName[256];
	UWORD		wKurzname;	// Zeigt auf den Beginn des Dateinamens (ohne Pfad)
	HWND		hwnd;				// Fensterhandle (fuer Fehlermeldungen etc.)

	SNOMDATA	pSnom[MAX_SNOM];

	int			iAktuell;		// Aktueller Datensatz
	int			iSaved;			// Wurde zuletzt als Nummer iSaved gespeichert
	int			iMax;				// Maximale Anzahl an Datensätzen
	BOOLEAN		IsDirty;		// Cache muss neu berechnet werden

	WORD		wMaskeW;		// Weite der Maske in Bytes ...
	LPBYTE	pMaske;			// NULL oder Pointer auf die aktuelle Maske
											// gesetzte Bits in der Maske werden bei Mittelung oder Glättung nicht berücktsichtigt (außer bei Max/Min)

	LPBITMAPINFO	pDib;	// Die letzten Farben
	LPUCHAR		pCacheBits;	// Die letzte Darstellung gecacht

	WORKMODE  Links, Rechts;						// Darstellung links (bzw. rechts)
	RECT			rectLinks, rectRechts;						// Ausmaße der BILDER in den Rechtecken
	
	BOOL		bIsScanLine;	// TRUE, wenn tatsächlich eine Scanline existiert
	int			lMaxScan;
	double	fScanBorder[4];
	BOOL		bPlotUnten;		// FALSE, wenn Scanlinie rechts daneben, sonst darunter
	RECT		rectScan[MAX_SCANLINE];	// Scanlinekoordinaten (Es gibt maximal vier Scanlines gleichzeitig!)

	RECT		rectFenster;	// Ausmaße des gesamten Fensters (Bilder+Scanlines+Text)
	RECT		rectPlot;			// Plotkoordinaten (in BITMAPKOORDINATEN!)

	BYTE		bCountDots;	// 1, if countin, 2 if removing
	UWORD		dot_number;
	UWORD		dot_radius;
	UWORD		dot_mean_level;			// zero level
	UWORD		dot_quantisation;	// how many height level are one hist level
	XYZ_COORD	*dot_histogramm;	// height data for each dot
	UWORD		dot_histogramm_count;	// height data for each dot

	BOOL		bMarkScanLine;	// TRUE, wenn gerade Scanline markiert wird
	LPDOUBLE pScanLine;		// Pointer auf Scanlinie (7-Spalten!)

	BOOL		bAddMaske;		// TRUE, wenn gerade Maske markiert wird
	RECT		rectMaske;
	BOOL		bMouseMoveMode;	// TRUE: Mauszeiger ist ein Kreuz

	BYTE		pKommentar[1024];	// Sollte reichen ...

	WORD		iTag, iMonat, iJahr;	// Das Datum der Messung ...
	WORD		iStunde, iMinute, iSekunde;

	PSI_HEADER	pPsi;		// PSI-HDF-Header
	EXTRADATA	Typ;			// Typ der importiereten Datei und der Extradaten (Topografie hat Vorrang!)
	LPUCHAR		pExtra;		// Entweder Pointer auf Ascii-Header oder HDF-Header
	LONG 		lExtraLen;	// Länge der Extradaten auf die pExtra zeigt
	double		fZoom;		// Zoomfaktor (ist nur bei Bild=Fenstergroß wichtig)
}
BMPDATA;


/* Und die Pointer */
#ifndef	BIT32
typedef BILD huge *			LPBILD;
typedef SNOMDATA huge * LPSNOMDATA;
typedef BMPDATA huge*		LPBMPDATA;
#else
typedef BILD *		LPBILD;
typedef SNOMDATA *LPSNOMDATA;
typedef BMPDATA *	LPBMPDATA;
#endif


// Für die Ermittelung einer Scanline
typedef struct
{
	LPDOUBLE  pPtr;
	LONG			lPos;
	double		dStartDist;
	LPBMPDATA pBmp;
	double		TopoMax, LumiMax, ErrorMax;
	double		TopoMin, LumiMin, ErrorMin;
} DDADataStruct;


#endif


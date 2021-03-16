/* Includes für Programmierung und History */

#ifndef SNOM_PROGRAMM
extern WNDPROC	pOldProgEditProc;
extern HWND			hwndEdit;
#else

/********************************************************************************************/
// Alle Typen für Programmierung

#define UNDEF_CONDITION	2

typedef enum { PROG_ABORT=0, PROG_EOF=2, PROG_EXECUTE=1 } PROGRAMMSTATUS;
typedef enum { TYP_HANDLE=1, TYP_BILD, TYP_ZAHL, TYP_STRING, TYP_PWORD, TYP_PLONG, TYP_PFLOAT, TYP_PDOUBLE } PROG_VARTYP;

// Alles Über Variablen
typedef struct
{
	BYTE					Name[256];	// Name der Variablen
	union
	{
		struct 
		{
			LPBMPDATA	pBmp;		// Pointer auf Werte
			WORKMODE	bModus;	// für Handels i.A. TOPO|ERRO|LUMI
		} Bmp;
		double	fWert;
		LPSTR		str;
	};
	PROG_VARTYP	Typ;	// Was ist es denn, Handle, Zahl, +++
} VARIABLEN;

// "Stack-Info"
typedef struct
{
	LONG				lZeilenNummer;
	struct
	{
		int	EXECUTE				:1;	// Nächste Zeile wird ausgeführt
		int	CONDITION			:2;	// Letzte Bedingung war wahr, falsch, undefiniert
		int BEGIN_BLOCK		:1;	// nächste "{" wird ignoriert
		int CONT_ON_RETURN:1; // bei "}" wird die neue Zeilennummer die aktuelle + 1
	};
} PROGSTACK;

#define PROG_MAX_HANDLES	10
#define PROG_MAX_BILDER		20
#define PROG_MAX_ZAHLEN		64
#define PROG_MAX_STRINGS	32

// Und schließlich alles über das Programm
typedef struct
{
	VARIABLEN		Handles[PROG_MAX_HANDLES];
	VARIABLEN		Bilder[PROG_MAX_BILDER];
	VARIABLEN		Zahlen[PROG_MAX_ZAHLEN];
	VARIABLEN		Strings[PROG_MAX_STRINGS];
	PROGSTACK		Stack[256];
	int					iStackPtr;
} PROGPTR;

typedef PROGPTR * LPPROGPTR;


/********************************************************************************************/

#endif

// Verwaltung Programmierfenster 
LRESULT WINAPI	ProgWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam );
// 12.1.98

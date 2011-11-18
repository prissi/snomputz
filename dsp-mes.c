/*************************************************************************/

#ifdef BIT32
#error "Nur 32Bit!"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>

// Nur zu Testzwecken!
//#define TEST_WITHOUT_DSP 1
//#include <math.h>

#define IMPORT_AS_DLL
#include "dsp\scantyp.h"
#include "pc32dll\target.h"
#include "snomputz.h"

#include "myportab.h"
#include "snom-typ.h"
#include "snom-var.h"
#include "snomlang.h"

#include "snom-dat.h"
#include "snom-dsp.h"
#include "snom-mat.h"
#include "snom-win.h"
#include "snom-mem.h"


/*************************************************************************/
// lokale Variablen

#define MESS_MESS_TIMER	14

#define WM_SCANLINE_READY	(WM_USER+27)
#define WM_SCANSPS_READY	(WM_USER+28)

// Private Variablen
#define ID_PID_STATUSBAR 19

HWND	hwndOsziCtl;
HWND	hwndMessBar, hwndMessInfo;
extern HWND	hModeLess;

UINT	iOn;
static BOOL	bForeverScan=FALSE;

int maxZScannerBetrag=32000;
/*********************************************************************************************
**** Für Oszi-Kontrol
*********************************************************************************************/


#define OSZI_MAX	1024
char	OsziStr[4][32];
int	OsziNum[4]={0,0,0,0};
long	OsziDaten[4][OSZI_MAX];
long	OsziMin[4]={32767,32767,32767,32767};
long	OsziMax[4]={-32768,-32768,-32768,-32768};
long  invert=0,iError1=0,PID_ermitteln=0;
COLORREF OsziColor[4]={ RGB(255,0,0), RGB(0,255,0), RGB(128,128,255), RGB(0,0,255) };

void	RedrawOszi( HWND hDlg )
{
	HWND	hAnsicht;

	hAnsicht = GetDlgItem( hDlg, MESS_OSZI );
	InvalidateRect( hAnsicht, NULL, FALSE );
//	UpdateWindow( hAnsicht );
}
// 2.9.97


// Oszi-"Control"
LONG WINAPI OsziWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HPEN	hOldPen;
	RECT	rect;
	static	iLastNr;
	float TempY;

	switch( message )
	{
		case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			HDC		hDC;
			HRGN	hRgn;
			int		i, j, x, y;  // Offsets zum Zentrieren

			GetClientRect( hwnd, &rect );
			hDC = BeginPaint( hwnd, &ps );
			hRgn = CreateRectRgnIndirect( &rect );
			SelectObject( hDC, GetStockObject( BLACK_BRUSH ) );
			SelectObject( hDC, hRgn );
			PaintRgn( hDC, hRgn );

			// Hier fehlt die Beschriftung ...

			SelectObject( hDC, GetStockObject( WHITE_PEN ) );
			y = rect.top+rect.bottom/5;
			for(  i=0;  i<4;  i++  )
			{
				MoveToEx( hDC, rect.left, y, NULL );
				LineTo( hDC, rect.left+rect.right, y );
				y += rect.bottom/5;
			}
			x = rect.left+rect.right/10;
			for(  i=0;  i<9;  i++  )
			{
				MoveToEx( hDC, x, rect.top, NULL );
				LineTo( hDC, x, rect.top+rect.bottom );
				x += rect.right/10;
			}

			hOldPen = SelectObject( hDC, CreatePen (PS_SOLID, 1, OsziColor[3] ) );
			SetTextAlign( hDC, TA_LEFT|TA_TOP );
#if 0
			//UP changed
			for(  j=3;  j>0;  j--  )
			{
				OsziMin[j]=OsziDaten[j][0];
				OsziMax[j]=OsziDaten[j][0];
				for(  i=1;  i<OsziNum[j]  &&  i<OSZI_MAX;  i++  )
				{
					if(OsziDaten[j][i]<OsziMin[j]) {
						OsziMin[j]=OsziDaten[j][i];
					}
					if(OsziDaten[j][i]>OsziMax[j]) {
						OsziMax[j]=OsziDaten[j][i];
					}
				}
			}
#endif
			for(  j=3;  j>=0;  j--  )
			{
				for(  i=0;  i<OsziNum[j]  &&  i<OSZI_MAX;  i++  )
				{
					x = (int)((LONG)i*(LONG)rect.right/(LONG)OsziNum[j]);
					y = (int)(((LONG)OsziDaten[j][i]-OsziMin[j])*(LONG)rect.bottom/((LONG)OsziMax[j]-(LONG)OsziMin[j]+1l));
					if(  i!=0  )
						LineTo( hDC, rect.left+x, rect.top+rect.bottom-y );
					else
						MoveToEx( hDC, rect.left+x, rect.top+rect.bottom-y, NULL );
				}
				if(  i>iLastNr  )
					iLastNr = i;
				if(  j>0  )
					DeleteObject( SelectObject( hDC, CreatePen (PS_SOLID, 1, OsziColor[j-1] ) ) );
				// BEschriftung
				SetTextColor( hDC, OsziColor[j] );
				TextOut( hDC, rect.left+10, rect.top+3+j*15, OsziStr[j], lstrlen(OsziStr[j]) );
			}
			DeleteObject( SelectObject( hDC, hOldPen ) );

			DeleteRgn( hRgn );
			EndPaint( hwnd, &ps );
			return 0;
		}

		case WM_MOUSEMOVE:
			if(  !(wParam&MK_LBUTTON)  )
				break;
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		{
			GetClientRect( hwnd, &rect );
			SendMessage( GetParent(hwnd), WM_COMMAND, MESS_OSZI, (LONG)((LOWORD(lParam)*1000l)/rect.right) );
		}
		return 0;
	}
	return DefWindowProc( hwnd, message, wParam, lParam );
} // 6.9.97




/*********************************************************************************************
**** Für DSP Kommunikation
*********************************************************************************************/


typedef struct
{
	long	lFreq;
	long	lSoll;
	long	lProp, lInt, lDiff;
	long	bFlags;
	long	lVoltage;
} PIDPAR;

SCANPAR	Scan={ TOPO|ERRO, 100l, 256, 256, 20000u, 20000u, -10000, -10000, 0, LINKS2RECHTS|RECHTS2LINKS, 0 };
#define NOT_INIT -222222l
GOPAR		Go={ 100l, 100l, 0, 0, 0, 0 };
PIDPAR	PID={ 25000, NOT_INIT, 10000, 5000, 0, 0, 19660 /* +3V */ };


CARDINFO HUGE	*pDSPInfo;
HINSTANCE		hDSP_DLL=NULL;

// Pfad der Dll
CHAR				strDspDllPath[256];
// Pfad des DSP-Programms
CHAR				strDspPrgPath[256];


// Funktionsvariablen definieren
int (FAR WINAPI * ReadMailbox) ( long a, long b );
long (FAR WINAPI * ReadMailboxTerminate) ( long a, long b, long *wert, long mode );
int	 (FAR WINAPI * WriteMailbox) ( long a, long b, long wert );
int	 (FAR WINAPI * WriteMailboxTerminate) ( long a, long b, long wert, int sizeflag );


/* Lädt die DLL, Initialisiert die DLL, lädt das Programm ... */
int	DSPInitProgramm( HWND hParent, int iTarget, LPSTR strDLL, LPSTR strDSPProgramm )
{
	FARPROC func;

#ifdef TEST_WITHOUT_DSP
	return 1;
#endif
	
	if(  hDSP_DLL==NULL  )
		hDSP_DLL = LoadLibrary( strDLL );
	if(  (UINT)hDSP_DLL<32  )
	{
		hDSP_DLL = NULL;
		FehlerRsc( E_DSP_DLL );
		return -1;
	}

	(FARPROC)func = GetProcAddress( hDSP_DLL, target_open );
	if(  func==NULL  ||  (*func)( iTarget )==0  )
	{
		FreeLibrary( hDSP_DLL );
		hDSP_DLL = NULL;
		FehlerRsc( E_DSP_TARGET );
		return -1;
	}

	// DSP-Reset
	(FARPROC)func = GetProcAddress(hDSP_DLL, target_reset);
	(*func)( iTarget );
	(FARPROC)func = GetProcAddress(hDSP_DLL, target_run);
	(*func)( iTarget );
	(FARPROC)func = GetProcAddress(hDSP_DLL, start_talker);
	(*func)( iTarget );

	// Programm laden
	(FARPROC)func = GetProcAddress(hDSP_DLL, iicoffld );
	if(  (*func)( strDSPProgramm, iTarget, NULL/*hParent*/ )  )
	{
		FreeLibrary( hDSP_DLL );
		hDSP_DLL = NULL;
		FehlerRsc( E_DSP_TARGET );
		return -1;
	}

	// und nun ausführen
	(FARPROC)func = GetProcAddress(hDSP_DLL, start_app );
	(*func)( iTarget );
	(FARPROC)func = GetProcAddress(hDSP_DLL, target_cardinfo );
	pDSPInfo = (CARDINFO *)(*func)( iTarget );

	ReadMailbox = GetProcAddress(hDSP_DLL, read_mailbox );
	ReadMailboxTerminate = GetProcAddress(hDSP_DLL, read_mb_terminate );
	WriteMailbox = GetProcAddress(hDSP_DLL, write_mailbox );
	WriteMailboxTerminate = GetProcAddress(hDSP_DLL, write_mb_terminate );

	return iTarget;
}
/* 3.11.98 */



/*****************************************
****	Die low-level Senderoutinen
*****************************************/


void	send_long( long what )
{
	(*WriteMailbox)( 0, 0, what );
}

void	send_block( long *p, long len )
{
	while(  len-->0  )
	(*WriteMailbox)( 0, 0, *p++ );
}


/* Da alles per Timer-Interupt läuft, hier GLOBALE Variablen */
typedef enum { QUERY_DSP=1, START_DSP, END_DSP, START_PID, END_PID, TUNNEL_VOLTAGE, TAKE_SPS, START_SCAN, END_SCAN, INVERT_PIEZO, EXPONENTIALLY } DSP_COMMAND;

#define MAX_DSP_QUEUE 16
#define DSP_CMD_SEMAPHORE	"DspCommandStack"

volatile DSP_COMMAND	DspCommand[MAX_DSP_QUEUE];
volatile LONG					DspParameter[MAX_DSP_QUEUE];
volatile LONG					iDspCommands=0;

// Der letzte Rückgabewert ...
volatile LONG				lDspLastResult;
volatile LONG				lDspData[2048];
volatile BOOL				bDspHasSpektrum;

// Ungleich Null, wenn der Prozess läuft
HANDLE	DspThreadHandle, hDspSemaphore;

// Über diese Funktion läuft alle Kommunikation mit dem DSP!
DWORD	WINAPI	DspThread(LPVOID param)
{
	HANDLE	hDspCommandStack;
	LONG		*pParam;
	int			iAktDspCommand;

	BOOLEAN	bReadScan;
	UWORD		*pData1, *pData2;
	LONG		lScanW=0, lScanH=0, lPos=0, l;
	HWND		hwndPID=NULL, hwndScan=NULL;

	// Als erstes DSP initialisieren
	if(  DSPInitProgramm( (HWND)param, 0, strDspDllPath, strDspPrgPath )==-1  )
	{
		DspThreadHandle = NULL;
		return 0l;
	}

	bReadScan = FALSE;
	iDspCommands = 0;
	hDspCommandStack = CreateSemaphore( NULL, 1, 1, DSP_CMD_SEMAPHORE );

	// Und nun für immer die Hauptschleife ausführen ...
	while(  DspThreadHandle!=NULL  )
	{
		Sleep(1);
#ifdef	TEST_WITHOUT_DSP
		if(  bReadScan  )
		{
			if(  lPos<lScanH  )
			{
				for(  l=0;  l<lScanW;  l++  )
				{
					if(  pData1  )
						*pData1++ = rand()%(256+lPos)+lPos+l*100-lScanW*50;
					if(  pData2  )
						*pData2++ = rand();
				}
				PostMessage( hwndScan, WM_SCANLINE_READY, lPos, 0 );
				lPos ++;
			}
			if(  lPos>=lScanH )
				bReadScan = FALSE;								 
		}
#else
		/* Da kam etwas vom DSP! */
		if(  (*ReadMailboxTerminate)( 0, 0, &l, TRUE )  )
		{
			switch( l )
			{
				case ABORTSCAN:	if(  bReadScan  )
												{
													/* evt. noch laufenden Scan abbrechen */
													DestroyWindow( hwndScan );
													hwndScan = FALSE;
													bReadScan = FALSE;
												}
												break;
				case STARTSPS:	{ /* Lese ein Tunnelspektrum */
													LONG	lData;
													for(  l=0;  l<2048;  l++  )
													{
														lData = (*ReadMailbox)( 0, 0 );
														lDspData[l] = lData;
													}
													if(hwndPID!=NULL) {
														PostMessage( hwndPID, WM_SCANSPS_READY, 2048, 0 );
													}
													// Sonst: Puffer leeren
													while(  (*ReadMailboxTerminate)( 0, 0, &l, TRUE )  )
														;
												}
												break;
				case STARTLINE:	if(  bReadScan  )
												{ /* Lese eine Scanzeile */
													LONG	lData;
													for(  l=0;  l<lScanW;  l++  )
													{
														lData = (*ReadMailbox)( 0, 0 );
														if(  pData1!=NULL  )
															*pData1++ = (UWORD)HIWORD(lData);
														if(  pData2!=NULL  )
															*pData2++ = (UWORD)LOWORD(lData);
													}
													PostMessage( hwndScan, WM_SCANLINE_READY, lPos, 0 );
													lPos++;
													if(  lPos>=lScanH )
													{
														bReadScan = FALSE;								 
													}
												}
												else
												{
													// Sonst: Puffer leeren
													while(  (*ReadMailboxTerminate)( 0, 0, &l, TRUE )  )
														;
												}
												break;
			}
		}
#endif

		// Kann ich auf den Kommando-Stack zugreifen?
		if(  WaitForSingleObject( hDspCommandStack, 1 )==WAIT_TIMEOUT  )
			continue;	// also Nein ...

		if(  iDspCommands==0  )
		{
			// Es gab aber keine neuen Anweisungen ...
			ReleaseSemaphore( hDspCommandStack, 1, NULL );
			continue;
		}

		/* Kommando lokal speichern und aus der Queue löschen */
		iAktDspCommand = iDspCommands-1;
		pParam = (LONG *)DspParameter[iAktDspCommand];

			/* Kommando ausführen */
		switch( DspCommand[iAktDspCommand]&0x00FF )
#ifdef	TEST_WITHOUT_DSP
		{
			case START_DSP:	
			case END_DSP:
			case END_PID:
			case END_SCAN:
				bReadScan = FALSE;
				break;
			case QUERY_DSP:
				lDspLastResult = rand()*2|(rand()<<17);
				break;
			case START_SCAN:
				hwndScan = (HWND)pParam[0];
				pData1 = (UWORD *)pParam[1];
				pData2 = (UWORD *)pParam[2];
				lScanW = ((SCANPAR *)(pParam+3))->iXPts;
				lScanH = ((SCANPAR *)(pParam+3))->iYPts;
				lPos = 0;
				bReadScan = TRUE;
			break;
		}
#else
		{
			case START_DSP:	
			case END_DSP:
			case END_SCAN:
			case END_PID:		send_long( ABORT );
											lDspLastResult = 0l;
											if(  bReadScan  )
											{
												/* evt. noch laufenden Scan abbrechen */
												DestroyWindow( hwndScan );
												hwndScan = NULL;
												bReadScan = FALSE;
											}
											lDspLastResult = (*ReadMailbox)( 0, 0 );
										break;
			case QUERY_DSP:	if(  !bReadScan  )
											{
												send_long( GET_VALUE );
												send_long( (LONG)pParam );
												lDspLastResult = (*ReadMailbox)( 0, 0 );
											}
											else
												lDspLastResult = ABORTSCAN;
										break;
			case INVERT_PIEZO:
											send_long( INVERT_PIEZO_CMD );
											send_long( (LONG)pParam );
										break;
			case EXPONENTIALLY:
											send_long( EXPONENTIALLY_CMD );
											send_long( (LONG)pParam );
										break;
			case TAKE_SPS:
											send_long( GET_SPS_CMD );
										break;
			case TUNNEL_VOLTAGE:
											send_long( TUNNELVOLTAGE_CMD );
											send_long( (LONG)pParam );
										break;
			case START_PID: 
											send_long( SET_PID_PARAMETER );
											send_block( pParam+1, 5 );
											lDspLastResult = (*ReadMailbox)( 0, 0 );
											hwndPID = (HWND)(*pParam);
										break;
			case START_SCAN:/* Starte einen neuen Scan */
											send_long( SET_SCAN_PARAMETER );
											send_block( pParam+3, 11 );
											hwndScan = (HWND)pParam[0];
											pData1 = (UWORD *)pParam[1];
											pData2 = (UWORD *)pParam[2];
											lScanW = ((SCANPAR *)(pParam+3))->iXPts;
											lScanH = ((SCANPAR *)(pParam+3))->iYPts;
											lPos = 0;
											lDspLastResult = (*ReadMailbox)( 0, 0 );
											bReadScan = TRUE;
											/* ACHTUNG: pData1, pData2, lScanW, und lScanH MÜSSEN gesetzt werden! */
										break;
		}
#endif
		iDspCommands--;
		ReleaseSemaphore( hDspCommandStack, 1, NULL );
	}
	// Und hier geht's nur bei Programmende hin ...
	CloseHandle( hDspCommandStack );
	send_long( ABORT );
	DspThreadHandle = NULL;
	return 1;
}
// 22.2.00


/************************************************************************************
****	Die eigentliche Kommunikation erfolgt über diese Routinen
************************************************************************************/

/* Beendet Regelung */
void	DspRetract( void )
{
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = END_PID;
			DspParameter[iDspCommands] = 0L;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}

/* Beendet Regelung */
long	DspQueryValue( LONG lModus )
{
	LONG	iAktDsp, iRetrys=50, iRet;

	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			iAktDsp = iDspCommands;
			DspCommand[iDspCommands] = QUERY_DSP;
			DspParameter[iDspCommands] = lModus;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}

	/* Warten auf Ausführung (ODER eine Sekunde warten) */
	do
	{
		Sleep(1);
		// Kann ich auf den Kommando-Stack zugreifen?
		if(  WaitForSingleObject( hDspSemaphore, 10 )==WAIT_TIMEOUT  )
			continue;
		if(	 iAktDsp>=iDspCommands  )
		{
			iRet = lDspLastResult;
			ReleaseSemaphore( hDspSemaphore, 1, NULL );
			return iRet;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
	while(  --iRetrys>0  );
	return ABORTSCAN;
}

/* Sended neuen PIEZO-invert-status */
void	DspInvertPiezo( int status )
{
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = INVERT_PIEZO;
			DspParameter[iDspCommands] = status;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}
/* Sended neuen PIEZO-exponentiell-linear-regelungs-status */
void	DspExponentRegel( int status )
{
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = EXPONENTIALLY;
			DspParameter[iDspCommands] = status;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}
/* Sended tunnel volts in DSP units (5V = 32767) */
void	DspTunnelVoltage( long volt )
{
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = TUNNEL_VOLTAGE;
			DspParameter[iDspCommands] = volt;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}
/* Sended tunnel volts in DSP units (5V = 32767) */
void	DspDoSps()
{
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = TAKE_SPS;
			DspParameter[iDspCommands] = 0;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}
/* Startet neuen PID */
void	DspPID( HWND hwndOszi, PIDPAR *pPID )
{
	static LONG	PIDParams[1+sizeof(PIDPAR)/sizeof(LONG)];

	PIDParams[0] = (LONG)hwndOszi;
	memcpy( PIDParams+1, pPID, sizeof(PIDPAR) );
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = START_PID;
			DspParameter[iDspCommands] = (LONG)PIDParams;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}

/* Startet neuen Scan */
void	DspScan( HWND hwndScan, UWORD *pData1, UWORD *pData2, SCANPAR *pSCAN )
{
	static LONG	SCANParams[3+sizeof(SCANPAR)/sizeof(LONG)];

	SCANParams[0] = (LONG)hwndScan;
	SCANParams[1] = (LONG)pData1;
	SCANParams[2] = (LONG)pData2;
	memcpy( SCANParams+3, pSCAN, sizeof(SCANPAR) );
	// Kann ich auf den Kommando-Stack zugreifen?
	if(  WaitForSingleObject( hDspSemaphore, 1000 )!=WAIT_TIMEOUT  )
	{
		// Ist da noch platz auf dem Stack?
		if(  iDspCommands<MAX_DSP_QUEUE  )
		{
			DspCommand[iDspCommands] = START_SCAN;
			DspParameter[iDspCommands] = (LONG)SCANParams;
			iDspCommands ++;
		}
		ReleaseSemaphore( hDspSemaphore, 1, NULL );
	}
}



/*********************************************************************************************/
/* Lock-in Kommunikation */
/**************************
EG&G 72xx Lock-In Kommandotabelle

DD 32		// SPACE als Trennzeichen

imode 1	// Stromeingang unempfinglich (Hohe Bandbreite)
float 1	// Keine Masseverbindung
cp 0		// AC-Coupling
lf 3 1	// 50 Hz

aqn			// Autophase
sen 27	// 1 µA..2fA(1)
of %li	// Oszillatorfrequenz in mHz
*/

// Für seriellen Port

HANDLE				hComm=INVALID_HANDLE_VALUE;
DCB						ComDCB;
COMMTIMEOUTS	ComTOS={ 50, 50, 100, 150, 100 };

WORD	iLockInID=0;

int			iComNr=0;
int			iLockIn=0;

double	fOszillatorFrequenz=33000.0;
double	fOszillatorAmplitude=0.050;


HANDLE	OpenLockin( int iComNr )
{
	COMMTIMEOUTS	ComTOS={ 150, 2000, 500, 500, 2000 };
	DCB				ComDCB;
	HANDLE		hComm;
	LONG			l;
	BYTE			comdcb[16], str[256];

	//iComNr = 2;
	wsprintf( str, "COM%i", iComNr );
#ifdef BIT32
	hComm = CreateFile( str, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
#else
	hComm = OpenComm( str, 4096, 128 );
#endif
	if(  hComm==INVALID_HANDLE_VALUE  )
	{
		iLockInID = 0;
		MessageBox( NULL, GetStringRsc( E_COM ) , NULL, MB_OK );
		return 0;
	}
	GetCommState( hComm, &ComDCB );
	SetupComm( hComm, 14, 16 );
	ClearCommBreak( hComm );
	SetCommState( hComm, &ComDCB);
	SetCommTimeouts( hComm, &ComTOS );
	ComDCB.fRtsControl = RTS_CONTROL_TOGGLE;
	SetCommTimeouts( hComm, &ComTOS );
	SetCommState( hComm, &ComDCB );
  WriteFile( hComm, "id", 2, &l, NULL );
	if(  !ReadFile( hComm, str, 6, &l, NULL )  ||  l==0  )
	{
		FehlerRsc( E_LOCKIN );
		CloseHandle( hComm );
		return NULL;
	}
	iLockInID = atoi( str );

	WriteFile( hComm, "DD 32\x0D"		// SPACE als Trennzeichen
										"imode 1\x0D"	// Stromeingang unempfinglich (Hohe Bandbreite)
										"float 1\x0D"	// Keine Masseverbindung
										"cp 0\x0D"		// AC-Coupling
										"lf 3 1\x0D",	// 50 Hz
										34, &l, NULL );
	return hComm;
}


/*************************************************************************/
/* AutoApproachdialog */



LRESULT CALLBACK MessAutoApproachDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//showWindow();
	
	return FALSE;
}
// 24.12.03


/*************************************************************************/
/* Approachdialog */

enum MOTOR_STATUS {MOTOR_IDLE, MOTOR_IWD, MOTOR_OWD};

typedef struct {

	unsigned long AnzPulse;
	unsigned long PulseTime;
	unsigned long PulseCount;
	unsigned long CycleCount;
	unsigned int  TastOn;
	unsigned int  TastOff;
	unsigned int  PortAdr;
	enum MOTOR_STATUS MotorStatus;

} MOTOR_CONTROL;


// this funktion can be also run in a thread loop
DWORD	WINAPI MotorThread( LPVOID lp )
{
	MOTOR_CONTROL *mc=(MOTOR_CONTROL *)lp;
	LARGE_INTEGER	llDeltaTime, llTime, llCurrentTime;
	long double		fDeltaTime, fTime, fCurrentTime;
	unsigned char OutByte=0;

	QueryPerformanceFrequency( &llDeltaTime );
	fDeltaTime = (double)llDeltaTime.LowPart + 2147483647.0*(double)llDeltaTime.HighPart;
	fDeltaTime = (fDeltaTime/1000.0)*mc->PulseTime;	// counts per Puls
	
	QueryPerformanceCounter( &llTime );
	fTime = (double)llTime.LowPart + 2147483647.0*(double)llTime.HighPart;

	// loop everything
	mc->PulseCount = 0;
	mc->CycleCount = 0;
	while(  mc->PulseCount<mc->AnzPulse  &&  mc->MotorStatus!=MOTOR_IDLE  )
	{
		fTime += fDeltaTime;

		//Ausgabebyte für LPT je nach Fahrrichtung setzen
		if(mc->MotorStatus==MOTOR_IWD  )
			OutByte = 0x03;
		else if(  mc->MotorStatus==MOTOR_OWD  )
			OutByte = 0x0C;

		//Während Low-Phase des Tastzyklus Ausgabebyte nullen
		if(  mc->CycleCount >= mc->TastOn)
			OutByte=0x00;

		//Ausgabebyte an LPT senden, ACHTUNG: _outp funktioniert vermutlich NICHT unter Windows NT
		_outp((unsigned short)mc->PortAdr,OutByte);

		//Interne Zähler inkrementieren
		mc->CycleCount++;

		//Nach abgelaufenem Tastzyklus Cyclecount zurücksetzen
		if(  mc->CycleCount > mc->TastOn+mc->TastOff  )
		{
			mc->CycleCount = 0;
			mc->PulseCount++;
		}
		
		// warten (use high performance timer ...
		do
		{
			QueryPerformanceCounter( &llCurrentTime );
			fCurrentTime = (double)llCurrentTime.LowPart + 2147483647.0*(double)llCurrentTime.HighPart;
		}
		while(  fCurrentTime<fTime  &&  mc->MotorStatus!=MOTOR_IDLE  );
		fTime = fCurrentTime;
	}
	//Checken ob Motor duchgelaufen ist und ggfs. stoppen
	_outp((unsigned short)mc->PortAdr,0);
	mc->MotorStatus = MOTOR_IDLE;
	return 1;
}


MOTOR_CONTROL Snom_MotorControl;	// sves the motor control parameters

#define LPT1 0x378			/* PC: LPT1 Registeradresse */
#define LPT2 0x278			/*     LPT2       "         */
#define LPT3 0x3BC			/* Nicht ganz korrekt, wenn LPT3, dann versch. sich normalerweise die anderen LPT-Nr. */


LRESULT CALLBACK MessApproachDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern HWND			hModeLess;
	static HWND			hCombo, hW, hH, hwndScan;
	static UINT			idTimer;
	static UINT			uUnbenannt=0,invert1=0,expo=0;	/* Zähler für Bildnummer */
	static double		fSkal=1000.0;	/* Default: mum */
	static long			pl;
	static HANDLE		MotorThreadHandle;
	HANDLE	h;
	BYTE		str[256];
	long		i, j;

	switch( message )
	{
		case WM_INITDIALOG:
		{
			extern HWND hwndFrame;

			CreateWindowEx( WS_EX_NOPARENTNOTIFY, "Timerproc", "Timerproc", WS_DISABLED, 0, 0, 0, 0, hwndFrame, NULL, hInst, hDlg );
			for(  j=0;  j<4;  j++  )
			{
				OsziNum[j]=0;
				for(  i=0;  i<OSZI_MAX;  i++  )
					OsziDaten[j][i] = 0;
			}

			h = GetDlgItem(hDlg,MESS_APPROACH_PORT);
			SendMessage( h, CB_RESETCONTENT, 0, 0 );
			SendMessage( h, CB_ADDSTRING, 0, (LONG)(LPSTR)"NONE" );
			j = 0;
			// Maximale Anzahl von Com-Ports abfragen
			for(  i=1;  i<4;  i++  )
			{
				HANDLE hComm;
				wsprintf( str, "LPT%i", i );
				hComm = CreateFile( str, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
				if(  hComm!=INVALID_HANDLE_VALUE  )
				{
					if(  j==0  )
						j = i;	// use first port
					SendMessage( h, CB_ADDSTRING, 0, (LONG)(LPSTR)str );
					CloseHandle( hComm );
				}
			}
			SendMessage( h, CB_SETCURSEL, j, 0 );
			SetFocus( h );

			// Sinvolle Werte für Motor von Approach
			Snom_MotorControl.PortAdr=0;
			switch( j )
			{
				case 1:
					Snom_MotorControl.PortAdr = LPT1;
					break;
				case 2:
					Snom_MotorControl.PortAdr = LPT2;
					break;
				case 3:
					Snom_MotorControl.PortAdr = LPT3;
					break;
			}
			Snom_MotorControl.MotorStatus = MOTOR_IDLE;
			Snom_MotorControl.AnzPulse = 1;
			Snom_MotorControl.TastOn = 4;
			Snom_MotorControl.TastOff = 4;
			Snom_MotorControl.PulseTime = 1;

			// Zuerst die Approach-Werte in den Dialog eintragen
			SetDlgItemInt( hDlg, MESS_APPROACH_PULSDAUER, Snom_MotorControl.PulseTime, FALSE );
			SetDlgItemInt( hDlg, MESS_APPROACH_ANZPULSE, Snom_MotorControl.AnzPulse, FALSE );
			SetDlgItemInt( hDlg, MESS_APPROACH_TASTDRIVE, Snom_MotorControl.TastOn, FALSE );
			SetDlgItemInt( hDlg, MESS_APPROACH_TASTSTOP, Snom_MotorControl.TastOff, FALSE );
			SetTimer( hDlg, 14, 100, NULL );
			MotorThreadHandle = NULL;
			hwndScan = NULL;
			return TRUE;
		}

		case WM_TIMER:
		{
			PAINTSTRUCT	ps;
			RECT  rect;
			extern HWND hwndFrame;
			HDC		hDC;
			HRGN	hRgn,hRgn2;
			int		x, y, x2, y2,laenge;
			LONG	lData = DspQueryValue( TOPO|ERRO );
			LONG	iError, iTopo;

			if(  lData==ABORTSCAN  )
				break;	/* Nix zu tun! */

			/* Immer Scrollen */
			MemMove( &(OsziDaten[0][0]), &(OsziDaten[0][1]), 127*sizeof(DWORD) );
			MemMove( &(OsziDaten[2][0]), &(OsziDaten[2][1]), 127*sizeof(DWORD) );
			OsziNum[0] = OsziNum[2] = 127;
			OsziNum[1] = OsziNum[3] = 0;

			/* Und nun Daten einfügen */
			iTopo = (signed short)HIWORD(lData);
			if(  iTopo<OsziMin[0]  )
				OsziMin[0] = iTopo;
			if(  iTopo>OsziMax[0]  )
				OsziMax[0] = iTopo;
			OsziDaten[0][OsziNum[0]++] = iTopo;
			wsprintf( str, GetStringRsc(STR_REGEL), iTopo );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)str );

			wsprintf( str, GetStringRsc(STR_REGEL), iTopo );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LONG)str );

			iError = (signed short)LOWORD(lData);
			if(  iError<OsziMin[2]  )
			OsziMin[2] = iError;
			if(  iError>OsziMax[2]  )
			OsziMax[2] = iError;
			OsziDaten[2][OsziNum[2]++] = iError;
    
			RedrawOszi( hDlg );
			wsprintf( str, "%i", iError );
			SetDlgItemText( hDlg, MESS_SP_IST, str  );

			wsprintf( str, "%i",  (iTopo+maxZScannerBetrag)*100/(2*maxZScannerBetrag));
			SetDlgItemText( hDlg, MESS_APPROACH_ZSCANNER1, str);

			EnableWindow(GetDlgItem(hDlg,MESS_APPROACH_APPROACH),Snom_MotorControl.MotorStatus==MOTOR_IDLE);
			EnableWindow(GetDlgItem(hDlg,MESS_APPROACH_RETRACT),Snom_MotorControl.MotorStatus==MOTOR_IDLE);
			EnableWindow(GetDlgItem(hDlg,MESS_APPROACH_STOP),Snom_MotorControl.MotorStatus!=MOTOR_IDLE);
		}
		/*****21.11.2003***/
		break;

		case WM_DESTROY:
			// should terminate everything anyway
			Snom_MotorControl.MotorStatus = MOTOR_IDLE;
			// still running?
			if(  MotorThreadHandle!=NULL  )
			{
				CloseHandle( MotorThreadHandle );
				MotorThreadHandle = NULL;
			}
			// Wichtige Parameter merken!
			KillTimer( hDlg, 14 );
			idTimer = 0;
			OsziNum[0] = OsziNum[2] = 0;
			Snom_MotorControl.PulseTime = GetDlgItemInt( hDlg, MESS_APPROACH_PULSDAUER, NULL, FALSE );
			Snom_MotorControl.AnzPulse = GetDlgItemInt( hDlg, MESS_APPROACH_ANZPULSE, NULL, FALSE );
			Snom_MotorControl.TastOn = GetDlgItemInt( hDlg, MESS_APPROACH_TASTDRIVE, NULL, FALSE );
			Snom_MotorControl.TastOff = GetDlgItemInt( hDlg, MESS_APPROACH_TASTSTOP, NULL, FALSE );
			SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_APPROACH, FALSE );
		break;

		case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
			case MESS_APPROACH_PORT:
				// Port öffnen und initialisieren
				h = GetDlgItem(hDlg,MESS_APPROACH_PORT);
#ifdef BIT32
				if(  HIWORD(wParam)==CBN_SELCHANGE  )
#else
				if(  HIWORD(lParam)==CBN_SELCHANGE  )
#endif
				{
					i = (WORD)SendMessage( h, CB_GETCURSEL, 0, 0l );
					if(  i>0  )
					{
						BYTE	str[32];
						SendMessage( h, CB_GETLBTEXT, i, (LPARAM)(LPSTR)str );
						switch( str[3]-'0' )
						{
							case 1:
								Snom_MotorControl.PortAdr = LPT1;
								break;
							case 2:
								Snom_MotorControl.PortAdr = LPT2;
								break;
							case 3:
								Snom_MotorControl.PortAdr = LPT3;
								break;
							// unknown
							default:
								Snom_MotorControl.PortAdr = 0;
								break;
						}
					}
				}
			break;

			//Ausfahren
			case MESS_APPROACH_APPROACH:
				// stop previous action
				Snom_MotorControl.MotorStatus = MOTOR_IDLE;
				// still running?
				if(  MotorThreadHandle!=NULL  )
				{
					CloseHandle( MotorThreadHandle );
					MotorThreadHandle = NULL;
				}
				Snom_MotorControl.PulseTime = GetDlgItemInt( hDlg, MESS_APPROACH_PULSDAUER, NULL, FALSE );
				Snom_MotorControl.AnzPulse = GetDlgItemInt( hDlg, MESS_APPROACH_ANZPULSE, NULL, FALSE );
				Snom_MotorControl.TastOn = GetDlgItemInt( hDlg, MESS_APPROACH_TASTDRIVE, NULL, FALSE );
				Snom_MotorControl.TastOff = GetDlgItemInt( hDlg, MESS_APPROACH_TASTSTOP, NULL, FALSE );
				Snom_MotorControl.MotorStatus = MOTOR_OWD;
				CreateThread( NULL, 0, MotorThread, (LPVOID)&Snom_MotorControl, 0, &MotorThreadHandle );
//				MotorThread( &Snom_MotorControl );
			break;

			//Einfahren
			case MESS_APPROACH_RETRACT:
				// stop previous action
				Snom_MotorControl.MotorStatus = MOTOR_IDLE;
				// still running?
				if(  MotorThreadHandle!=NULL  )
				{
					CloseHandle( MotorThreadHandle );
					MotorThreadHandle = NULL;
				}
				Snom_MotorControl.PulseTime = GetDlgItemInt( hDlg, MESS_APPROACH_PULSDAUER, NULL, FALSE );
				Snom_MotorControl.AnzPulse = GetDlgItemInt( hDlg, MESS_APPROACH_ANZPULSE, NULL, FALSE );
				Snom_MotorControl.TastOn = GetDlgItemInt( hDlg, MESS_APPROACH_TASTDRIVE, NULL, FALSE );
				Snom_MotorControl.TastOff = GetDlgItemInt( hDlg, MESS_APPROACH_TASTSTOP, NULL, FALSE );
				Snom_MotorControl.MotorStatus = MOTOR_IWD;
				CreateThread( NULL, 0, MotorThread, (LPVOID)&Snom_MotorControl, 0, &MotorThreadHandle );
//				MotorThread( &Snom_MotorControl );
			break;

			//Motor anhalten
			case MESS_APPROACH_STOP:
				// should terminate everything anyway
				Snom_MotorControl.MotorStatus = MOTOR_IDLE;
				// still running?
				if(  MotorThreadHandle!=NULL  )
				{
					CloseHandle( MotorThreadHandle );
					MotorThreadHandle = NULL;
				}
			break;
		}
	}
	return FALSE;
} // 24.1.03


/*************************************************************************/
/* Messdialog */


// PID-Dialog
LRESULT CALLBACK MessPIDDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern HWND			hModeLess;
	static HWND			hCombo, hW, hH, hwndScan;
	static UINT			idTimer;
	static double		fSkal=1000.0;	/* Default: mum */
	static long			pl;
	BYTE		str[256],str2[256];
	long		i, j;
	static UINT			CoarseApproachActive=0;
	static UINT			FineApproachActive=0;
	HPEN	hOldPen;
	HBRUSH hOldBrush;
	RECT	rect;
	int iTopo_alt=0,nahe=0,iIst_alt=0; 

	switch( message )
	{
		case WM_INITDIALOG:
		{
			extern HWND hwndFrame;
			OsziNum[1] = OsziNum[3] = 0;	// may hold SPS data afterwards
			CreateWindowEx( WS_EX_NOPARENTNOTIFY, "Timerproc", "Timerproc", WS_DISABLED, 0, 0, 0, 0, hwndFrame, NULL, hInst, hDlg );
			for(  j=0;  j<4;  j++  )
			{
				OsziNum[j]=0;
				for(  i=0;  i<OSZI_MAX;  i++  )
					OsziDaten[j][i] = 0;
			}

			hCombo = GetDlgItem( hDlg, MESS_MESS_MODUS );
			SendMessage( hCombo, CB_RESETCONTENT, 0, 0l );
			SendMessage( hCombo, CB_ADDSTRING, 0, (LONG)STR_NIX );
			SendMessage( hCombo, CB_ADDSTRING, 0, (LONG)STR_ERROR );
			SendMessage( hCombo, CB_ADDSTRING, 0, (LONG)STR_LUMI );
			SendMessage( hCombo, CB_SETCURSEL, 0, 0 );
			if(  Scan.iModus&ERRO  )
				SendMessage( hCombo, CB_SETCURSEL, 1, 0 );
			else if(  Scan.iModus&LUMI  )
				SendMessage( hCombo, CB_SETCURSEL, 2, 0 );
			if(  Scan.iYPts==0  )
				Scan.iYPts = Scan.iXPts;

			// Zuerst die Scan-Werte in den Dialog eintragen
			SetDlgItemInt( hDlg, MESS_SCAN_XPTS, Scan.iXPts, FALSE );
			SetDlgItemInt( hDlg, MESS_SCAN_YPTS, Scan.iYPts, FALSE );
			SetDlgItemInt( hDlg, MESS_SCAN_VERSATZ, Scan.iOffset, FALSE );
			SetDlgItemInt( hDlg, MESS_SCAN_FREQ, Scan.lFreq, FALSE );

			/* PID-Werte */
			SetDlgItemText( hDlg, MESS_PID_FREQ, (LPSTR)gcvt( ((double)PID.lFreq)/1000.0, 6, str ) );
			if(  PID.lSoll==NOT_INIT  )
				PID.lSoll = (signed short)LOWORD(DspQueryValue(TOPO|ERRO));
			SetDlgItemText( hDlg, MESS_PID_SOLL, (LPSTR)ltoa( PID.lSoll, str, 10 ) );
			SetDlgItemText( hDlg, MESS_PID_PROP, (LPSTR)gcvt( ((double)PID.lProp)/1000.0, 6, str ) );
			SetDlgItemText( hDlg, MESS_PID_INT, (LPSTR)gcvt( ((double)PID.lInt)/1000.0, 6, str ) );
			SetDlgItemText( hDlg, MESS_PID_DIFF, (LPSTR)gcvt( ((double)PID.lDiff)/1000.0, 6, str ) );
			SetDlgItemText( hDlg, MESS_PID_VOLTAGE, (LPSTR)gcvt( ((double)PID.lVoltage)/6553.4, 6, str ) );

			// check for Piezo invert
			CheckDlgButton( hDlg, MESS_SCAN_INVERT, (PID.bFlags&PIEZO_IS_INVERT)!=0 );
			DspInvertPiezo( (PID.bFlags&PIEZO_IS_INVERT)!=0 );
			CheckDlgButton( hDlg, MESS_EXPONENTIALLY, (PID.bFlags&EXPONENTIAL_PID)!=0 );
			DspExponentRegel( (PID.bFlags&EXPONENTIAL_PID)!=0 );

			// Setzt gleichzeitig auch Bild- und Piezo-Werte!
			SendMessage( hDlg, WM_COMMAND, HUB_MKM, BM_SETCHECK );
			SetTimer( hDlg, 15, 10, NULL );
			
			hwndScan = NULL;

			CheckDlgButton( hDlg, MESS_SCAN_FOREVER, bForeverScan );

			// Sinvolle Werte für Motor von Approach
			Snom_MotorControl.MotorStatus = MOTOR_IDLE;
			Snom_MotorControl.PortAdr=0x378;	// LPT1
			Snom_MotorControl.AnzPulse = 1;
			Snom_MotorControl.TastOn = 4;
			Snom_MotorControl.TastOff = 4;
			Snom_MotorControl.PulseTime = 1;
			return TRUE;
		}

		case WM_TIMER:
		{
			LONG	lData = DspQueryValue( TOPO|ERRO );
			LONG	iTopo,iError;
			static LONG	iLastError;

			if(  lData==ABORTSCAN  )
				break;	/* Nix zu tun! */

			/* Immer Scrollen */
			MemMove( &(OsziDaten[0][0]), &(OsziDaten[0][1]), 127*sizeof(DWORD) );
			MemMove( &(OsziDaten[2][0]), &(OsziDaten[2][1]), 127*sizeof(DWORD) );
			OsziNum[0] = OsziNum[2] = 127;
			strcpy( OsziStr[0], "Topography" );
			strcpy( OsziStr[2], "Error" );
			/* Und nun Daten einfügen */
			iTopo = (signed short)HIWORD(lData);
			wsprintf( str, "%i",  (iTopo+maxZScannerBetrag)*100/(2*maxZScannerBetrag));
			SetDlgItemText( hDlg, MESS_PROZENT, str);
			if(  iTopo<OsziMin[0]  )
				OsziMin[0] = iTopo;
			if(  iTopo>OsziMax[0]  )
				OsziMax[0] = iTopo;
			OsziDaten[0][OsziNum[0]++] = iTopo;
			wsprintf( str, GetStringRsc(STR_REGEL), iTopo );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)str );

			wsprintf( str, GetStringRsc(STR_REGEL), iTopo );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LONG)str );

			iError = (signed short)LOWORD(lData);
			iError1 = iError;
			if(  iError<OsziMin[2]  )
				OsziMin[2] = iError;
			if(  iError>OsziMax[2]  )
				OsziMax[2] = iError;
			OsziDaten[2][OsziNum[2]++] = iError;
			iLastError = (0.9*(double)iLastError + 0.1*(double)iError);
			wsprintf( str, "%i", iLastError );
			SetDlgItemText( hDlg, MESS_PID_SHOW_SOLL, str );
			RedrawOszi( hDlg );

			//Bei aktivem Coarse Approach warten bis der Piezo voll ausgefahren ist
			if(FineApproachActive==1)
			{
				if(iTopo>maxZScannerBetrag-1)
				{
					nahe = iError-iIst_alt;
					if(nahe<0)
						nahe = -nahe;
					Snom_MotorControl.MotorStatus = MOTOR_OWD;
					Snom_MotorControl.AnzPulse = 1;
					Snom_MotorControl.TastOn = 4;
					Snom_MotorControl.TastOff = 4;
					Snom_MotorControl.PulseTime = 1;
					// time enough for one loop ...
					MotorThread( &Snom_MotorControl );
					//Flags setzen
					CoarseApproachActive = 1;
					FineApproachActive = 0;
					iTopo_alt = iTopo;
					iIst_alt = iError;
				}
			}

			//Bei aktivem Coarse Approach mit voll ausgefahrenem Piezo überprüfen
			//ob Kontakt hergestellt wurde
			if(CoarseApproachActive==1 && FineApproachActive==0)
			{
				//Bei Signaländerung Motogeschwindigkeit auf 1 Puls à 1ms verlangsamen
				if(iTopo<maxZScannerBetrag-5)
				{
					int wertx=iTopo_alt-iTopo;
					iTopo_alt=iTopo;
					Snom_MotorControl.AnzPulse = 1;
					Snom_MotorControl.TastOn = 4;
					Snom_MotorControl.PulseTime = 1;
					Snom_MotorControl.TastOff = 20;
				}
				//Bei Topo-Wertin mitte Motor stoppen
				Snom_MotorControl.MotorStatus = MOTOR_OWD;
				if(iTopo<maxZScannerBetrag/2) 
				{
					CoarseApproachActive = 0;
					SetDlgItemText(hDlg,MESS_PID_COARSEAPPROACH,"Coarse Approach");
				}
				else
					MotorThread( &Snom_MotorControl );
			}	
		}
		break;

		case WM_SCANSPS_READY:
		{
			// put an SPS-spectum to the oszi
			int i;
			strcpy( OsziStr[3], "SPS" );
			OsziMax[3] = -32768;
			OsziMin[3] = 32767;
			OsziNum[3] = 1024;
			for( i=0;  i<2048;  i+=2  ) {
				long w = lDspData[i];
				if(w<OsziMin[3]) {
					OsziMin[3] = w;
				}
				if(w>OsziMax[3]) {
					OsziMax[3] = w;
				}
				OsziDaten[3][i/2] = w;
			}
		}
		break;

		
		case WM_DESTROY:
			// PID-Regelung beenden (sollte man doch besser nachfragen!)
			//DspRetract();

			// Wichtige Parameter merken!
			/*** ACHTUNG: 
			 ***	PID-Parameter und Scan-Parameter wurden ja beim letzten 
			 ***	Start eines Bildes bzw. der Regelung schon ausgelesen und
			 ***	in den entsprechenden Strukturen gespeichert.
			 ***	Also muss nur noch die Piezo-Kalibration gelesen werden;
			 ***	die restlichen, evt. geänderten (aber ungetesteten) Werte
			 ***	vergisst diese Routine!
			 ***/
			GetDlgItemText( hDlg, MESS_PIEZO_X, str, 256 );
			fPiezoSkalX = atof(str)*fSkal/65536.0;	// Weite pro Pixel
			GetDlgItemText( hDlg, MESS_PIEZO_Y, str, 256 );
			fPiezoSkalY = atof(str)*fSkal/65536.0;	// Hoehe pro Pixel
			GetDlgItemText( hDlg, MESS_PIEZO_Z, str, 256 );
			fPiezoSkalZ = atof(str)*fSkal/65536.0;	// Z pro Pixel
			// Aufräumen
			KillTimer( hDlg, 15 );
			idTimer = 0;
			OsziNum[0] = OsziNum[2] = 0;
			SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_PID, FALSE );
		break;

		case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
			//** Skalierung einstelllen **
			case HUB_MKM:
				fSkal = 1000.0;
				goto mess_bildwerte_eintragen;
			case HUB_NM:
				fSkal = 1.0;
				goto mess_bildwerte_eintragen;
			case HUB_AA:
				fSkal = 0.1;
mess_bildwerte_eintragen:
				CheckRadioButton( hDlg, HUB_NM, HUB_MKM, wParam );
				/* Bild-Werte */
				sprintf( str, "%.5g", (Scan.iW+0.5)*fPiezoSkalX/fSkal );
				SetDlgItemText( hDlg, MESS_WEITE, str );
				sprintf( str, "%.5g", (Scan.iH+0.5)*fPiezoSkalY/fSkal );
				SetDlgItemText( hDlg, MESS_HOEHE, str );
				sprintf( str, "%.5g", (Scan.iXOff+Scan.iW/2.0)*fPiezoSkalX/fSkal );
				SetDlgItemText( hDlg, MESS_XOFF, str );
				sprintf( str, "%.5g", (Scan.iYOff+Scan.iH/2.0)*fPiezoSkalY/fSkal );
				SetDlgItemText( hDlg, MESS_YOFF, str );
				/* Scanner-Kalibration ... */
				sprintf( str, "%5g", fPiezoSkalX*65536.0/fSkal );
				SetDlgItemText( hDlg, MESS_PIEZO_X, str );
				sprintf( str, "%5g", fPiezoSkalY*65536.0/fSkal );
				SetDlgItemText( hDlg, MESS_PIEZO_Y, str );
				sprintf( str, "%5g", fPiezoSkalZ*65536.0/fSkal );
				SetDlgItemText( hDlg, MESS_PIEZO_Z, str );
			break;
			
			// Schneller Abbruch
			case MESS_ABORT:
				//** Zuerst: Werte auslesen: **
				DspRetract();
				OsziNum[3] = 0;	// clear tunnel spectrum
				PID_ermitteln=0;
				if(  hwndScan  )
				{
					HWND h=hwndScan;
					hwndScan = NULL;
					DestroyWindow(h);
				}
				// Die Scanroutine benachrichtigt die Interruptroutine DSPQueueProc!
				SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)GetStringRsc(STR_ENDE_PID) );
			break;

			// langsam Ausfahren
			case MESS_APPROACH_APPROACH:
				Snom_MotorControl.MotorStatus = MOTOR_OWD;
				Snom_MotorControl.AnzPulse = 1;
				Snom_MotorControl.TastOn = 4;
				Snom_MotorControl.TastOff = 4;
				Snom_MotorControl.PulseTime = 1;
				MotorThread( &Snom_MotorControl );
				break;

			// langsam Zurück
			case MESS_APPROACH_RETRACT:
				Snom_MotorControl.MotorStatus = MOTOR_IWD;
				Snom_MotorControl.AnzPulse = 1;
				Snom_MotorControl.TastOn = 4;
				Snom_MotorControl.TastOff = 4;
				Snom_MotorControl.PulseTime = 1;
				MotorThread( &Snom_MotorControl );
				break;

			// Start measuring
			case MESS_PID_SPS:
				DspDoSps();
				break;

				// Start measuring
			case MESS_SCAN_SEND:
			case MESS_PID_SEND:
				// first get tunnel voltage
				GetDlgItemText( hDlg, MESS_PID_VOLTAGE, str, 256 );
				PID.lVoltage = (long)(atof(str)*3276.7 );
				DspTunnelVoltage( PID.lVoltage );
				DspPID( hDlg, &PID );
				// now the rest
				GetDlgItemText( hDlg, MESS_PID_FREQ, str, 256 );
				PID.lFreq = (long)(atof(str)*1000.0);
				GetDlgItemText( hDlg, MESS_PID_SOLL, str, 256 );
				PID.lSoll = atol(str);
				GetDlgItemText( hDlg, MESS_PID_PROP, str, 256 );
				PID.lProp = (long)(atof(str)*1000.0);
				GetDlgItemText( hDlg, MESS_PID_INT, str, 256 );
				PID.lInt = (long)(atof(str)*1000.0);
				GetDlgItemText( hDlg, MESS_PID_DIFF, str, 256 );
				PID.lDiff = (long)(atof(str)*1000.0);
#if 0
				//???
        if(PID_ermitteln==0)
				 {
				   wait(1000);
					 long bestp=0,besti=0,differenz=0,bestd=0,minnoise=64000,minmin=32000,maxmax=-32000;
					 int k=0,x=0,l=0;
					 for(l=0;l<3;l++)
					 {
					  for(x=0;x<1000;x=x+5)
						{
							PID.lProp = (long)(x*1000.0);
				      DspPID( hDlg, &PID );
						  for(k=0;k<20;k++)
							{
								iError1 = (signed short)LOWORD( DspQueryValue( TOPO|ERRO ));
								if(iError1>maxmax) maxmax=iError1;
							  if(iError1<minmin) minmin=iError1;

							}
							 differenz=abs(maxmax-minmin);
							 if(minnoise>differenz)
                {
								 bestp=x;
								 minnoise=differenz;
								}
						}
						PID.lProp = (long)(bestp*1000.0);
				    DspPID( hDlg, &PID );
						minnoise=64000;
						maxmax=-32000;
						minmin=32000;
						for( x=0;x<1000;x=x+5)
						{
						  PID.lInt = (long)(x*1000.0);
				      DspPID( hDlg, &PID );
						  for( k=0;k<20;k++)
							{
								iError1 = (signed short)LOWORD( DspQueryValue( TOPO|ERRO ));
								if(iError1>maxmax) maxmax=iError1;
							  if(iError1<minmin) minmin=iError1;

							}
							 differenz=abs(maxmax-minmin);
							 if(minnoise>differenz)
                {
								 besti=x;
								 minnoise=differenz;
								}
						}
						PID.lInt = (long)(besti*1000.0);
				    DspPID( hDlg, &PID );
						minnoise=64000;
						maxmax=-32000;
						minmin=32000;
						for( x=0;x<1000;x=x+5)
						{
							PID.lDiff = (long)(x*1000.0);
				      DspPID( hDlg, &PID );
						  for( k=0;k<20;k++)
							{
								iError1 = (signed short)LOWORD( DspQueryValue( TOPO|ERRO ));
								if(iError1>maxmax) maxmax=iError1;
							  if(iError1<minmin) minmin=iError1;

							}
							 differenz=abs(maxmax-minmin);
							 if(minnoise>differenz)
                {
								 bestd=x;
								 minnoise=differenz;
								}
						}
						PID.lDiff = (long)(bestd*1000.0);
				    DspPID( hDlg, &PID );
						minnoise=64000;
						maxmax=-32000;
						minmin=32000;
					 }      
					 PID_ermitteln=1;
           SetDlgItemText( hDlg, MESS_PID_PROP, bestp );
					 PID.lProp = (long)(bestp*1000.0);
					 SetDlgItemText( hDlg, MESS_PID_INT, besti );
					 PID.lInt = (long)(besti*1000.0);
					 SetDlgItemText( hDlg, MESS_PID_DIFF, bestd );
					 PID.lDiff = (long)(bestd*1000.0);
					 DspPID( hDlg, &PID );
        				 
 				 } /*pid ermitteln ende*/
#endif
				// Wenn nur PID, dann hier abbrechen ...
				if(  wParam==MESS_PID_SEND  )
					break;

				//** Alles für eine Messung vorbereiten **
				//** Zuerst: Werte auslesen: **
				if(  hwndScan  )
				{
					HWND h=hwndScan;
					hwndScan = NULL;
					DestroyWindow(h);
				}

				// Erst mal Testen, ob überhaupt brauchbare Werte da sind ...
				Scan.iXPts = GetDlgItemInt( hDlg, MESS_SCAN_XPTS, NULL, FALSE );
				Scan.iYPts = GetDlgItemInt( hDlg, MESS_SCAN_YPTS, NULL, FALSE );
				if(  Scan.iXPts==0  ||  Scan.iYPts==0  )
					break;

				// Und nun die Scan-Werte
				i = SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
				Scan.iModus = TOPO;
				if(  i>0  )
					Scan.iModus |= (1<<i);

				GetDlgItemText( hDlg, MESS_SCAN_FREQ, str, 256 );
				Scan.lFreq = (long)(atof(str));			// Punkte pro Regelung
				Scan.lWinkel = 0;	// keine Drehung
				Scan.iDir = (LINKS2RECHTS);  // Hin & Zurück
				Scan.iOffset = (short signed)GetDlgItemInt( hDlg, MESS_SCAN_VERSATZ, NULL, TRUE );

				// Für alles weitere brauchen wir die Piezo-Eichung ...
				GetDlgItemText( hDlg, MESS_PIEZO_X, str, 256 );
				fPiezoSkalX = atof(str)*fSkal/65536.0;	// Weite pro Pixel
				GetDlgItemText( hDlg, MESS_PIEZO_Y, str, 256 );
				fPiezoSkalY = atof(str)*fSkal/65536.0;	// Hoehe pro Pixel
				GetDlgItemText( hDlg, MESS_PIEZO_Z, str, 256 );
				fPiezoSkalZ = atof(str)*fSkal/65536.0;	// Z pro Pixel

				// Und nun die Ausmaße berechnen
				GetDlgItemText( hDlg, MESS_WEITE, str, 256 );
				Scan.iW = atof(str)*fSkal/fPiezoSkalX;	// Gewünschte Weite/Gesamtweite mal Pixelzahl = Pixelzahl
				if(  Scan.iW>65535l  )
					Scan.iW = 65535l;
				GetDlgItemText( hDlg, MESS_HOEHE, str, 256 );
				Scan.iH = atof(str)*fSkal/fPiezoSkalY;	// Gewünschte Hoehe/Gesamtweite mal Pixelzahl = Pixelzahl
				if(  Scan.iH>65535l  )
					Scan.iH = 65535l;
				GetDlgItemText( hDlg, MESS_XOFF, str, 256 );
				Scan.iXOff = atof(str)*fSkal/fPiezoSkalX - (Scan.iW/2);	// Offset muss noch die halbe Scanweite abziehen, da linke obere Ecke
				if(  Scan.iXOff>32767  )
					Scan.iXOff = 32767;
				else if(  Scan.iXOff<-32767  )
					Scan.iXOff = -32767;
				GetDlgItemText( hDlg, MESS_YOFF, str, 256 );
				Scan.iYOff = atof(str)*fSkal/fPiezoSkalY - (Scan.iH/2);	// Offset muss noch die halbe Scanweite abziehen, da linke obere Ecke
				if(  Scan.iYOff>32767  )
					Scan.iYOff = 32767;
				else if(  Scan.iYOff<-32767  )
					Scan.iYOff = -32767;
				// Und nun die evt. veränderten Werte wieder setzen:
				if(  fSkal==1000.0  )
					SendMessage( hDlg, WM_COMMAND, HUB_MKM, BM_SETCHECK );
				else
					SendMessage( hDlg, WM_COMMAND, HUB_NM, BM_SETCHECK );

				pl = (LONG)hDlg;
				hwndScan = CreateWindowEx( WS_EX_NOPARENTNOTIFY, "MessBmpShow", "MSP", WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_VISIBLE, 
												CW_USEDEFAULT, CW_USEDEFAULT, Scan.iXPts+GetSystemMetrics(SM_CXFRAME), Scan.iYPts+70+GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION), 
												NULL, NULL, hInst, &pl );
			break;

			// invertiere Z-Kanal vom Piezo
			case MESS_SCAN_INVERT:
				PID.bFlags &= ~PIEZO_IS_INVERT;
				if(  IsDlgButtonChecked( hDlg, MESS_SCAN_INVERT )>0  )
					PID.bFlags |= PIEZO_IS_INVERT;
				CheckDlgButton( hDlg, MESS_SCAN_INVERT, (PID.bFlags&PIEZO_IS_INVERT)!=0 );
				DspInvertPiezo( (PID.bFlags&PIEZO_IS_INVERT)!=0 );
				break;

			// Exponentielle Regelung
			case MESS_EXPONENTIALLY:
				PID.bFlags &= ~EXPONENTIAL_PID;
				if(  IsDlgButtonChecked( hDlg, MESS_EXPONENTIALLY )>0  )
					PID.bFlags |= EXPONENTIAL_PID;
				CheckDlgButton( hDlg, MESS_EXPONENTIALLY, (PID.bFlags&EXPONENTIAL_PID)!=0 );
				DspExponentRegel( (PID.bFlags&EXPONENTIAL_PID)!=0 );
				break;

			// invertiere Z-Kanal vom Piezo
			case MESS_SCAN_FOREVER:
				bForeverScan = IsDlgButtonChecked( hDlg, MESS_SCAN_INVERT )==0;
				CheckDlgButton( hDlg, MESS_SCAN_FOREVER, bForeverScan );
			break;

				//Grobannäherung
			case MESS_PID_COARSEAPPROACH:
				if(CoarseApproachActive==0 && FineApproachActive==0) {
					//Zunächst ausfahren des Piezos wie bei MESS_PID_SEND
					GetDlgItemText( hDlg, MESS_PID_FREQ, str, 256 );
					PID.lFreq = (long)(atof(str)*1000.0);
					GetDlgItemText( hDlg, MESS_PID_SOLL, str, 256 );
					PID.lSoll = atol(str);
					GetDlgItemText( hDlg, MESS_PID_PROP, str, 256 );
					PID.lProp = (long)(atof(str)*1000.0);
					GetDlgItemText( hDlg, MESS_PID_INT, str, 256 );
					PID.lInt = (long)(atof(str)*1000.0);
					GetDlgItemText( hDlg, MESS_PID_DIFF, str, 256 );
					PID.lDiff = (long)(atof(str)*1000.0);
					DspPID( hDlg, &PID );
					//Button deaktivieren
					SetDlgItemText(hDlg,MESS_PID_COARSEAPPROACH,"Stop!");
					//Flag so setzen, dass der Windows-Timer zunächst das Ausfahren des Piezos abwartet
					FineApproachActive=1;
					iIst_alt=iError1;
			} else {
					CoarseApproachActive=0;
					FineApproachActive=0;
					SetDlgItemText(hDlg,MESS_PID_COARSEAPPROACH,"Coarse Approach");
				}


			break;
			
		}
		break;
	}
	return FALSE;
} // 20.9.97


/*************************************************************************/
// Fenster für Darstellung der Messwerte
// Werte"bitmap"
BYTE	pBmpSkala[256]=
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 
};


typedef struct
{
	HWND			hwndPID;
	// Die Daten
	LPBMPDATA	pBmp;
	LPUWORD		puTopo, puData2, pDispTopo;
	LONG			lTopoMax, lTopoMin;
	LONG			lData2Max, lData2Min;
	LONG			iWPts, iHPts;
	// Die Bitmaps
	LPUCHAR				puBmpTopo, puBmpData2;
	long					lBmpWidth, lBmpYOff, lYOff;
	long					lBmpMin, lBmpMax;
	LPBITMAPINFO	pBmpHdr;
} MESS_WINDOW_DATEN;


LRESULT CALLBACK MessBmpWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOLEAN		bMesseNichtJetzt;
	MESS_WINDOW_DATEN	*pMess;
	int	i;
	
	pMess = (MESS_WINDOW_DATEN *)GetWindowLong( hwnd, 0 );
	switch( uMsg )
	{
		case WM_CREATE:	
		{// Hier muss noch die Scan-Routine initialisiert werden ...
			LPBMPDATA					pBmp;
			SNOMPUTZ_SCANDATA	*pScn;
			LPBITMAPINFO			pBmpHdr;
			LPWORD						p=NULL;

			if(  (pBmp = (LPBMPDATA)pMalloc( sizeof(BMPDATA) ))==NULL  )
				DestroyWindow( hwnd );	// Zu wenig Speicher
			pMess = pMalloc( sizeof(MESS_WINDOW_DATEN) );
			SetWindowLong( hwnd, 0, (LONG)pMess );
			pMess->hwndPID = (HWND)*(long *)((LPCREATESTRUCT)lParam)->lpCreateParams;
			pScn = (SNOMPUTZ_SCANDATA *)pMalloc( sizeof(SNOMPUTZ_SCANDATA) );

			/* Scanparameter merken */
			pScn->fPid = PID.lProp/1000.0;
			pScn->fInt = PID.lInt/1000.0;
			pScn->fDiff = PID.lDiff/1000.0;
			pScn->lFreq = PID.lFreq;
			pScn->iSoll = PID.lSoll;
			pScn->lZyklus = Scan.lFreq;
			pScn->fW = Scan.iW*fPiezoSkalX;
			pScn->fH = Scan.iH*fPiezoSkalY;
			pScn->fX = Scan.iXOff*fPiezoSkalX;
			pScn->fY = Scan.iYOff*fPiezoSkalY;
			pMess->iWPts = pScn->iWPts = Scan.iXPts;
			pMess->iHPts = pScn->iHPts = Scan.iYPts;
			pScn->iVersatz = Scan.iOffset;
			pScn->iModus = Scan.iModus;
			pScn->iRichtung = Scan.iDir;
			pScn->fPiezoX = fPiezoSkalX;
			pScn->fPiezoY = fPiezoSkalY;
			pScn->fPiezoZ = fPiezoSkalZ;
			pBmp->pExtra = (LPUCHAR)pScn;
			pBmp->Typ = SNOMPUTZ;
			pBmp->lExtraLen = sizeof(SNOMPUTZ_SCANDATA);

			pBmp->Links = TOPO;
			pBmp->Rechts = NONE;
			pBmp->pSnom[0].fX = Scan.iW*fPiezoSkalX/Scan.iXPts;
			pBmp->pSnom[0].fY = fPiezoSkalY/65536.0/Scan.iYPts;
			pBmp->pSnom[0].Topo.fSkal = fPiezoSkalZ;
			pBmp->pSnom[0].Error.fSkal = fPiezoSkalZ;
			pBmp->pSnom[0].Lumi.fSkal = fIntens;
			wsprintf( pBmp->szName, STR_UNBENANNT, uUnbenannt++ );
			pMess->puTopo = pMalloc( Scan.iXPts*Scan.iYPts*sizeof(UWORD) );
			pMess->pDispTopo = pMalloc( Scan.iXPts*sizeof(UWORD) );
			if(  Scan.iModus&ERRO  )
			{
				pMess->puData2 = pMalloc( Scan.iXPts*Scan.iYPts*sizeof(UWORD) );
				pBmp->Rechts = ERRO;
			}
			if(  Scan.iModus&LUMI  )
			{
				pMess->puData2 = pMalloc( Scan.iXPts*Scan.iYPts*sizeof(UWORD) );
				pBmp->Rechts = LUMI;
			}
			pBmp->iSaved = -1;
			{
				SYSTEMTIME	st;
				GetLocalTime( &st );
				pBmp->iJahr = st.wYear;
				pBmp->iMonat = st.wMonth;
				pBmp->iTag = st.wDay;
				pBmp->iStunde = st.wHour;
				pBmp->iMinute = st.wMinute;
				pBmp->iSekunde = st.wSecond;
			}
			pBmp->pSnom[0].Topo.puDaten = NULL;
			pBmp->pSnom[0].Error.puDaten = NULL;
			pBmp->pSnom[0].Lumi.puDaten = NULL;
			pMess->pBmp = pBmp;

			/* Schließlich noch die Bitmap vorbereiten */
			pMess->lBmpWidth = ((Scan.iXPts+3)/4)*4;
			pMess->lBmpYOff = pMess->lBmpWidth*Scan.iYPts;
			pBmpHdr = (LPBITMAPINFO)pMalloc( sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256+sizeof(BYTE)*Scan.iXPts*Scan.iYPts );
			pBmpHdr->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pBmpHdr->bmiHeader.biWidth = Scan.iXPts;
			pBmpHdr->bmiHeader.biHeight = Scan.iYPts;
			pBmpHdr->bmiHeader.biPlanes = 1;
			pBmpHdr->bmiHeader.biBitCount = 8;
			pBmpHdr->bmiHeader.biCompression = BI_RGB;
			pBmpHdr->bmiHeader.biSizeImage = pMess->lBmpYOff;
			// Farben für Daten und Skala sind gleich!
			for(  i=0;  i<256;  i++  )
			{
				pBmpHdr->bmiColors[i].rgbRed = (BYTE)255-i;
				pBmpHdr->bmiColors[i].rgbGreen = (BYTE)255-i;
				pBmpHdr->bmiColors[i].rgbBlue = (BYTE)255-i;
			}
			pMess->pBmpHdr = pBmpHdr;
			pMess->puBmpTopo = (LPBITMAPINFO)(((LPBYTE)pBmpHdr)+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256);

			pMess->lBmpYOff -= pMess->lBmpWidth;
			pMess->lYOff = 0;
			pMess->lBmpMin = 0;
			pMess->lBmpMax = 1;
			pMess->lTopoMin = +1000000l;
			pMess->lTopoMax = -1000000l;
			pMess->lData2Min = +1000000l;
			pMess->lData2Max = -1000000l;

			// Scannen starten
			DspScan( hwnd, pMess->puTopo, pMess->puData2, &Scan );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)GetStringRsc(STR_SCANNING) );
			bMesseNichtJetzt = FALSE;
		}
		break;

		// Frische neue Daten sind eingetroffen ...
		case WM_SCANLINE_READY:
		if(  !bMesseNichtJetzt  )
		{
			HDC		hdc;
			LONG	x, offset=(pMess->iWPts*wParam);
			char	str[128];
			double	pf[2];

			pMess->lYOff = wParam;
			OsziNum[0] = pMess->iWPts;
			OsziNum[2] = OsziNum[1] = 0;
			for(  i=0;  i<pMess->iWPts;  i++  )
				pMess->pDispTopo[i] = ((signed short)pMess->puTopo[i+offset])/2+16384;
			if(  MittelZeile( pMess->pDispTopo, NULL, pMess->iWPts, pf, 2 )  )
			{
				LONG		lMW=(LONG)((pf[0]+pf[1])/2.0+0.5), iOffset, lThisTopoMax, lThisTopoMin;
				double	fSteigung=2.0*(pf[1]-pf[0])/(double)pMess->iWPts, fScale, fFullScale;

				lThisTopoMax = -200000000;
				lThisTopoMin = 200000000;
				for(  x=0;  x<pMess->iWPts;  x++  )
				{
					i = (short)pMess->pDispTopo[x];
					i = ((LONG)(ULONG)i - (LONG)(x*fSteigung) - lMW + 32768)/2;
					OsziDaten[0][x] = i;
					if(  i<lThisTopoMin  )
						lThisTopoMin = i;
					if(  i>lThisTopoMax  )
						lThisTopoMax = i;
					if(  i<pMess->lTopoMin  )
						pMess->lTopoMin = i;
					if(  i>pMess->lTopoMax  )
						pMess->lTopoMax = i;
					if(pMess->lBmpMax-pMess->lBmpMin==0)
						pMess->lBmpMax = pMess->lBmpMin+1;
					i =	((i-pMess->lBmpMin)*256)/(pMess->lBmpMax-pMess->lBmpMin);
					if(  i>255  )
						pMess->puBmpTopo[pMess->lBmpYOff+x] = 255;
					else if(  i<0  )
						pMess->puBmpTopo[pMess->lBmpYOff+x] = 0;
					else
						pMess->puBmpTopo[pMess->lBmpYOff+x] = i;
				}
				// elaborate scale
				fFullScale = fPiezoSkalZ*(double)(lThisTopoMax-lThisTopoMin)*2.0;
				fScale = (int)(log(fFullScale)/log(10.0));
				fScale = pow( 10.0, fScale );
				// now we have a 10^x of the scale:
				// get the fine rannge
				fScale *= (int)(fFullScale/fScale);
				iOffset = fScale/2.0/fPiezoSkalZ;
				iOffset -= (lThisTopoMax-lThisTopoMin)/2;
				OsziMin[0] = lThisTopoMin-iOffset;
				OsziMax[0] = lThisTopoMax+iOffset;
				OsziNum[0] = pMess->iWPts;
				sprintf( OsziStr[0], "%.1f nm", fScale, OsziStr[0] );
				OsziNum[1] = OsziNum[2] = 0;
				OsziStr[1][0] = OsziStr[2][0] = 0;

				// Auch noch andere Daten ins Oszi zu packen?
				if(  pMess->puData2  )
				{
					int	m;

					if(  pMess->pBmp->Rechts==ERRO  )
						m = 2;
					else
						m = 1;	// Lumi

					for(  x=0;  x<pMess->iWPts;  x++  )
					{
						i = (short)pMess->puData2[offset+x];
						OsziDaten[m][x] = i;
						if(  i<pMess->lData2Min  )
							pMess->lData2Min = i;
						if(  i>pMess->lData2Max  )
							pMess->lData2Max = i;
						OsziNum[m] = pMess->iWPts;
						OsziMin[m] = pMess->lData2Min;
						OsziMax[m] = pMess->lData2Max;
					}
				}
				RedrawOszi( pMess->hwndPID );
				pMess->lBmpYOff -= pMess->lBmpWidth;
				pMess->lYOff ++;

				i = (long)( 2.0*(double)pMess->lBmpYOff*(double)Scan.lFreq/(double)PID.lFreq+1.5 );
				sprintf( str, STR_MITTEL_ZEIT, (long)((pf[0]+pf[1])/2.0+0.5), i/60, i%60 );
				SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)str );

				/* Die erste fünf Zeilen wird automatisch neu skaliert! */
				if(  pMess->lYOff<5  )
				{
					pMess->lBmpMin = pMess->lTopoMin;
					pMess->lBmpMax = pMess->lTopoMax;
					InvalidateRect( hwnd, NULL, TRUE );
				}
				else
				{
					/* Neuzeichen */
					hdc = GetDC( hwnd );
					SetStretchBltMode( hdc, COLORONCOLOR );
					StretchDIBits (hdc, 0, 70, pMess->iWPts, pMess->iHPts, 
															0, 0, pMess->iWPts, pMess->iHPts, 
															(LPSTR) pMess->puBmpTopo,
															pMess->pBmpHdr,
															DIB_RGB_COLORS, SRCCOPY);
					ReleaseDC( hwnd, hdc );
				}
				// evt. Speichern und sichern auslösen?
				if(  pMess->lYOff>=pMess->iHPts  )
					DestroyWindow( hwnd );
			}
		}
		break;

		case WM_PAINT:
		{
			HDC		hDC;
			PAINTSTRUCT	ps;
				
			hDC = BeginPaint (hwnd, &ps) ;

			// Erst Skala
			pMess->pBmpHdr->bmiHeader.biWidth = 256;
			pMess->pBmpHdr->bmiHeader.biHeight = 1;
			StretchDIBits (hDC, 5, 2, pMess->iWPts-5, 16, 
													0, 0, 255, 1,
													pBmpSkala,
													pMess->pBmpHdr,
													DIB_RGB_COLORS, SRCCOPY) ;
			SelectObject( hDC, GetStockObject( BLACK_PEN ) );
			DrawHorizontalAxis( hDC, 5, 18, pMess->iWPts-5, 3, 5, (pMess->lBmpMax-pMess->lBmpMin)*fPiezoSkalZ, 0.0, STR_TOPO_UNIT, NULL );

			// Dann Daten ...
			pMess->pBmpHdr->bmiHeader.biWidth = pMess->iWPts;
			pMess->pBmpHdr->bmiHeader.biHeight = pMess->iHPts;
			SetStretchBltMode( hDC, COLORONCOLOR );
			StretchDIBits (hDC, 0, 70, pMess->iWPts, pMess->iHPts, 
													0, 0, pMess->iWPts, pMess->iHPts, 
													(LPSTR) pMess->puBmpTopo,
													pMess->pBmpHdr,
													DIB_RGB_COLORS, SRCCOPY);
			EndPaint( hwnd, &ps );
		}
		break;

		case WM_DESTROY:
		{
			MDICREATESTRUCT	mdicreate;

			bMesseNichtJetzt = TRUE;
			if(  pMess->lYOff<pMess->iHPts  )				// Vorheriger Abbruch -> Stopp!
				DspRetract();
			KillTimer( hwnd, 14 );
			MemFree( pMess->pBmpHdr );
			if(  pMess->lYOff==0  )
			{
				MemFree( pMess->pBmp->pExtra );
				MemFree( pMess->pBmp );
				MemFree( pMess->puTopo );
				MemFree( pMess->puData2 );
				return 0;
			}
			LadeBlock( pMess->pBmp, pMess->puTopo, pMess->iWPts, pMess->iWPts, pMess->lYOff, 16, TOPO, 32768, TRUE );
			MemFree( pMess->puTopo );
			if(  (Scan.iModus&~TOPO)!=0  )
			{
				LadeBlock( pMess->pBmp, pMess->puData2, pMess->iWPts, pMess->iWPts, pMess->lYOff, 16, Scan.iModus&~TOPO, 32768, TRUE );
				MemFree( pMess->puData2 );
			}
			mdicreate.szClass = (LPSTR)szBmpClass ;
			mdicreate.szTitle = pMess->pBmp->szName;
			mdicreate.hOwner  = hInst;
			mdicreate.x       = CW_USEDEFAULT;
			mdicreate.y       = CW_USEDEFAULT;
			mdicreate.cx      = CW_USEDEFAULT;
			mdicreate.cy      = CW_USEDEFAULT;
			mdicreate.style   = WS_CHILD|WS_BORDER;
			mdicreate.lParam  = (LONG)(LPBMPDATA)pMess->pBmp;
			SendMessage (hwndClient, WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT) &mdicreate) ;
			bMesseNichtJetzt = FALSE;
			if(  pMess->lYOff>=pMess->iHPts  &&				// Vorheriger Abbruch -> Stopp!
				   bForeverScan  )
				// Sonst einfach nächstes Bild
				PostMessage( pMess->hwndPID, WM_COMMAND, MESS_SCAN_SEND, 0 );
			MemFree( pMess );
			pMess = 0;
		}
		break;

		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
	return 0;
}

/*************************************************************************/

LRESULT CALLBACK MessSeriellDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT		idTimer;
	static LONG	pos, line;
	static BOOL TERMINAL, bTimer;

	HWND	h;
	BYTE		str[256];
	int			i;

	switch( message )
	{
		//case WM_CREATE:
		case WM_INITDIALOG:
		{
			if(  DspThreadHandle==NULL  )
			{
				CreateThread( NULL, 0, DspThread, (LPVOID)hDlg, 0, &DspThreadHandle );
				hDspSemaphore = CreateSemaphore( NULL, 1, 1, DSP_CMD_SEMAPHORE );
			}
			h = GetDlgItem(hDlg,MESS_SER_PORT);
			SendMessage( h, CB_RESETCONTENT, 0, 0 );
			SendMessage( h, CB_ADDSTRING, 0, (LONG)(LPSTR)"NONE" );
			// Maximale Anzahl von Com-Ports abfragen
			for(  i=1;  i<9;  i++  )
			{
				wsprintf( str, "COM%i", i );
#ifdef BIT32
				hComm = CreateFile( str, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
#else
				hComm = OpenComm( str, 4096, 128 );
#endif
				if(  hComm!=INVALID_HANDLE_VALUE  )
				{
					SendMessage( h, CB_ADDSTRING, 0, (LONG)(LPSTR)str );
#ifdef BIT32
					CloseHandle( hComm );
#else
					CloseComm( hComm );
#endif
				}
			}
			SendMessage( h, CB_SETCURSEL, 0, 0 );
			SetFocus( h );
			TERMINAL = TRUE;
			h = GetDlgItem(hDlg,MESS_SER_TERMINAL);
			pos = LOWORD( SendMessage( h, EM_GETSEL, 0, 0L ) );
			line	= SendMessage( h, EM_LINEFROMCHAR, -1, 0l );
			idTimer = SetTimer( hDlg, 13, 250, NULL );
			bTimer = FALSE;
		}
		break;

		case WM_DESTROY:
			SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_SERIAL, FALSE );
			KillTimer( hDlg, idTimer );
			idTimer = 0L;
		break;

		case WM_TIMER:
		// Alle 1000ms Update auf der Com ...
		if(  hComm!=INVALID_HANDLE_VALUE  &&  !bTimer  )
		{
#ifndef BIT32
			COMSTAT	cmst;
#endif
			BYTE	str[2];

			bTimer = TRUE;
			h = GetDlgItem(hDlg,MESS_SER_TERMINAL);
#ifdef BIT32
			TERMINAL = FALSE;
			while(  ReadFile( hComm, str, 1, &i, NULL )  &&  i>0  )
			{
				if(  str[0]!=13  )
					SendMessage( h, WM_CHAR, str[0], 0 );
				else if(  str[0]!=10  )
					SendMessage( h, WM_CHAR, 13, 0 );
			}
			pos = LOWORD( SendMessage( h, EM_GETSEL, 0, 0L ) );
			line	= SendMessage( h, EM_LINEFROMCHAR, -1, 0l );
			TERMINAL = TRUE;
#else
			GetCommError( ComHdl, &cmst );
			// Wenn Zeichen da, dann ausgeben!
			if(  cmst.cbInQue>0  &&  TERMINAL  )
			{
				int		status;

				TERMINAL = FALSE;
				status = TRUE;
				line = cmst.cbInQue;
				while(  line-->0  &&  status>0  )
				{
					status = ReadComm( ComHdl, str, 1 );
					if(  str[0]!=13  &&  status!=0  )
						SendMessage( h, WM_CHAR, str[0], 0 );
				}
				pos = LOWORD( SendMessage( h, EM_GETSEL, 0, 0L ) );
				line	= SendMessage( h, EM_LINEFROMCHAR, -1, 0l );
				TERMINAL = TRUE;
			}
#endif
			bTimer = FALSE;
		}
		break;

		case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
			case MESS_SER_TERMINAL:
				h = GetDlgItem( hDlg, MESS_SER_TERMINAL );
#ifdef BIT32
				if(  HIWORD(lParam)==EN_UPDATE  &&  TERMINAL  )
#else
				if(  HIWORD(wParam)==EN_UPDATE  &&  TERMINAL  )
#endif
				{
					LONG	neu_pos = LOWORD( SendMessage( h, EM_GETSEL, 0, 0L ) );
					LONG	neu_line	= SendMessage( h, EM_LINEFROMCHAR, -1, 0l );
					signed	len=0, i;
					long		j;
																				 
					if(  neu_line==line  )
					{
						if(  neu_pos-pos<0  )
						{
							for(  i=0;  i<pos-neu_pos;  i++  )
								str[i] = 8;	// Backspace
							str[i] = 0;
							len = i;
						}
						else
						{
							*((WORD *)str) = (WORD)neu_pos;
							len = (WORD)SendMessage( h, EM_GETLINE, (WPARAM)line, (LPARAM)(LPSTR)str );
							str[len] = 0;
							i = (WORD)SendMessage( h, EM_LINEINDEX, -1, 0L );
							lstrcpy( str, str+pos-i );
							len -= (WORD)(pos-i);
						}
					}
					else
					{
						if(  neu_line>line  )
						{
							str[0] = 13;
							len = 1;
						}
						pos = 0;
					}

					if(  hComm>0  )
					{
						TERMINAL = FALSE;
						for(  i=0;  i<len;  i++  )
							TransmitCommChar( hComm, str[i] );
						TERMINAL = TRUE;
					}
					pos = LOWORD( SendMessage( h, EM_GETSEL, 0, 0L ) );
					line	= SendMessage( h, EM_LINEFROMCHAR, -1, 0l );
				}
			break;

			case MESS_SER_PORT:
				// Port öffnen und initialisieren
				h = GetDlgItem(hDlg,MESS_SER_PORT);
#ifdef BIT32
				if(  HIWORD(wParam)==CBN_SELCHANGE  )
#else
				if(  HIWORD(lParam)==CBN_SELCHANGE  )
#endif
				{
					if(  hComm>0  )
#ifdef BIT32
						CloseHandle( hComm );
#else
						CloseComm( hComm );
#endif
					i = (WORD)SendMessage( h, CB_GETCURSEL, 0, 0l );
					iComNr = -1;
					if(  i>0  )
					{
						COMMTIMEOUTS	ComTOS={ 50, 50, 100, 150, 1000 };

						SetDlgItemText( hDlg, MESS_SER_TERMINAL, "" );
						SendMessage( h, CB_GETLBTEXT, i, (LPARAM)(LPSTR)str );
#ifdef BIT32
						hComm = CreateFile( str, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
						if(  hComm<=0  )
						{
							hComm = INVALID_HANDLE_VALUE;
							MessageBox( hDlg, GetStringRsc(E_COM), NULL, MB_OK );
							break;
						}
						lstrcat(str,":9600,e,7,1");
						iComNr = str[3]-'0';
						BuildCommDCB( str, &ComDCB );
						ComDCB.fNull = FALSE;
						ComDCB.fBinary = TRUE;
						ComDCB.fOutxCtsFlow = TRUE;
						ComDCB.fOutxDsrFlow = TRUE;
						ComDCB.fParity = TRUE;	// Even Parity
						ComDCB.Parity = 2;
						ComDCB.fDtrControl = DTR_CONTROL_HANDSHAKE;
						ComDCB.fRtsControl = RTS_CONTROL_TOGGLE;
						ComDCB.fTXContinueOnXoff = TRUE;
						ComDCB.fOutX = FALSE;
						ComDCB.fInX = FALSE;
						bTimer = TRUE;
						SetupComm( hComm, 14, 16 );
						ClearCommBreak( hComm );
						SetCommTimeouts( hComm, &ComTOS );
						SetCommState( hComm, &ComDCB);
					  WriteFile( hComm, "id\015\012", 4, &i, NULL );
						if(  ReadFile( hComm, str, 10, &i, NULL )  &&  i>0  )
						{
							iLockIn = atoi(str);
							WriteFile( hComm, "imode 1\015\021",	// Stromeingang unempfinglich (Hohe Bandbreite)
																8, &i, NULL );
						}
						bTimer = FALSE;
#else
						hComm = OpenComm( str, 4096, 128 );
						lstrcat(str,":9600,e,7,1");
						if(  hComm<=0  )
						{
							hComm = 0;
		MessageBox( hDlg, GetStringRsc(E_COM), NULL, MB_OK );
							break;
						}
						iComNr = str[3]-'0';
						BuildCommDCB( str, &ComDCB );
						ComDCB.fNull = FALSE;
						ComDCB.fBinary = TRUE;
						ComDCB.fRtsDisable = FALSE;
						ComDCB.fOutxCtsFlow = TRUE;
						SetCommState(&ComDCB);
						WriteComm( ComHdl, "id\x0D\x0A", 4 );
#endif
						// Init-String
					}
				}
			break;

			case MESS_SER_PAR:
				if(  iComNr>0  )
				{
					COMMCONFIG ccSer;

					hmemcpy( &(ccSer.dcb), &ComDCB, sizeof(DCB) );
					wsprintf( str, "COM%i", iComNr );
					if(  CommConfigDialog( str, hDlg, &ccSer )  )
					{
						hmemcpy( &ComDCB, &(ccSer.dcb), sizeof(DCB) );
						ClearCommBreak( hComm );
						SetCommState( hComm, &ComDCB);
					  WriteFile( hComm, "id\015\012", 4, &i, NULL );
					}
				}
			break;
		}
		break;
	}
	return FALSE;
}
// 26.8.97

/*************************************************************************/

#define	MESS_FREQ_PUNKTE	50

WORD	uAmplitude[MESS_FREQ_PUNKTE];

LRESULT CALLBACK MessFreqDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT		idTimer;
	HWND	h;
	BYTE		str[256];
	int			i;
	long		l;
	static int		iPos;
	static double	fStart, fDist;

	switch( message )
	{
		//case WM_CREATE:
		case WM_INITDIALOG:
		{
			fStart = fOszillatorFrequenz;
			sprintf( str, "%.2f", fOszillatorFrequenz );
			SetDlgItemText( hDlg, MESS_FREQ_MITTE, str );
			fDist = 1000.0;
			sprintf( str, "%.1f", 1000.0 );
			SetDlgItemText( hDlg, MESS_FREQ_WEITE, str );
			sprintf( str, "%.3f", fOszillatorAmplitude );
			SetDlgItemText( hDlg, MESS_FREQ_AMPLITUDE, str );
			if(  hComm==INVALID_HANDLE_VALUE  &&  iLockIn==0  )
				EndDialog( hDlg, FALSE );
			OsziNum[0] = OsziNum[1] = OsziNum[2] = OsziNum[3] = 0;
			OsziMin[0] = 0;
			OsziMax[0] = 1;
		}
		break;

		case WM_DESTROY:
			SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_FREQ, FALSE );
			KillTimer( hDlg, idTimer );
			idTimer = 0;
		break;

		case WM_TIMER:
		if(  hComm!=INVALID_HANDLE_VALUE  &&  iPos<MESS_FREQ_PUNKTE  )
		{
			sprintf( str, "%.2f Hz", fStart+(fDist*iPos) );
			SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)str );
			l = (fStart+(fDist*(double)iPos))*1000.0;
			sprintf( str, "OF %ld", l );
			WriteFile( hComm, str, lstrlen(str), &l, NULL );
			WriteFile( hComm, "\015X\015", 3, &l, NULL );
			ReadFile( hComm, str, 16, &l, NULL );
			str[l+1] = 0;
			i = atoi( str );
			OsziDaten[0][iPos] = uAmplitude[iPos] = i;
			if(  i<OsziMin[0]  )
				OsziMin[0] = i;
			if(  i>OsziMax[0]  )
				OsziMax[0] = i;
			iPos ++;
			OsziNum[0] = iPos;
			if(  iPos%5==0  )
				RedrawOszi( hDlg );

			// Nach hundert Punkten => Auswertung
			if(  iPos==MESS_FREQ_PUNKTE  )
			{
				KillTimer( hDlg, idTimer );
				idTimer = 0;
				// Maximum finden
				for(  i=0;  i<MESS_FREQ_PUNKTE;  i++  )
					if(  uAmplitude[i]>uAmplitude[iPos]  )
						iPos = i;
				// Und einstellen
				fOszillatorFrequenz = fStart += fDist*iPos;
				sprintf( str, "%.2f", fStart );
				SetDlgItemText( hDlg, MESS_FREQ_MITTE, str );
				sprintf( str, "%.2f", fDist*(float)MESS_FREQ_PUNKTE/10.0 );
				SetDlgItemText( hDlg, MESS_FREQ_WEITE, str );
				sprintf( str, GetStringRsc(STR_NEW_F), fStart );
				SendMessage( hwndMessInfo, SB_SETTEXT, 0, (LPARAM)str );
				// Erneut scannen?
				l = fStart*1000.0;
				sprintf( str, "OF %ld", l );
				WriteFile( hComm, str, lstrlen(str), &l, NULL );
				WriteFile( hComm, "\015", 1, &l, NULL );
				iPos = MESS_FREQ_PUNKTE;
				RedrawOszi( hDlg );
				WriteFile( hComm, "AQN\015", 4, &i, NULL );
			}
		}
		break;

		case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
			case MESS_OSZI:
			{
				double f=fStart-fDist*(500-(double)lParam)/500.0;
				if(  (long)(fOszillatorFrequenz*100)!=(long)(f*100)  &&  iPos==MESS_FREQ_PUNKTE  )
				{
					fOszillatorFrequenz = f;
					sprintf( str, "%.2f", fOszillatorFrequenz );
					SetDlgItemText( hDlg, MESS_FREQ_MITTE, str );

					sprintf( str, "OF %li", (long)(f*1000.0) );
					WriteFile( hComm, str, lstrlen(str), &l, NULL );
					WriteFile( hComm, "\015", 1, &l, NULL );
				}
			}
			break;

			case MESS_ABORT:
			{
				KillTimer( hDlg, idTimer );
				idTimer = 0;
				RedrawOszi( hDlg );
			}
			break;

			case MESS_FREQ_PHASE:
			if(  hComm!=INVALID_HANDLE_VALUE  )
			{
				// Autophase
				WriteFile( hComm, "AQN\015", 4, &i, NULL );

				// Oszillator einstellen
				GetDlgItemText( hDlg, MESS_FREQ_AMPLITUDE, str, 256 );
				fOszillatorAmplitude = atof( str );
				sprintf( str, "OA. %f", fOszillatorAmplitude );
				WriteFile( hComm, str, lstrlen(str), &l, NULL );
				WriteFile( hComm, "\015", 1, &l, NULL );

			}
			break;

			case MESS_SCAN_SEND:
			if(  hComm!=INVALID_HANDLE_VALUE  )
			{
				OsziMin[0] = 32767;
				OsziMax[0] = 0;

				// Oszillator einstellen
				GetDlgItemText( hDlg, MESS_FREQ_AMPLITUDE, str, 256 );
				fOszillatorAmplitude = atof( str );
				sprintf( str, "OA. %f", fOszillatorAmplitude );
				WriteFile( hComm, str, lstrlen(str), &l, NULL );
				WriteFile( hComm, "\015", 1, &l, NULL );

				// Oszillator setzen auf Start
				GetDlgItemText( hDlg, MESS_FREQ_WEITE, str, 256 );
				fDist = atof( str );
				GetDlgItemText( hDlg, MESS_FREQ_MITTE, str, 256 );
				fStart = atof( str )-fDist;
				fDist = (fDist*2.0)/(double)MESS_FREQ_PUNKTE;
				sprintf( str, "OF %li", (long)(fStart*1000.0) );
				WriteFile( hComm, str, lstrlen(str), &l, NULL );
				WriteFile( hComm, "\015", 1, &l, NULL );

				// Alle hundert Millisekunden was Neues ...
				iPos = 0;
				idTimer = SetTimer( hDlg, 13, 100, NULL );
			}
			break;
		}
		break;
	}
	return FALSE;
}
// 26.8.97




/*************************************************************************/

// Identifier für Tool/Statusbar
#define ID_MESSBAR	19
#define ID_MESSSTATUSBAR 20

#define iMaxScan 7

TBBUTTON pScanbarButtons[]=
{
	{ 0, IDM_MESS_SERIAL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 1, IDM_MESS_FREQ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 3, IDM_MESS_APPROACH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
	{ 2, IDM_MESS_PID, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },
#if 0
	{ 4, IDM_MESS_MESS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 }
	{ 5, IDM_MESS_OFF, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 }
#endif
};
// 4.1.99


// Routine, die die einzelnen Unterroutinen aufruft
LRESULT CALLBACK MessWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	static HWND	hwndCreateParent;	// Handle für die Erzeugung neuer Fenster ...
	RECT xywh;
	static int iYOffset;

	
	switch (message)
	{
		case WM_CREATE:
		{
			int		w, h;
			RECT	xywh;

			hwndCreateParent = (HWND)lParam;
			hwndMessBar = CreateToolbarEx( hwnd, WS_CHILD|WS_VISIBLE, ID_MESSBAR, iMaxScan, hInst, SCANBARBMP, pScanbarButtons, iMaxScan, 16, 16, 16, 16, sizeof(TBBUTTON) );
			//|TBSTYLE_TOOLTIPS
			hwndMessInfo = CreateStatusWindow( WS_CHILD|WS_BORDER|WS_VISIBLE, NULL, hwnd, ID_MESSSTATUSBAR);
			iOn = IDM_MESS_SERIAL;
			SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_SERIAL, TRUE );
			hModeLess = CreateDialog( hInst, "MessSerial", hwnd, MessSeriellDialog );
			SendMessage( hwndToolbar, TB_PRESSBUTTON, IDM_MESS, TRUE );
			SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MESS, FALSE );
			EnableMenuItem (hMenuInit, IDM_MESS, MF_DISABLED|MF_GRAYED);
			EnableMenuItem (hMenuBmp, IDM_MESS, MF_DISABLED|MF_GRAYED);

			GetWindowRect( hwnd, &xywh );
			w = xywh.right-xywh.left;
			h = xywh.bottom-xywh.top;
			GetClientRect( hwnd, &xywh );
			w -= xywh.right;
			h -= xywh.bottom;
			GetWindowRect( hModeLess, &xywh );
			w += xywh.right-xywh.left;
			h += xywh.bottom-xywh.top;
			GetWindowRect( hwndMessInfo, &xywh );
			h += xywh.bottom-xywh.top;
			GetWindowRect( hwndMessBar, &xywh );
			iYOffset = xywh.bottom-xywh.top;
			h += xywh.bottom-xywh.top;
			SetWindowPos( hwnd, NULL, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER );
			SendMessage( hwndMessBar, WM_SIZE, SIZE_RESTORED, MAKELONG(w,h) );
			SendMessage( hwndMessInfo, WM_SIZE, SIZE_RESTORED, MAKELONG(w,h) );
			SetWindowPos( hModeLess, NULL, 0, iYOffset, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
		}
		break;

		case WM_DESTROY:
		{
			DestroyWindow( hModeLess );
			hModeLess = NULL;
			if(  hComm!=INVALID_HANDLE_VALUE  )
#ifdef BIT32
				CloseHandle( hComm );
#else
				CloseComm( hComm );
#endif
			hComm = INVALID_HANDLE_VALUE;
			CloseHandle( hDspSemaphore );
			DspRetract();
			SendMessage( hwndToolbar, TB_PRESSBUTTON, IDM_MESS, FALSE );
			SendMessage( hwndToolbar, TB_ENABLEBUTTON, IDM_MESS, TRUE );
			EnableMenuItem( hMenuInit, IDM_MESS, MF_ENABLED );
			EnableMenuItem( hMenuBmp, IDM_MESS, MF_ENABLED );
			DspThreadHandle = NULL;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD(wParam) )
			{
				case IDM_MESS_SERIAL:
					if(  IDM_MESS_SERIAL!=iOn  )
					{
						DestroyWindow( hModeLess );
						hModeLess = hModeLess = CreateDialog( hInst, "MessSerial", hwnd, MessSeriellDialog );
						SetWindowPos( hModeLess, NULL, 0, iYOffset, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
						iOn = IDM_MESS_SERIAL;
					}
					SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_SERIAL, TRUE );
					break;

				case IDM_MESS_APPROACH:
					if(  IDM_MESS_APPROACH!=iOn  )
					{
						DestroyWindow( hModeLess );
						hModeLess = CreateDialog( hInst, "MessApproach", hwnd, MessApproachDialog );
						SetWindowPos( hModeLess, NULL, 0, iYOffset, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
						iOn = IDM_MESS_APPROACH;
					}
					SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_APPROACH, TRUE );
					break;

				case IDM_MESS_FREQ:
					if(  IDM_MESS_FREQ!=iOn  )
					{
						DestroyWindow( hModeLess );
						hModeLess = CreateDialog( hInst, "MessFreq", hwnd, MessFreqDialog );
						SetWindowPos( hModeLess, NULL, 0, iYOffset, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
						iOn = IDM_MESS_FREQ;
					}
					SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_FREQ, TRUE );
					break;

				case IDM_MESS_PID:
					if(  IDM_MESS_PID!=iOn  )
					{	
						DestroyWindow( hModeLess );
						hModeLess = CreateDialog( hInst, "MessPID", hwnd, MessPIDDialog );
						SetWindowPos( hModeLess, NULL, 0, iYOffset, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
						iOn = IDM_MESS_PID;
					}
					SendMessage( hwndMessBar, TB_PRESSBUTTON, IDM_MESS_PID, TRUE );
					break;

			}
		}
		break;
	}

	// Weitergabe an DefFrameProc (ersetzt DefWindowProc)
	return DefWindowProc(hwnd, message, wParam, lParam) ;
}
// 4.1.99





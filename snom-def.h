
// Service ...
#ifndef  __SNOM_DEF
#define  __SNOM_DEF

#define AUTOKORR 1
#define FFT			 2
#define PSD      3
#define DIFF		 4

#define WM_RECALC_SLIDER	(WM_USER+17)

//	Maximale Höhe eines "3D-Pixels"
#define		MAX_3D_HOEHE	200.0

// Maximale Anzahl Bilder (muss kleiner 256 sein!)
#define MAX_SNOM 128

// Unterschiedliche Behandlung von WM_SCROLL!
#ifndef	BIT32
#define	GET_SCROLL_HANDLE(lParam)  (HIWORD((lParam)))
#else
#define	GET_SCROLL_HANDLE(lParam)  ((HANDLE)(lParam))
#endif

/* Anzahl Spalten in der Matrix für die ScanLine
#define SCANDATENANZAHL	9*/

#endif

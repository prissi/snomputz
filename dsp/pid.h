/****************************************************************************************
 ****	Kompletter PID-Algorithmus, einschließlich echter Integration                ******
 ****************************************************************************************/

#include <periph.h>
#include <analog.h> 
#include <math.h> 


#ifndef	_PIDPAR
#include "scantyp.h"
#endif

// Mailboxnummer
#define SNOMPUTZ_MB	0

#define PID_TIMER 0
#define PID_INTERRUPT 8
#define SCAN_TIMER 1
#define SCAN_INTERRUPT 9

#define LUMI_AD			3
#define DAEMPFUNG_AD	0

#define ADC2ISTWERT(x) (x)
/*#define ADC2ISTWERT(x) ((int)(log((float)x+32768.0)*5900.0)+32768u)*/


#define	Z_DA			0
#define X_DA			1
#define Y_DA			2
#define WALK_DA			3

/****************************************************************************************
 * Öffentliche Routinen */



/* Wartet iMs Millisekunden */
/* DARF NICHT AUS INTERRUPT AUFGERUFEN WERDEN !!!*/
void	wait( float iMs );
                               
/* SOnst gehen alle Timer-Routinen nicht mehr */                               
void	InitPIDInt( float fFreq );

/* Parameter sind:
 * float fSoll				// Sollwert (eigentlich ein unsigned ...)
 * float fProp				// Proportionalbereich (5% ... 300%)
 * float fInt				// Zeit über die integriert wird "Nachstellzeit" (unendlich  ... 0.00001)
 * float fDiff				// Steilheit der Ableitung "Vorstellzeit" (0s ... unendlich)
 * float fAbtastFrequnenz	// Abtastfrequenz
 */
int	InitPID( long lFreq, long iSoll, long lProp, long lInt, long lDiff );

/* Scan muss z-channel inertieren, alter Wert zurueck */
int	invert_piezo(int bInvert);

/* Bricht einen Scan in jedem Fall ab */
void AbortScan(void);
/* 3.11.98 */


/* Scankopf nach x,y; ACHTUNG: Bricht Scan ab! */
int	InitGo( GOPAR *pGo );
/* 18.4.99 */

/* Bereitet alles für die Bildaufnahme vor */
int	InitScan( SCANPAR *pScan );
/* 3.11.98 */


/****************************************************************************************
 * Private Routinen */

#define PIEZO_LIMIT	 32767

/* Grenzen des Regelbereiches */
/* -32767*16 */
#define MIN_REGEL	((-PIEZO_LIMIT)*16)
/* 32767*16 */
#define MAX_REGEL	((PIEZO_LIMIT)*16)

/*
 *	This handler is invoked whenever TIM0 expires.
 *	and handels the PID
 */               
void do_pid(void);

/* Macht eine echte PID-Regelung (kein Geschwindigkeitsalgorithmus)
 * Rückgabewert ist der neue Ausgangswert
 */
signed	PID( long lIst );





/* Scan of the image
 */  
void do_go(void);
/* 3.11.98 */

void do_scan(void);
/* 3.11.98 */

void do_sps(void);
/* 3.11.98 */

/* Setzt DA-Position */
void set_pos( void );
/* 3.11.98 */

/* Setzt DAC */                                  
void set_dac(unsigned int channel, long wert); 

/* Liest DAC aus (d.h. letzten Wert ermitteln) */
short get_dac(unsigned int channel);

/* Liest ADC aus */
short get_adc(unsigned int channel); 

/* Wartet iMs Millisekunden */
/* DARF NICHT AUS INTERRUPT AUFGERUFEN WERDEN !!!*/
inline void	wait( float fWaitMs );



/****************************************************************************************
****	Grobapproach
****************************************************************************************/




#include <periph.h>
#include <analog.h>

#include "approach.h"
#include "scantyp.h"
#include "pid.h"


#define DIO	((int *)0x818000) 	/* Digital I/O */

/* Hilfsvariablen (aktueller Wert des DAC) mal 16 (wegen Rechnefehlerminimierung) */
extern volatile long	iRegel;
       
/* Anzeige, ob regelt ... */
extern volatile int	bDoPID, bDoScan, bDoGo;
extern volatile int	iXPosition, iYPosition;

/***************************************************************************************/
/* Zuerst die Feinarbeit */
                          
/* N‰hert die Spitze langsam an */
int	Approach( int lSollwert  )
{
	bDoPID = FALSE;
	bDoScan = FALSE;
	bDoGo = FALSE;
	if(  iRegel<MIN_REGEL  )
		iRegel = MIN_REGEL;
  while( abs(get_dac( DAEMPFUNG_AD )-lSollwert) > 10  )
	{
		iRegel += 5;
		if(  iRegel>MAX_REGEL  )
			return FALSE;
		set_dac(Z_DA, (iRegel>>4) );
	}
  return TRUE;
}
/* 24.10.98 */

  

/* Zieht die Spitze langsam zur¸ck und beendet die PID-Regelung */
void	Retract(  void  )
{
	/* PID sperren ... */
	bDoPID = FALSE;
	bDoScan = FALSE;
	bDoGo = FALSE;
	while(  iRegel>MIN_REGEL  )
	{
		set_dac(Z_DA,(iRegel>>4));
		wait(1.0);
		iRegel -= 1600;
	}
	iRegel = MIN_REGEL;
	set_dac(Z_DA,(iRegel>>4));
 	iXPosition = iYPosition = 0;
 	set_dac(X_DA,0);
 	set_dac(Y_DA,0);
}
/* 24.10.98 */



/***************************************************************************************/
/* dann das Grobe */
                          
                          
/* Macht einen groﬂen Schritt */
int	GoForward( int iAmplitude )
{          
	int	iWert=0;                                            
	
	*DIO &= 0xFFF0;	/* Alle vier unteren Bits aus */
	set_dac( WALK_DA, iWert );
	*DIO |= 0x000F;	/* Alle vier unteren Bits an */
	while(  iWert<iAmplitude  )
	{
		wait( 1.0 );
		iWert += 50;
		set_dac( WALK_DA, iWert );
	}                                            
	*DIO &= 0xFFF7;	/* Bit 3 aus */           
	wait( 100.0 );
	*DIO &= 0xFFF3;	/* Bit 2 aus */           
	wait( 100.0 );
	*DIO &= 0xFFF1;	/* Bit 1 aus */           
	wait( 100.0 );
	*DIO &= 0xFFF0;	/* Bit 0 aus */           
	wait( 100.0 );
} /* 11.1.99 */



/* Macht einen groﬂen Schritt */
int	GoBack( int iAmplitude )
{          
	*DIO &= 0xFFF0;	/* Alle vier unteren Bits aus */
	set_dac( WALK_DA, iAmplitude );
	wait( 50.0 );
	*DIO |= 0x0001;	/* Bit 1 an */           
	wait( 100.0 );
	*DIO |= 0x0003;	/* Bit 2 an */           
	wait( 100.0 );         
	*DIO &= 0x0007;	/* Bit 3 an */           
	wait( 100.0 );
	*DIO &= 0x000F;	/* Bit 4 an */           
	wait( 100.0 );
	while(  iAmplitude>0  )
	{
		wait( 1.0 );
		iAmplitude -= 50;
		set_dac( WALK_DA, iAmplitude );
	}                                            
	set_dac( WALK_DA, 0 );
	*DIO &= 0xFFF0;	/* Alle vier unteren Bits aus */
} /* 11.1.99 */




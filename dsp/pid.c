/****************************************************************************************
****	Kompletter PID-Algorithmus, einschließlich echter Integration                ****
****************************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include <periph.h>
#include <analog.h>

         
         
/****************************************************************************************/



/* Prototypes: Interrupt subroutines on chip! */
/* Interupt muessen c_int00 bis c_int99 heißen!!! */
#pragma CODE_SECTION(c_int90, ".onchip");
#define do_pid c_int90
/*#pragma INTERRUPT(do_pid)*/


#include "approach.h"
#include "scantyp.h"
#include "pid.h"

/****************************************************************************************/

/* für den eigenen Timer */
/* wird von do_pid erhöht */
volatile float	fMs;     
volatile float	fDeltaMs=0.0;
volatile int	bDoPID=FALSE;
volatile int	bDoGo=FALSE;
volatile int	bDoSPS=FALSE;
volatile int	bDoScan=FALSE;
volatile int	bInvertFeedback=FALSE;
/*float E = 2.71828;*/
volatile int	iXPosition, iYPosition;
float Regel=0;
/* Wartet iMs Millisekunden */
/* DARF NICHT AUS INTERRUPT AUFGERUFEN WERDEN !!!*/
void	wait( float fWaitMs )
{               
	fMs = -fWaitMs;
	fWaitMs = 0.0;
	while(  fWaitMs>fMs  )
		   ; /***********************************************/
		
}
/* 3.11.98 */



/*
 * Liest ADC aus */
short get_adc(unsigned int channel)
{         
	int wert;
    AIO_PAIR* adc = (AIO_PAIR*)&Periph->Adc[channel];
    adc->both = 0;
    wert = adc->lo;
    return wert;
}
/* 24.10.98 */

/* wenn TRUE, dann muss Z_DA invertiert werden */
static bInvertPiezo = 0; 
/* wenn TRUE, exponentielee regelung */
static bExpoRegel = 0;
/* merkt sich den gesetzten DAC wert, da write only register */
static long	shadow_dac[4];

/* Setzt DAC */                                  
void set_dac(unsigned int channel, long wert)
{
  long * dac = (long *)&Periph->Dac[channel];
  long * ldac = (long *)&Periph->Ldac[channel];
	if(  wert>PIEZO_LIMIT  )
		wert = PIEZO_LIMIT;
	else if(  wert<-PIEZO_LIMIT  )
		wert = -PIEZO_LIMIT;
	if(  channel==Z_DA  &&  bInvertPiezo  )
		shadow_dac[channel] = *dac = -wert;
	else
		shadow_dac[channel] = *dac = wert;
	*ldac = 0;
}
/* 24.10.98 */            

/* Scan muss z-channel invertieren */
int	invert_piezo(int bInvert)
{
	int	bOld = bInvertPiezo;
	bInvertPiezo = (bInvert!=0);
	set_dac(Z_DA,0);
	wait(500.0);
	return bOld;
}
/* 12.9.2003 */

/* Scan muss exponentiell regeln */
int	expo_regel(int bExp)
{
	int	bOldexp = bExpoRegel;
	bExpoRegel = (bExp!=0);
	return bOldexp;
}                  
                  
/*
 * Liest DAC aus (d.h. letzten Wert ermitteln) */
short get_dac(unsigned int channel)
{
	return shadow_dac[channel];
}
/* 24.10.98 */



/****************************************************************************************/

/* Werte für Regelung
 * fKp = 1/fProp,
 * fKi = 1/(fAbtastFrequenz*fInt),
 * fKd = fAbtastFrequenz*fDiff
 */
long 	lSoll;
float	fKp, fKi, fKd;

/* Hilfsvariablen (aktueller Wert des DAC) mal 16 (wegen Rechnefehlerminimierung) */
volatile long	iRegel=MIN_REGEL;
/* Für das Scannen: Fehler mitmessen */
int	iError;
                    
long	iHin[1024], iHinOk=FALSE;                    
long	iRueck[1024], iRueckOk=FALSE;                    

/****************************************************************************************/


/****************************************************************************************/
   
/* Setzt die Parameter für PID_Regelung und macht evt. vorher Approach */
/* Parameter sind:
unsigned wSoll			// Sollwert (eigentlich ein unsigned ...)
float fProp				// Proportionalbereich (5% ... 300%) %
float fInt				// Zeit über die integriert wird "Nachstellzeit" (unendlich  ... 0.00001) s
float fDiff				// Steilheit der Ableitung "Vorstellzeit" (0s ... unendlich) s
float fFrequenz			// Abtastfrequenz Hz

Gibt TRUE zurück, wenn erfolgreich
*/
void	InitPIDInt( float fFreq )
{
	/* Interrupt mit der Frequenz initialisieren */
	bDoPID = FALSE;	/* PID stop */
	fDeltaMs = 1000.0/fFreq;	/* Zählt in Millisekunden */

	install_int_vector( do_pid, PID_INTERRUPT+1 );
	enable_interrupt(PID_INTERRUPT); 			/* Enable timer interrupts */
	timebase( PID_TIMER, fFreq, (float)MHZ ); 

	/* Timer start */
	fMs = 0.0;
}


int	InitPID( long lFreq, long iSoll, long lProp, long lInt, long lDiff )
{
	float	f, fFreq=(float)lFreq;
	int		i;
              
  InitPIDInt( (float)lFreq );
	/* Variablen initialisieren */
	/* Millionenfache deshalb, da Langworte übertragen werden */
	fKp = ((float)lProp)/100000.0;
	if(  lInt==0  )
		fKi = 0.0;
	else
		fKi = 1000000.0/(fFreq*((float)lInt));
/*	fKd = 1000000.0/(fFreq*((float)lDiff));*/
	fKd = fFreq*((float)lDiff)/1000000.0;

	lSoll = iSoll;
	if(  iRegel<=MIN_REGEL  )
		/* neuen Approach */
	Approach( lSoll );
	bDoPID = TRUE;
	return TRUE;
}
/* 24.10.98 */
                              
                              
/* Differenz entweder linear oder logarithmisch o.a. */                              
float	Differenz2Regelwert( long iIst, long iSoll )
{
#if 1
	/* echt Logarithmisch: */
	if(bExpoRegel)
	{
		if(bInvertFeedback) {
			if(  iIst<-32000  ) {
				return 200.0;
			}
			/* to handle negative values, make everything positive */
			return (log(33000.0-(float)iIst)-log(33000.0-(float)iSoll))*15.0;
		}
		else {
			if(  iIst>32000  ) {
				return 200.0;
			}
			/* to handle negative values, make everything positive */
			return (log((float)iIst+33000.0)-log((float)iSoll+33000.0))*15.0;
		}
	}
	/* linear: */
	if(bInvertFeedback) {
		return (float)(iSoll-iIst);
	}
	else {
		return (float)(iIst-iSoll);
	}
/* 5.11.99 */
#endif
#if 0  
/*einstellen ob log oder lin geregelt wird mittels checkbox von Breusi */
	if(bExpoRegel) 
	{ /*echt Logarithmisch:*/ 
	int diff= iIst-iSoll; 
	if(  diff>0 )	
		{
		 if(diff/800>=1)
		  return -log(0.0001)*1000;
	   else
	    return -log(1-diff/800)*1000; 
	}else{
	  if(diff/800<=-1)
		  return log(0.0001)*1000;
	   else
	    return log(1+diff/800)*1000; 
  }
 }
 else
  {/* linear:  */
	return iIst-iSoll;
  }
 
#endif
#if 0
/*einstellen ob log oder lin geregelt wird mittels checkbox von Breusi */
 int diff= iIst-iSoll;
 if(bExpoRegel) 
	{ /*echt Logarithmisch:*/ 
	if (abs(diff) <= 1) 
	 return 0;
	if(  diff>0 ){
     return -log(1/diff)*1000; 
	}else{
	   return log(-1/diff)*1000; 
  }
 }
 else
  {/* linear:  */
	return diff;
  }
#endif
}
/* 20.11.03 */



/* Macht eine Geschwindikeits-PID-Regelung (Geschwindigkeitsalgorithmus)
 * Rückgabewert ist der neue Ausgangswert
 */
signed	PID( long lIst )
{
	static float	fErr=0, fErr1=0, fErr2=0, fErr3=0, fErr4=0;	
	/*static float	fErr, fErr1, fErr2, fErr3, fErr4;	/* das gleiche als float */
	float 			fDelta;         
	int				iDeltaRegel;

	fErr = Differenz2Regelwert( lIst, lSoll );
	iError = fErr;
/*	fErr = (float)ADC2ISTWERT(lIst)-(float)ADC2ISTWERT(lSoll);*/

	/* Ausgangssignal berechnen
	 * (Geschwindigkeitsalgorithmus)
	 */
/* von Prissi 
	fDelta = fKp*( (fErr-fErr1) 
					+ fKi*fErr 
					+ fKd*(fErr-2.0*fErr1+fErr2) );
	iDeltaRegel = (long)(fDelta*16.0+0.5);

*/
/* von Breusi*/ 
   fDelta = fKp*(fErr)
          + fKi*(fErr+fErr1+fErr2*0.75+fErr3*0.5+fErr4*0.25)/3.5
          + fKd/10.0*((fErr-fErr1)+(fErr-fErr4)/(4.0*4.0));
  if(bExpoRegel) 
   iDeltaRegel = (long)(fDelta*1);
  else
   iDeltaRegel = (long)(fDelta*16.0);
   
  /*einstellen ob log oder lin geregelt wird mittels checkbox
  if(bExpoRegel) 
	{/* echt logarithmisch: 
	 if(  iDeltaRegel<0 )	
	  {
	  	if(iDeltaRegel/100.0<-1)
	  	 iDeltaRegel=ceil(-log(0.0001)*1.0);
		  else
	     iDeltaRegel=ceil(-log(1+iDeltaRegel/100.0)*1.0);
	  }else{
      if(iDeltaRegel/100.0>1)
	  	 iDeltaRegel=ceil(log(0.0001)*1.0);
		  else
	     iDeltaRegel=ceil(log(1-iDeltaRegel/100.0)*1.0);
    }
  } /*logarithmisch ende*/
  
  if(  iDeltaRegel>5000  )
    iDeltaRegel = 5000;  
  else if(  iDeltaRegel<-5000  )
    iDeltaRegel = -5000;

	iRegel -= iDeltaRegel;
  
			/* Fehlerwerte veralten */
	fErr1 = fErr;
	fErr2 = fErr1;
	fErr3 = fErr2;
	fErr4 = fErr3;
 	
 	/* Grenzen überprüfen */
  	if(  iRegel>MAX_REGEL  )
	  	iRegel = MAX_REGEL;
	  else if(  iRegel<MIN_REGEL  )
		  iRegel = MIN_REGEL;
  
 return iRegel;

}
/* 24.10.98 */

                                                                
                                                                
                  
volatile int	iScanCount;
int	lScanFreq;

/*
 *	This handler is invoked whenever TIM0 expires.
 *	and handels the PID
 */          
void do_pid(void)
{             
	fMs += fDeltaMs;
	if(  bDoPID  )   
	{
		
		int iNeu = PID( get_adc( DAEMPFUNG_AD ) );
		set_dac( Z_DA, iNeu>>4 );
	}
	
	/* Die folgenden zwei schließen sich aus:
	 * entweder wird gescannt oder der Scankopf wird 
	 * neu positioniert (oder nix) */
	if(  bDoScan  )
	{
		/* Bild aufnehmen */
		iScanCount = (++iScanCount)%lScanFreq;
		if(  iScanCount==0  &&  bDoSPS) {
			do_sps();
			iScanCount = 1;	/* wait another round */
		}
		if(  iScanCount==0) {
			do_scan();                     
		}
	}
	else {
		if(  bDoGo  )
		{
			/* Scankopf irgendwo hinbewegen */
			iScanCount = (++iScanCount)%lScanFreq;  
			if(  iScanCount==0  )
				do_go();
		}
	}
}
/* 20.10.98 */
/* 18.4.99 */



/****************************************************************************************/
/* Ab hier Bildaufnahme */

GOPAR		Go;
SCANPAR	Scan; 
int			iX, iY, iDir; 


/* Bricht einen Scan in jedem Fall ab */
void AbortScan(void)
{
	bDoScan = FALSE;	/* Speichern an */
 	iHinOk = iRueckOk = FALSE;
	Retract();
	write_mailbox( ABORTSCAN, SNOMPUTZ_MB );
}
/* 3.11.98 */



/* Bereitet alles für die Bildaufnahme vor */
int	InitGo( GOPAR *pGo )
{
	bDoScan = FALSE;	/* Scannen aus */
 	iHinOk = iRueckOk = FALSE;

	/* An die gewünschte Position fahren */
	if(  bDoPID  )
	{
		/* entweder langsam mit Regelung */
		Go.iPts = pGo->iPts;
		lScanFreq = Go.lFreq = pGo->lFreq;
		Go.iEndX = pGo->iEndX;
		Go.iEndY = pGo->iEndY;
		Go.iStartX = iXPosition;
		Go.iStartY = iYPosition;
		iX = 0;
	 	iScanCount = 1; /* Maximale Zahl Zyklen warten ... */
		bDoGo = TRUE;
		while(  bDoGo  );	/* Warten, bis da! */
	}
	else
	{
		/* Oder eher rasant */
		set_dac(X_DA,pGo->iEndX);
		set_dac(Y_DA,pGo->iEndY);
		iXPosition = pGo->iEndX;
		iYPosition = pGo->iEndY;
	}
}
/* 18.4.99 */



/* Take a tunnel spectrum */
void do_sps()
{
	int i;
	int bOldPID=bDoPID;
	int old_volt = get_dac(WALK_DA);
	bDoPID = FALSE;
	write_mailbox( STARTSPS,  SNOMPUTZ_MB );
	for(i=0; i<2048; i++  ) {
		set_dac( WALK_DA, 32767-(i*32) );
		wait( 0.1 );
		write_mailbox( get_adc(DAEMPFUNG_AD), SNOMPUTZ_MB );
	}
	set_dac( WALK_DA, old_volt );
	wait( 10.0 );
	bDoPID = bOldPID;
}



/* Scankopf bewegen (nur ohne Scannen!) */
void	do_go(void)
{
	if(  ++iX<Go.iPts  )
	{
		iXPosition = ((Go.iEndX-Go.iStartX)*iX)/Go.iPts+Go.iStartX;
		iYPosition = ((Go.iEndY-Go.iStartY)*iX)/Go.iPts+Go.iStartY;
		set_dac( X_DA, iXPosition );
		set_dac( Y_DA, iYPosition );
	}
	else
	{
		/* am Ziel angekommen ... */
		set_dac( X_DA, Go.iEndX );
		set_dac( Y_DA, Go.iEndY );
		bDoGo = FALSE;
	}
}
/* 18.4.99 */



/* Bereitet alles für die Bildaufnahme vor */
int	InitScan( SCANPAR *pScan )
{
	bDoScan = FALSE;	/* Speichern aus */
 	iHinOk = iRueckOk = FALSE;

	/* An die gewünschte Position fahren */
	if(  bDoPID  )
	{
		/* entweder langsam mit Regelung */
		Go.iPts = pScan->iXPts;
		lScanFreq = Go.lFreq = pScan->lFreq;
		Go.iEndX = pScan->iXOff;
		Go.iEndY = pScan->iYOff;
		Go.iStartX =  iXPosition;
		Go.iStartY =  iYPosition;
		iX = 0;
	 	iScanCount = 1; /* Maximale Zahl Zyklen warten ... */
		bDoGo = TRUE;
		while(  bDoGo  );	/* Warten, bis da! */
	}
	else
	{
		/* Oder eher rasant */
		set_dac(X_DA,pScan->iXOff);
		set_dac(Y_DA,pScan->iYOff);
	}

	Scan.iModus = pScan->iModus;
	Scan.lFreq = pScan->lFreq;
	Scan.iXPts = pScan->iXPts;
	Scan.iYPts = pScan->iYPts;
	Scan.iW = pScan->iW;
	Scan.iH = pScan->iH;
	Scan.iXOff = pScan->iXOff;
	Scan.iYOff = pScan->iYOff;
	Scan.lWinkel = pScan->lWinkel;
	Scan.iDir = pScan->iDir;
	Scan.iOffset = pScan->iOffset;

	iX = iY = 0;
	iDir = LINKS2RECHTS;
	set_pos();

	if(  !bDoPID  )
 		Approach( lSoll );                                          

 	lScanFreq = Scan.lFreq;
 	iScanCount = 1; /* Maximale Zahl Zyklen warten ... */
	bDoPID = TRUE;	/* Regelung an */
	bDoScan = TRUE;	/* Speichern an */
}
/* 3.11.98 */

  
 
       
/* Neue XY-Position am DA setzen.
 * (iX, iY, Scan sind globale Variablen 
 */  
void set_pos( void )
{
	iXPosition = Scan.iXOff+(iX/(float)Scan.iXPts)*Scan.iW;
	iYPosition = Scan.iYOff+(iY/(float)Scan.iYPts)*Scan.iH;
	set_dac( X_DA, iXPosition );
	set_dac( Y_DA, iYPosition );
} 
/* 3.11.98 */

 
/*	This handler is invoked whenever TIM1 expires.
 *	and handels the Scan of the image
 */  
void do_scan(void)
{
  int i=iRegel>>4, j=iError;
                    
  if(  iHinOk  &&  iRueckOk  )
   	return;
  if(  iY>=Scan.iYPts  )
  {              
  	/* Kann scannen beendet werden? */
  	if(  iHinOk  ||  iRueckOk  )
  		return;
		/* Abbruch */
		bDoScan = FALSE;
		return;
  }

  Scan.iOffset = 0;                
	/* Zuerst von Links nach Rechts */
	if(  iDir==LINKS2RECHTS  )
	{
		if(  iX<Scan.iXPts+Scan.iOffset  )
		{
			/* X und Y DA für nächsten Punkt setzen */
			set_pos();

			if(  Scan.iDir&LINKS2RECHTS  &&  iX>=Scan.iOffset  )
			{
				iHinOk = FALSE;
				i <<= 16;
				if(  Scan.iModus&ERRO  )
					iHin[iX-Scan.iOffset] = i|(0x0000FFFFu & j);
				else if(  Scan.iModus&LUMI  )
					iHin[iX-Scan.iOffset] = i|(0x0000FFFFu & get_adc( LUMI_AD ));
				else
					iHin[iX-Scan.iOffset] = i;
			}
			iX ++;
			return;
		}
		else
		{
			/* Scanrichtung wieder zurück */
			if(  Scan.iDir&LINKS2RECHTS  )
			{
				iHinOk = TRUE;
				iY ++;
			}
			/* X und Y DA für nächsten Punkt setzen */
			iX = Scan.iXPts+Scan.iOffset;
			iDir = RECHTS2LINKS;
			set_pos();
			return ;
		}
	}

	/* dann von Rechts nach Links */
	if(  iDir==RECHTS2LINKS  )
	{
		if(  iX>=0  )
		{
			/* X und Y DA für nächsten Punkt setzen */
			set_pos();
			iX --;

			if(  Scan.iDir&RECHTS2LINKS  &&  iX<Scan.iXPts  )
			{
				iRueckOk = FALSE;
				i <<= 16;
				if(  Scan.iModus&ERRO  )
					iRueck[iX] = i|(0x0000FFFFu & j);
				else if(  Scan.iModus&LUMI  )
					iRueck[iX] = i|(0x0000FFFFu & get_adc( LUMI_AD ));
				else
					iRueck[iX] = i;
			}
			return;
		}
		else
		{
			/* Scanrichtung wieder zurück */
			iX = 0;
			if(  Scan.iDir&RECHTS2LINKS  )
			{
				iRueckOk = TRUE;
				iY ++;
			}
			iDir = LINKS2RECHTS;
			/* X und Y DA für nächsten Punkt setzen */
			set_pos();
			return ;
		}
	}
}
/* 3.11.98 */

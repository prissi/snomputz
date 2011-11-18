/*                
 * MAIN-DSP.C
 */                                                   
 

#include <periph.h>
#include <analog.h>

#include "scantyp.h"
#include "approach.h"
#include "pid.h"
                              



/* Hauptverteiler */
void main( void )
{
	extern volatile long iRegel;
	extern volatile int bDoPID, bInvertFeedback, bDoScan, bDoSPS;
	int		modus, iWert, message;
	int		iFreq, iSoll, iProp, iInt, iDiff;
	SCANPAR	Scan;
	GOPAR		Go;

	enable_cache();                                     
	enable_interrupts();                                
	disable_clock();

	/* Damit sinnvolle Zeitbasis vorhanden! */
	InitPIDInt( 25000.0 );
	iRegel = 0;/*MIN_REGEL<<4;*/
	Retract();

	while(  1  )
	{
		if(  !read_mb_terminate( &message, 0, SNOMPUTZ_MB )  )
		{   
			int	i;
			extern volatile int iHinOk, iRueckOk;
			extern volatile int iHin[1024], iRueck[1024];   
			
			/* Nix gelesen, muss einen Rasterzeile übertragen werden? */
			if(  iHinOk  )
			{
				for(  i=0;  i<10  &&  iHinOk  &&  !write_mb_terminate( STARTLINE, 4, SNOMPUTZ_MB );  i++  )
				{
					wait( 200.0 );
					if(  read_mb_terminate( &message, 0, SNOMPUTZ_MB )  )
						goto MessageDa;
				}
				if(  i==10  )
					continue;
				for(  i=0;  i<Scan.iXPts  &&  iHinOk;  i++  )  
				{
					write_mailbox( iHin[i], SNOMPUTZ_MB );
				}
				iHinOk = FALSE;
				continue;
			}
			if(  iRueckOk  )
			{
				for(  i=0;  i<10  &&  iRueckOk  &&  !write_mb_terminate( STARTLINE, 4, SNOMPUTZ_MB );  i++  )
				{
					if(  read_mb_terminate( &message, 0, SNOMPUTZ_MB )  )
						goto MessageDa;
					wait( 2.0 );
				}
				if(  i==100  )
					continue;
				for(  i=0;  i<Scan.iXPts  &&  iRueckOk;  i++  )
				{
/*					if(  read_mb_terminate( &message, 0, SNOMPUTZ_MB )  )
						goto MessageDa;*/
					write_mailbox( iRueck[i], SNOMPUTZ_MB );
				}
				iRueckOk = FALSE;
				continue;
			}
			message = 0;
			continue;
		}    
MessageDa:

		/* Ok, Kommando muss bearbeitet werden! */
		switch( message )
		{
			case SET_PID_PARAMETER :	
						/* Neue PID-Parameter */
						iFreq = read_mailbox( SNOMPUTZ_MB );
						iSoll = read_mailbox( SNOMPUTZ_MB );
						iProp = read_mailbox( SNOMPUTZ_MB );
						iInt  = read_mailbox( SNOMPUTZ_MB );
						iDiff = read_mailbox( SNOMPUTZ_MB );
						InitPID( iFreq, iSoll, iProp, iInt, iDiff );
						write_mailbox( 1, SNOMPUTZ_MB );
					break;

			case INVERT_PIEZO_CMD:
						invert_piezo( read_mailbox( SNOMPUTZ_MB ) );
					break;

	      case EXPONENTIALLY_CMD:
							expo_regel( read_mailbox( SNOMPUTZ_MB ) );
						break;
	
	      case TUNNELVOLTAGE_CMD:
	      	{
	      		long tunnel_voltage = read_mailbox( SNOMPUTZ_MB );
				int old_volt = get_dac(WALK_DA);
				int bOldPID=bDoPID;
				bDoPID = FALSE;
				set_dac( WALK_DA, tunnel_voltage );
				bInvertFeedback = (tunnel_voltage<0);
				wait( 10.0 );
				bDoPID = bOldPID;
			}
			break;
	
			case COARSE_FORWARD:
						GoForward( read_mailbox( SNOMPUTZ_MB ) );
					break;

			case COARSE_BACK:
						GoBack( read_mailbox( SNOMPUTZ_MB ) );
					break;

			case ABORT :  
						/* Abbruch, Zurueckziehen */
						AbortScan();
					break;

			case GET_SPS_CMD:
					{
						bDoSPS = TRUE;
						if(!bDoScan) {
							do_sps();
						}
					}
					break;

			case GET_VALUE :	
						/* Liest ein Wertepaar */
						modus = read_mailbox( SNOMPUTZ_MB );
						if(  modus&TOPO  )
							iWert = (iRegel>>4);
						if(  modus&ERRO  )
							write_mailbox( (iWert<<16)|((get_adc( DAEMPFUNG_AD ))&0x0000FFFF),  SNOMPUTZ_MB );
						else
							write_mailbox( (iWert<<16)|((get_adc( LUMI_AD ))&0x0000FFFF),  SNOMPUTZ_MB );
					break;

			case SET_POSITION :
						/* Scanparameter + Scan starten */
						Go.lFreq = read_mailbox( SNOMPUTZ_MB );	/* Scanfrequenz in Hz = 1/(Scanzeilen pro s) */
						Go.iPts = read_mailbox( SNOMPUTZ_MB );		/* Soviel Zeilen pro Bild */
						Go.iEndX = read_mailbox( SNOMPUTZ_MB );		/* Zyklenzahl */
						Go.iEndY = read_mailbox( SNOMPUTZ_MB );		/* Sollwert */
						read_mailbox( SNOMPUTZ_MB );		/* Start-X */
						read_mailbox( SNOMPUTZ_MB );		/* Start-Y, kennen wir besser! */
						InitGo( &Go );
						write_mailbox( 1, SNOMPUTZ_MB );
						break;
						
			case SET_SCAN_PARAMETER :
						/* Scanparameter + Scan starten */
						Scan.iModus = read_mailbox( SNOMPUTZ_MB );		/* Was soll gescann werden? TOPO+ERRO oder LUMI */
						Scan.lFreq = read_mailbox( SNOMPUTZ_MB );	/* Scanfrequenz in Hz = 1/(Scanzeilen pro s) */
						Scan.iXPts = read_mailbox( SNOMPUTZ_MB );		/* Soviel Punkte pro Zeile */
						Scan.iYPts = read_mailbox( SNOMPUTZ_MB );		/* Soviel Zeilen pro Bild */
						Scan.iW	  = read_mailbox( SNOMPUTZ_MB );		/* Breite in Rasterpunkten */
						Scan.iH	  = read_mailbox( SNOMPUTZ_MB );		/* Hoehe in Rasterzeilen */
						Scan.iXOff = read_mailbox( SNOMPUTZ_MB );		/* Frequenz in Hz */
						Scan.iYOff = read_mailbox( SNOMPUTZ_MB );		/* Sollwert */
						Scan.lWinkel = read_mailbox( SNOMPUTZ_MB );	/* Winkel */
						Scan.iDir  = read_mailbox( SNOMPUTZ_MB );		/* 1=Links->Rechts, 2 = Rechts->Links, 3=beide */
						Scan.iOffset = read_mailbox( SNOMPUTZ_MB );		/* Offset für Rechts->Links scannen */
						InitScan( &Scan );
						write_mailbox( 1, SNOMPUTZ_MB );
					break;
		}
	}
	/* This never ends ... */
}
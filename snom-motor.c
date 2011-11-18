
/*************************************************************************/
// Ansteuerung eines Linearmotors vom Typ PI M-227.25 zur Grobannäherung
// by Ulrich Poschinger --------- poschi@poschitech.de ------ 24.02.03
/*************************************************************************/



#ifdef BIT32
#error "Nur 32Bit!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <conio.h>

#include "snomputz.h"
#include "snom-motor.h"
#include "snom-typ.h"

extern MOTOR_CONTROL Snom_MotorControl;

//Callback Prozedur des Multimediatimers, wird alle 1ms aufgerufen
void CALLBACK mmTimerProc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2) 
{ 
	motor_proc((HWND)dwUser);
} 



// this funktion runs un a thread loop
void	MotorThread( MOTOR_CONTROL *mc )
{
	LARGE_INTEGER	llDeltaTime, llTime, llCurrentTime;
	long double		fDeltaTime, fTime, fCurrentTime;
	unsigned char OutByte=0;

	QueryPerformanceFrequency( &llDeltaTime );
	fDeltaTime = (double)llDeltaTime.LowPart + 2147483647.0*(double)llDeltaTime.HighPart;
	fDeltaTime /= 1000.0*mc->PulseTime;	// counts per Puls
	
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
		while(  fCurrentTime<fTime  );
		fTime = fCurrentTime;
	}
	//Checken ob Motor duchgelaufen ist und ggfs. stoppen
	_outp((unsigned short)mc->PortAdr,0);
	mc->MotorStatus = MOTOR_IDLE;
	StopMotor();
}



//zentrale Steuerprozedur des Motors
UINT motor_proc(HWND hDlg)
{
	BYTE OutByte,InByte;
	
	if(Snom_MotorControl.MotorStatus==MOTOR_IDLE)
		return(0);
	
	//Ausgabebyte für LPT je nach Fahrrichtung setzen
	if(Snom_MotorControl.MotorStatus==MOTOR_IWD)
		OutByte=0x03;
	else if(Snom_MotorControl.MotorStatus==MOTOR_OWD)
		OutByte=0x0c;
	
	//Nach abgelaufenem Tastzyklus Cyclecount zurücksetzen
	if(Snom_MotorControl.CycleCount>=Snom_MotorControl.TastOn+Snom_MotorControl.TastOff)
	{
		Snom_MotorControl.CycleCount=0;
	}
	//Während Low-Phase des Tastzyklus Ausgabebyte nullen
	if(Snom_MotorControl.CycleCount>=Snom_MotorControl.TastOn)
	{
		OutByte=0x00;
	}

	//Ausgabebyte an LPT senden, ACHTUNG: _outp funktioniert vermutlich NICHT
	//unter Windows NT
	if(OutByte!=0)
		_outp((unsigned short)Snom_MotorControl.PortAdr,OutByte);
	else
		_outp((unsigned short)Snom_MotorControl.PortAdr,(unsigned short)0x00);

	//Interne Zähler inkrementieren
	Snom_MotorControl.PulseCount++;
	Snom_MotorControl.CycleCount++;
	
	//Checken ob Motor duchgelaufen ist und ggfs. stoppen
	if(Snom_MotorControl.PulseCount>Snom_MotorControl.AnzPulse)
	{
		Snom_MotorControl.MotorStatus=MOTOR_IDLE;
		StopMotor();
	}
	return(0);
}


//Pseudo-Methoden zum setzen der Motorparameter, dieser Weg wurde
//aus Übersichlichkeitsgründen gewählt
void SetMotorAnzPulse(unsigned long AnzPulse)
{
	Snom_MotorControl.AnzPulse=AnzPulse;
}

void SetMotorPulseTime(unsigned long PulseTime)
{
	Snom_MotorControl.PulseTime=PulseTime;
}

void SetMotorTastOn(unsigned int TastOn)
{
	Snom_MotorControl.TastOn=TastOn;
}

void SetMotorTastOff(unsigned int TastOff)
{
	Snom_MotorControl.TastOff=TastOff;
}

void SetMotorStatus(enum MOTOR_STATUS MotorStatus)
{
	Snom_MotorControl.MotorStatus=MotorStatus;
}


//Interne Zähler zurücksetzen
void ResetMotor(void)
{	
	Snom_MotorControl.CycleCount=0;
	Snom_MotorControl.PulseCount=0;
	Snom_MotorControl.PortAdr=0x378;
}

//Defaultwerte setzen
void InitMotor(void)
{
	Snom_MotorControl.AnzPulse=1;
	Snom_MotorControl.PulseTime=1;
	Snom_MotorControl.TastOff=1;
	Snom_MotorControl.TastOn=1;
}

//Setzt den LPT auf 0x00, Achtung: Motor wird nicht dauerhaft angehalten solange#
//der Multimediatimer noch aktiv ist!!
void StopMotor(void)
{
	_outp((unsigned short)Snom_MotorControl.PortAdr,0x00);		
}


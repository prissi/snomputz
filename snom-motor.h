
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>


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

void CALLBACK mmTimerProc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

// this funktion may run in a thread loop
void	MotorThread( MOTOR_CONTROL *mc );

UINT MotorProc(HWND hDlg);
void SetMotorAnzPulse(unsigned long AnzPulse);
void SetMotorPulseTime(unsigned long PulseTime);
void SetMotorTastOn(unsigned int TastOn);
void SetMotorTastOff(unsigned int TastOff);
void SetMotorStatus(enum MOTOR_STATUS MotorStatus);
void ResetMotor(void);
void StopMotor(void);
void InitMotor(void);
long	DspQueryValue( LONG lModus );



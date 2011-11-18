//
//    cardinfo.h   --   definition of CARDINFO structure
//

#ifndef __CARDINFO_H__
#define __CARDINFO_H__

#include "ii_iostr.h"       // Common IO Driver/DLL Structures
#include "mailbox.h"        // Definition of MAILBOX structures

//
//  BoardInfo structure
//
typedef struct _BoardInfo
{
    ULONG             ProcessorCount;
    ULONG             DLL_Version;            // Version ID numbers
    ULONG             DrvVersion;
    ULONG             TalkerVersion;
    ULONG             CellSize;               // Targ memory cell size, in bytes 
    ULONG             CtlReg;                 // Shadow of control register 
	ULONG			  FlashSectorSize;		  // Size of flash sectors, in bytes
	ULONG			  FlashDeviceId;		  // Flash device ID
    ULONG             QuietMode;              // Don't Display Messages if true
} BoardInfo;

//
//  InterruptInfo structure
//
typedef struct _InterruptInfo
{
    ULONG            IRQ;                     // IRQ of attached interrupt
    HANDLE           Ring0Event;              // Ring 0 event handle 
    HANDLE           Ring3Event;              // Ring 3 event handle 
    void             (*Vector)(void *);       // Virtual ISR function pointer 
	void *			 Context;				  // Virtual ISR context pointer
 } InterruptInfo;

//
//  SerialInfo structure
//


typedef struct _SerialInfo
{
    LONG             In;          // Buffer for last character received 
    LONG             ReadFlag;    // True when character received 
    LONG             MbValue;     // Multi-byte value 
    LONG             MbCtr;       // Multi-byte read state 
    ULONG            RTS_state;   // Current state of the RTS output
	LONG			 Bcr;		  // Bus control register value for Flash access
	LONG             Reading;     // TRUE if currently reading a character
    OVERLAPPED       RxOverlap;   // Info used in asynch input
    OVERLAPPED       TxOverlap;   // Info used in asynch output
    COMMTIMEOUTS     Timeouts;    // Info for set/query time-out parameters 
    DCB              Dcb;         // Device control block 
} SerialInfo;

//
//  CARDINFO  structure
//
typedef struct _cardinfo
{
    ULONG             Target;                 // Number of current target
    HANDLE            Device;                 // Handle to Driver for device
    BoardInfo         Info;                   // Board Info
    MAILBOX *         Mail;                   // Talker Mailbox Array 
    IoPortBlock       Port;                   // Primary Port Block Information
    IoPortBlock       OpReg;                  // Secondary Port Block Information
    MemoryBlock       DualPort;               // Shared Memory Area Information
    MemoryBlock       BusMaster;              // BusMaster Memory Information
    InterruptInfo     Interrupt;              // Interrupt Information
    SerialInfo        Serial;                 // Serial Port I/O (SBC's)
} CARDINFO;


#endif



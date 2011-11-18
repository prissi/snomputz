//
//  PC32ioctl.h   --  PC32 IO Control Definitions
//
#ifndef __PC32_IOCTL_H__
#define __PC32_IOCTL_H__

#include "devioctl.h"
#include "ii_iostr.h"    // Common IOCTL structures

#define  PC32_IOCTL_MAP_DUALPORT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB0, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_UNMAP_DUALPORT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB1, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_PORT_READ \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB2, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_PORT_WRITE \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB3, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_DUALPORT_READ \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB4, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_DUALPORT_WRITE \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB5, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_RESET \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB6, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_RUN \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB7, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_WAITFOR_INT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB8, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_ENABLE_INT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xAB9, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_DISABLE_INT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xABA, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_GET_BD_RESOURCES \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xABB, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_LOAD_IRQEVENT \
	CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xABC, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define  PC32_IOCTL_LOAD_TARGET \
    CTL_CODE ( FILE_DEVICE_UNKNOWN, 0xABD, METHOD_BUFFERED, FILE_ANY_ACCESS )



//--------------------------------------------------
//  PC32 32 IO Port Register Offsets
//
#define  CONTROL  0x00
#define  INT_0	  0x02
#define  INT_1	  0x04
#define  SEM_0	  0x06
#define  SEM_1    0x08
#define  SEM_2    0x0A
#define  SEM_3    0x0C

//==================================================
//   Structures for PC32 IOCTL Commands
//
//
typedef struct _PC32_PORTIO
	{
	ULONG			Offset;
	USHORT			Data;
	} PC32_PORTIO, *PPC32_PORTIO;

typedef struct _PC32_MEMIO
	{
	ULONG			Offset;
	USHORT			Data;
	} PC32_MEMIO, *PPC32_MEMIO;

typedef struct _PC32_BOARD_DESC
    {
    MemoryBlock     DualPort;
    IoPortBlock     IO;
    ULONG           IRQ;
    ULONG           Version;
    } PC32_BOARD_DESC, *PPC32_BOARD_DESC;

typedef struct _PC32_LOAD_EVENT
    {
    HANDLE          Ring0Handle;
    } PC32_LOAD_EVENT, *PPC32_LOAD_EVENT;


typedef struct _PC32_LOAD_TARGET
    {
    ULONG           Target;
    } PC32_LOAD_TARGET, *PPC32_LOAD_TARGET;



#endif

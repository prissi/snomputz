//
//  target.h  
//
//  This header holds Common Definitions of a
//     Target Board. 
//
//
#ifndef __target_h__
#define __target_h__

#include "cardinfo.h"     //  Common Board Description Structure
#include "alias.h"        //  Board Specific Function Aliases
//
//  Aliased Prototypes
//
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef	IMPORT_AS_DLL

#undef  EXPORT
#define EXPORT EXTERN_C _stdcall
//#define EXPORT extern _cdecl

//
//   General Functions
//
int    EXPORT target_open(int instance);
int    EXPORT target_close(int target);
LPVOID EXPORT target_cardinfo(int target);
int    EXPORT iicoffld(char *, int target, HWND hParent);
//
//  Conversion Functions
//
int    EXPORT from_ieee(float x);       
float  EXPORT to_ieee(unsigned int i);      
//
//  Interrupt Functions
//
void   EXPORT host_interrupt_install(int target, void (*virtual_isr)(void *), void * context);
void   EXPORT host_interrupt_deinstall(int target);
BOOL   EXPORT host_interrupt_enable(int target);
BOOL   EXPORT host_interrupt_disable(int target);
//
//  Low Level I/O
//
void   EXPORT target_reset(int target);
void   EXPORT target_run(int target);
void   EXPORT target_outport(int target, int port, int value);
int    EXPORT target_inport(int target, int port);
void   EXPORT target_opreg_outport(int target, int port, int value);
int    EXPORT target_opreg_inport(int target, int port);
void   EXPORT target_control(int target, int bit, int state);
//
//  Mailbox I/O Functions
//
long   EXPORT read_mailbox(int target, int);
void   EXPORT write_mailbox(int target, int, int);
BOOL   EXPORT check_outbox(int target, int);
BOOL   EXPORT check_inbox(int target, int);
int    EXPORT read_mb_terminate(int target, int, int *, int wide);
int    EXPORT write_mb_terminate(int target, int box_number, int value, int wide);
void   EXPORT clear_mailboxes(int target);
int    EXPORT target_key(int target);
void   EXPORT target_emit(int target, int value);
void   EXPORT target_Tx(int target, int value);
int    EXPORT target_Rx(int target);
void   EXPORT get_semaphore(int target, int semaphore);
//
//  Board Specific Functions
//
void         EXPORT target_interrupt(int target);
void         EXPORT request_semaphore(int target, int semaphore);
BOOL         EXPORT own_semaphore(int target, int semaphore);
void         EXPORT release_semaphore(int target, int semaphore);
void         EXPORT mailbox_interrupt(int target, unsigned int value);
unsigned int EXPORT mailbox_interrupt_ack(int target);
//
//  Talker Functions
//
int    EXPORT target_check(int target);
void   EXPORT start_app(int target);
int    EXPORT start_talker(int target);
int    EXPORT talker_revision(int target);
int    EXPORT talker_fetch(int target, int address);
void   EXPORT talker_store(int target, int address, int value);
void   EXPORT talker_download(int target, int address, int count);
void   EXPORT talker_launch(int target, int address);
void   EXPORT talker_resume(int target);
int    EXPORT talker_registers(int target);
void   EXPORT target_slow(int target);
void   EXPORT target_fast(int target);
void   EXPORT talker_flash_sector_erase(int target, int sector);
void   EXPORT talker_flash_init(int target);
void   EXPORT talker_flash_offset(int target, int offset);

#else

#define DSPDllName "PC32DLL.DLL"

#endif

#ifdef __cplusplus
}
#endif

#endif

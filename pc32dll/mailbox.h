/*
 *      mailbox.h   --  mailboz specific definitions
 *
 *      This file is designed for dual-port mailboxes
 */

#ifndef __mailbox_h__
#define __mailbox_h__

/* 
 *  Mailbox IDs
 */

#define MONITOR_MBOX     0   
#define TERMINAL_MBOX    0

/*
 *  MAILBOX structure -- overlay in dual port memory
 *     
 *    note -- this structure is a "mirror image" of the target side structure
 *              in "trg_mbox.h"
 */
  
typedef struct _host_mailbox
{
    volatile unsigned long     RCV;  /* RCV contains data transmitted from target to PC    */
    volatile unsigned long     ACK;  /* ACK signals that target has transmitted to PC (-1) */
    volatile unsigned long     XMT;  /* XMT contains data transmitted from PC to target    */
    volatile unsigned long     REQ;  /* REQ signals that PC has transmitted to target (-1) */
} MAILBOX;



#endif
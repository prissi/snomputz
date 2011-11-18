//
//  alias.h  
//
//  Board-Specific Definitions for PC32 Target
//
#ifndef __alias_h__
#define __alias_h__

#define   TARGET_PC32
#define   BOARD_NAME  "PC32"
#define   INTERRUPT_MBOX    1
//
//  Board Specific Definitions
//
#define   HASP_ADDR         0
//
//  DLL Aliases -- convert general name to board-specific
//"true name"
//
//   HASP Header Aliases
//
#define DSP_HASP                  PC32_HASP
//

#ifndef	IMPORT_AS_DLL
//
//   General Functions
//
#define target_open               PC32_open
#define target_close              PC32_close
#define target_cardinfo           PC32_cardinfo
#define iicoffld                  PC32_iicoffld
//
//  Conversion Functions
//
#define from_ieee                 PC32_from_ieee       
#define to_ieee                   PC32_to_ieee   
//
//  Interrupt Functions
//
#define host_interrupt_install    PC32_host_interrupt_install
#define host_interrupt_deinstall  PC32_host_interrupt_deinstall
#define host_interrupt_enable     PC32_host_interrupt_enable
#define host_interrupt_disable    PC32_host_interrupt_disable
//
//  Low Level I/O
//
#define target_reset              PC32_reset
#define target_run                PC32_run
#define target_outport            PC32_outport
#define target_inport             PC32_inport
#define target_opreg_outport      PC32_opreg_outport
#define target_opreg_inport       PC32_opreg_inport
#define target_control            PC32_control
//
//  Mailbox I/O Functions
//
#define read_mailbox              PC32_read_mailbox
#define write_mailbox             PC32_write_mailbox
#define check_outbox              PC32_check_outbox
#define check_inbox               PC32_check_inbox
#define read_mb_terminate         PC32_read_mb_terminate
#define write_mb_terminate        PC32_write_mb_terminate
#define clear_mailboxes           PC32_clear_mailboxes
#define target_key                PC32_key
#define target_emit               PC32_emit
#define target_Tx                 PC32_Tx
#define target_Rx                 PC32_Rx
#define get_semaphore             PC32_get_semaphore
//
//  Board Specific Functions
//
#define target_interrupt          PC32_interrupt
#define request_semaphore         PC32_request_semaphore
#define own_semaphore             PC32_own_semaphore
#define release_semaphore         PC32_release_semaphore
#define mailbox_interrupt         PC32_mailbox_interrupt
#define mailbox_interrupt_ack     PC32_mailbox_interrupt_ack
//
//  Talker Functions
//
#define target_check              PC32_check
#define start_app                 PC32_start_app
#define start_talker              PC32_start_talker
#define talker_revision           PC32_talker_revision
#define talker_fetch              PC32_talker_fetch
#define talker_store              PC32_talker_store
#define talker_download           PC32_talker_download
#define talker_launch             PC32_talker_launch
#define talker_resume             PC32_talker_resume
#define talker_registers          PC32_talker_registers
#define target_slow               PC32_slow
#define target_fast               PC32_fast
#define talker_flash_sector_erase PC32_flash_sector_erase
#define talker_flash_init         PC32_flash_init
#define talker_flash_offset       PC32_flash_offset

#else
/////////////////////////////////////////////////////////////////
// Prepare for DLL-Import

//
#define target_open  "PC32_open"
#define target_close  "PC32_close"
#define target_cardinfo  "PC32_cardinfo"
#define iicoffld  "PC32_iicoffld"
//
//  Conversion Functions
//
#define from_ieee  "PC32_from_ieee"
#define to_ieee  "PC32_to_ieee"
//
//  Interrupt Functions
//
#define host_interrupt_install  "PC32_host_interrupt_install"
#define host_interrupt_deinstall  "PC32_host_interrupt_deinstall"
#define host_interrupt_enable  "PC32_host_interrupt_enable"
#define host_interrupt_disable  "PC32_host_interrupt_disable"
//
//  Low Level I/O
//
#define target_reset  "PC32_reset"
#define target_run  "PC32_run"
#define target_outport  "PC32_outport"
#define target_inport  "PC32_inport"
#define target_opreg_outport  "PC32_opreg_outport"
#define target_opreg_inport  "PC32_opreg_inport"
#define target_control  "PC32_control"
//
//  Mailbox I/O Functions
//
#define read_mailbox  "PC32_read_mailbox"
#define write_mailbox  "PC32_write_mailbox"
#define check_outbox  "PC32_check_outbox"
#define check_inbox  "PC32_check_inbox"
#define read_mb_terminate  "PC32_read_mb_terminate"
#define write_mb_terminate  "PC32_write_mb_terminate"
#define clear_mailboxes  "PC32_clear_mailboxes"
#define target_key  "PC32_key"
#define target_emit  "PC32_emit"
#define target_Tx  "PC32_Tx"
#define target_Rx  "PC32_Rx"
#define get_semaphore  "PC32_get_semaphore"
//
//  Board Specific Functions
//
#define target_interrupt  "PC32_interrupt"
#define request_semaphore  "PC32_request_semaphore"
#define own_semaphore  "PC32_own_semaphore"
#define release_semaphore  "PC32_release_semaphore"
#define mailbox_interrupt  "PC32_mailbox_interrupt"
#define mailbox_interrupt_ack  "PC32_mailbox_interrupt_ack"
//
//  Talker Functions
//
#define target_check  "PC32_check"
#define start_app  "PC32_start_app"
#define start_talker  "PC32_start_talker"
#define talker_revision  "PC32_talker_revision"
#define talker_fetch  "PC32_talker_fetch"
#define talker_store  "PC32_talker_store"
#define talker_download  "PC32_talker_download"
#define talker_launch  "PC32_talker_launch"
#define talker_resume  "PC32_talker_resume"
#define talker_registers  "PC32_talker_registers"
#define target_slow  "PC32_slow"
#define target_fast  "PC32_fast"
#define talker_flash_sector_erase  "PC32_flash_sector_erase"
#define talker_flash_init  "PC32_flash_init"
#define talker_flash_offset  "PC32_flash_offset"

#endif

#endif


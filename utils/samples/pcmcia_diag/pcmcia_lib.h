/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

#ifndef _PCMCIA_LIB_H_
#define _PCMCIA_LIB_H_

/************************************************************************
*  File: pcmcia_lib.h
*
*  Library for accessing PCMCIA devices.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#include "wdc_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Default manufacturer and device IDs (0 = all) */
/* TODO: Replace the ID values with your device's manufacturer and device IDs */
#define PCMCIA_DEFAULT_MANUFACTURER_ID 0x0 /* Manufacturer ID */
#define PCMCIA_DEFAULT_DEVICE_ID 0x0 /* Device ID */

/* TODO: use correct values according to the spec. of your device */
#define INTCSR 0x00
#define INTCSR_ADDR_SPACE 0
#define ALL_INT_MASK ((BYTE)0xFF)

/* PCMCIA run-time registers */
/* [Values should correlate to the registers' indexes in the gPCMCIA_Regs array] */
typedef enum {
    PCMCIA_INTCSR,    /* INTCSR - Interrupt Control/Status */
    PCMCIA_REGS_NUM,  /* Number of run-time registers */
} PCMCIA_REGS;

/* PCMCIA base address spaces (BARs) */
enum {
    PCMCIA_AD_BAR0 = 0, /* BAR0 */
};
/* TODO: You can add entries to the BARs enum as required for your device */

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR  sType[MAX_TYPE];
    CHAR  sName[MAX_NAME];
    CHAR  sDesc[MAX_DESC];
} PCMCIA_ADDR_SPACE_INFO;

/* Interrupt result information struct */
typedef struct {
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values in windrvr.h */
} PCMCIA_INT_RESULT;
/* TODO: You can add fields to PCMCIA_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in pcmcia_diag.c) */

/* PCMCIA diagnostics interrupt handler function type */
typedef void (*PCMCIA_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    PCMCIA_INT_RESULT *pIntResult);

/* PCMCIA diagnostics plug-and-play and power management events handler function type */
typedef void (*PCMCIA_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);

/*************************************************************
  Function prototypes
 *************************************************************/
DWORD PCMCIA_LibInit(void);
DWORD PCMCIA_LibUninit(void);

WDC_DEVICE_HANDLE PCMCIA_DeviceOpen(const WD_PCMCIA_CARD_INFO *pDeviceInfo);
BOOL PCMCIA_DeviceClose(WDC_DEVICE_HANDLE hDev);

DWORD PCMCIA_IntEnable(WDC_DEVICE_HANDLE hDev, PCMCIA_INT_HANDLER funcIntHandler);
DWORD PCMCIA_IntDisable(WDC_DEVICE_HANDLE hDev);
BOOL PCMCIA_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

DWORD PCMCIA_EventRegister(WDC_DEVICE_HANDLE hDev, PCMCIA_EVENT_HANDLER funcEventHandler);
DWORD PCMCIA_EventUnregister(WDC_DEVICE_HANDLE hDev);
BOOL PCMCIA_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

DWORD PCMCIA_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
BOOL PCMCIA_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev, PCMCIA_ADDR_SPACE_INFO *pAddrSpaceInfo);

const char *PCMCIA_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif

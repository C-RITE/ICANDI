/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

#ifndef _PCI_LIB_H_
#define _PCI_LIB_H_

/************************************************************************
*  File: pci_lib.h
*
*  Library for accessing PCI devices ,possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#include "wdc_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_PCI_DRIVER_NAME "KP_PCI"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user-mode) /
 * KP_PCI_Call() (kernel-mode) */
enum {
    KP_PCI_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel Plugin messages status */
enum {
    KP_PCI_STATUS_OK = 0x1,
    KP_PCI_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs (0 == all) */
/* TODO: Replace the ID values with your device's vendor and device IDs */
#define PCI_DEFAULT_VENDOR_ID 0x0 /* Vendor ID */
#define PCI_DEFAULT_DEVICE_ID 0x0 /* Device ID */

/* TODO: use correct values according to the spec. of your device */
#define INTCSR 0x00
#define INTCSR_ADDR_SPACE AD_PCI_BAR0
#define ALL_INT_MASK 0xFFFFFFFF

/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_PCI_VERSION;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} PCI_ADDR_SPACE_INFO;

/* Interrupt result information struct */
typedef struct {
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    DWORD dwEnabledIntType; /* Interrupt type that was actually enabled
                               (MSI/MSI-X/Level Sensitive/Edge-Triggered) */
    DWORD dwLastMessage; /* Message ID of the last received MSI (irrelevant
                            for line-based interrupts) */
} PCI_INT_RESULT;
/* TODO: You can add fields to PCI_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in pci_diag.c) */

/* PCI diagnostics interrupt handler function type */
typedef void (*PCI_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    PCI_INT_RESULT *pIntResult);

/* PCI diagnostics plug-and-play and power management events handler function
 * type */
typedef void (*PCI_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/*************************************************************
  Function prototypes
 *************************************************************/
DWORD PCI_LibInit(void);
DWORD PCI_LibUninit(void);

#if !defined(__KERNEL__)
WDC_DEVICE_HANDLE PCI_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo);
BOOL PCI_DeviceClose(WDC_DEVICE_HANDLE hDev);

DWORD PCI_IntEnable(WDC_DEVICE_HANDLE hDev, PCI_INT_HANDLER funcIntHandler);
DWORD PCI_IntDisable(WDC_DEVICE_HANDLE hDev);
BOOL PCI_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

DWORD PCI_EventRegister(WDC_DEVICE_HANDLE hDev,
    PCI_EVENT_HANDLER funcEventHandler);
DWORD PCI_EventUnregister(WDC_DEVICE_HANDLE hDev);
BOOL PCI_EventIsRegistered(WDC_DEVICE_HANDLE hDev);
#endif

DWORD PCI_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
BOOL PCI_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    PCI_ADDR_SPACE_INFO *pAddrSpaceInfo);

const char *PCI_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif

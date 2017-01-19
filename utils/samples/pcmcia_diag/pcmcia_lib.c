/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/************************************************************************
*  File: pcmcia_lib.c
*
*  Library for accessing PCMCIA devices.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "wdc_defs.h"
#include "utils.h"
#include "status_strings.h"
#include "pcmcia_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with your specific WinDriver license registration string and
         the driver name below with your driver's name */
#define PCMCIA_DEFAULT_LICENSE_STRING "12345abcde1234.license"
#define PCMCIA_DEFAULT_DRIVER_NAME "windrvr6"

/* PCMCIA device information struct */
typedef struct {
    WD_TRANSFER         *pIntTransCmds;
    PCMCIA_INT_HANDLER   funcDiagIntHandler;
    PCMCIA_EVENT_HANDLER funcDiagEventHandler;
} PCMCIA_DEV_CTX, *PPCMCIA_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information */

/*************************************************************
  Global variables definitions
 *************************************************************/
/* String for storing last error information */
static CHAR gsPCMCIA_LastErr[256];

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
static BOOL DeviceValidate(const PWDC_DEVICE pDev);
static void DLLCALLCONV PCMCIA_IntHandler(PVOID pData);
static void PCMCIA_EventHandler(WD_EVENT *pEvent, PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PPCMCIA_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsPCMCIA_LastErr, sizeof(gsPCMCIA_LastErr) - 1, "%s: NULL device %s\n",
            sFunc, !pDev ? "handle" : "context");
        ErrLog(gsPCMCIA_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    PCMCIA and WDC library initialize/uninit
   ----------------------------------------------- */
DWORD PCMCIA_LibInit(void)
{
    DWORD dwStatus;
 
    /* Set the driver name */
    if (!WD_DriverName(PCMCIA_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.\n");
        return WD_SYSTEM_INTERNAL_ERROR;
    }

    /* Set WDC library's debug options (default: level TRACE, output to Debug Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        
        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, PCMCIA_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to intialize the WDC library. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

DWORD PCMCIA_LibUninit(void)
{
    DWORD dwStatus;

    /* Uninit the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to uninit the WDC library. Error 0x%lx - %x\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
WDC_DEVICE_HANDLE PCMCIA_DeviceOpen(const WD_PCMCIA_CARD_INFO *pDeviceInfo)
{
    DWORD dwStatus;
    PPCMCIA_DEV_CTX pDevCtx = NULL;
    WDC_DEVICE_HANDLE hDev = NULL;

    /* Validate arguments */
    if (!pDeviceInfo)
    {
        ErrLog("PCMCIA_DeviceOpen: Error - NULL device information struct pointer\n");
        return NULL;
    }

    /* Allocate memory for the PCMCIA device context */
    pDevCtx = (PPCMCIA_DEV_CTX)malloc(sizeof (PCMCIA_DEV_CTX));
    if (!pDevCtx)
    {
        ErrLog("Failed allocating memory for PCMCIA device context\n");
        return NULL;
    }

    BZERO(*pDevCtx);

    /* Open a WDC device handle */
    dwStatus = WDC_PcmciaDeviceOpen(&hDev, pDeviceInfo, pDevCtx, NULL, NULL, NULL);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed opening a WDC device handle. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Validate device information */
    if (!DeviceValidate((PWDC_DEVICE)hDev))
        goto Error;

    /* Return handle to the new device */
    TraceLog("PCMCIA_DeviceOpen: Opened a PCMCIA device (handle 0x%p)\n", hDev);
    return hDev;

Error:    
    if (hDev)
        PCMCIA_DeviceClose(hDev);
    else
        free(pDevCtx);
    
    return NULL;
}

BOOL PCMCIA_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCMCIA_DEV_CTX pDevCtx;
    
    TraceLog("PCMCIA_DeviceClose entered. Device handle: 0x%p\n", hDev);

    if (!hDev)
    {
        ErrLog("PCMCIA_DeviceClose: Error - NULL device handle\n");
        return FALSE;
    }

    pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);
    
    /* Disable interrupts */
    if (WDC_IntIsEnabled(hDev))
    {
        dwStatus = PCMCIA_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }

    /* Close the device */
    dwStatus = WDC_PcmciaDeviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed closing a WDC device handle (0x%p). Error 0x%lx - %s\n",
            hDev, dwStatus, Stat2Str(dwStatus));
    }

    /* Free PCMCIA device context memory */
    if (pDevCtx)
        free (pDevCtx);
    
    return (WD_STATUS_SUCCESS == dwStatus);
}

static BOOL DeviceValidate(const PWDC_DEVICE pDev)
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    /* NOTE: You can modify the implementation of this function in order to
             verify that the device has the resources you expect to find */
    
    /* Verify that the device has at least one active address space */
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return TRUE;
    }
    
    ErrLog("Device does not have any active memory or I/O address spaces\n");
    return FALSE;
}

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
static void DLLCALLCONV PCMCIA_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCMCIA_DEV_CTX pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);
    PCMCIA_INT_RESULT intResult;

    BZERO(intResult);
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
    
    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

DWORD PCMCIA_IntEnable(WDC_DEVICE_HANDLE hDev, PCMCIA_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCMCIA_DEV_CTX pDevCtx;
    WDC_ADDR_DESC *pAddrDesc;
    WD_TRANSFER *pTrans;

    TraceLog("PCMCIA_IntEnable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PCMCIA_IntEnable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);

    /* Check if interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Define the number of interrupt transfer commands to use */
    #define NUM_TRANS_CMDS 2 /* TODO: Change this value, if needed */

    /*
       This sample demonstrates how to set up two transfer commands, one for
       reading the device's INTCSR register (as defined in gPCMCIA_Regs) and one
       for writing to it to acknowledge the interrupt. The transfer commands
       will be executed by WinDriver in the kernel when an interrupt occurs.

       TODO: Since the information for acknowledging the interrupts is
             hardware-specific, YOU MUST MODIFY THE CODE below and set up
             transfer commands in order to correctly acknowledge the interrupts
             on your device, as dictated by your hardware's specifications.
             
       *************************************************************************   
       * NOTE: If you attempt to use this code without first modifying it in   *
       *       order to correctly acknowledge your device's interrupts, as     *
       *       explained above, you will continue to receive notificaitons     *
       *       for the same interrupt repeatedly.                              *
       *************************************************************************
    */
    
    /* Allocate memory for the interrupt transfer commands */
    pTrans = (WD_TRANSFER*)calloc(NUM_TRANS_CMDS, sizeof(WD_TRANSFER));
    if (!pTrans)
    {
        ErrLog("Failed allocating memory for interrupt transfer commands\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    /* Prepare the interrupt transfer commands */
    /* The transfer commands will be executed by WinDriver in the kernel
       for each interrupt that is received */

    /* #1: Read status from the INTCSR register */
    pAddrDesc = &pDev->pAddrDesc[INTCSR_ADDR_SPACE];

    pTrans[0].dwPort = pAddrDesc->kptAddr + INTCSR;
    pTrans[0].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_BYTE : RP_BYTE; /* 8 bit read */
    
    /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the interrupt */
    pTrans[1].dwPort = pTrans[0].dwPort; /* In this example both commands access the same address (register) */
    pTrans[1].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_BYTE : WP_BYTE; /* 8 bit write  */
    pTrans[1].Data.Byte = ALL_INT_MASK;

    /* Store the diag interrupt handler routine, which will be executed by
       PCMCIA_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;
    
    /* Enable the interrupts */
    dwStatus = WDC_IntEnable(hDev, pTrans, NUM_TRANS_CMDS, INTERRUPT_CMD_COPY,
        PCMCIA_IntHandler, (PVOID)pDev, FALSE);
        
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        
        free(pTrans);
        
        return dwStatus;
    }

    /* Store the interrupt transfer commands in the device context */
    pDevCtx->pIntTransCmds = pTrans;

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    TraceLog("PCMCIA_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

DWORD PCMCIA_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCMCIA_DEV_CTX pDevCtx;

    TraceLog("PCMCIA_IntDisable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PCMCIA_IntDisable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);
 
    if (!WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already disabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* TODO: You can add code here to write to the device in order
             to physically disable the hardware interrupts */

    /* Disable the interrupts */
    dwStatus = WDC_IntDisable(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    /* Free the memory allocated for the interrupt transfer commands */
    if (pDevCtx->pIntTransCmds)
    {
        free(pDevCtx->pIntTransCmds);
        pDevCtx->pIntTransCmds = NULL;
    }

    return dwStatus;
}

BOOL PCMCIA_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCMCIA_IntIsEnabled"))
        return FALSE;

    return WDC_IntIsEnabled(hDev);
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void PCMCIA_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCMCIA_DEV_CTX pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);
    /* TODO: If you passed data to the callback in PCMCIA_EventRegister() (see
             the call to WDC_EventRegister()), cast pData to the relevant data
             type and use the data as needed */

    TraceLog("PCMCIA_EventHandler entered. pData 0x%p, dwAction 0x%lx\n",
        pData, pEvent->dwAction);
    
    /* TODO: Add code here to identify the event that was received
             (dwAction & WD_EVENT_ACTION value) and handle it */

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

DWORD PCMCIA_EventRegister(WDC_DEVICE_HANDLE hDev, PCMCIA_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCMCIA_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h */

    TraceLog("PCMCIA_EventRegister entered. Device handle: 0x%p\n", hDev);
    
    if (!IsValidDevice(pDev, "PCMCIA_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCMCIA_DEV_CTX)(pDev->pCtx);

    /* Check if event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from PCMCIA_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register event */
    dwStatus = WDC_EventRegister(hDev, dwActions, PCMCIA_EventHandler, NULL, FALSE);
    
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");

    return WD_STATUS_SUCCESS;
}

DWORD PCMCIA_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    
    TraceLog("PCMCIA_EventUnregister entered. Device handle: 0x%p\n", hDev);
    
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCMCIA_EventUnregister"))
        return WD_INVALID_PARAMETER;

    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = WDC_EventUnregister(hDev);
    
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to unregister events. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

BOOL PCMCIA_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCMCIA_EventIsRegistered"))
        return FALSE;

    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
DWORD PCMCIA_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    
    if (!IsValidDevice(pDev, "PCMCIA_GetNumAddrSpaces"))
        return 0;

    return pDev->dwNumAddrSpaces;
}

BOOL PCMCIA_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev, PCMCIA_ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace, dwMaxAddrSpace;
    BOOL fIsMemory;
    
    if (!IsValidDevice(pDev, "PCMCIA_GetAddrSpaceInfo"))
        return FALSE;

#if defined(DEBUG)
    if (!pAddrSpaceInfo)
    {
        ErrLog("PCMCIA_GetAddrSpaceInfo: Error - NULL address space information pointer\n");
        return FALSE;
    }
#endif

    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;
    dwMaxAddrSpace = pDev->dwNumAddrSpaces - 1;

    if (dwAddrSpace > dwMaxAddrSpace)
    {
        ErrLog("PCMCIA_GetAddrSpaceInfo: Error - Address space %ld is out of range (0 - %ld)\n",
            dwAddrSpace, dwMaxAddrSpace);
        return FALSE;
    }

    pAddrDesc = &pDev->pAddrDesc[dwAddrSpace];

    fIsMemory = WDC_ADDR_IS_MEM(pAddrDesc);
    
    snprintf(pAddrSpaceInfo->sName, MAX_NAME - 1, "BAR %ld", dwAddrSpace);
    snprintf(pAddrSpaceInfo->sType, MAX_TYPE - 1, fIsMemory ? "Memory" : "I/O");
        
    if (WDC_AddrSpaceIsActive(pDev, dwAddrSpace))
    {
        WD_ITEMS *pItem = &pDev->cardReg.Card.Item[pAddrDesc->dwItemIndex];
        DWORD dwAddr = fIsMemory ? pItem->I.Mem.dwPhysicalAddr : (DWORD)pItem->I.IO.dwAddr;
        
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1, "0x%0*lX - 0x%0*lX (%ld bytes)",
            (int)WDC_SIZE_32 * 2, dwAddr,
            (int)WDC_SIZE_32 * 2, dwAddr + pAddrDesc->dwBytes - 1,
            pAddrDesc->dwBytes);
    }
    else
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1, "Inactive address space");

    /* TODO: You can modify the code above to set a different address space name/description */

    return TRUE;
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(gsPCMCIA_LastErr, sizeof(gsPCMCIA_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
    WDC_Err("PCMCIA lib: %s", gsPCMCIA_LastErr);
#endif
    va_end(argp);
}

static void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("PCMCIA lib: %s", sMsg);
    va_end(argp);
#endif
}

const char *PCMCIA_GetLastErr(void)
{
    return gsPCMCIA_LastErr;
}

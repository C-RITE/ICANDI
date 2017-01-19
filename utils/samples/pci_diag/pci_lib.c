/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/************************************************************************
*  File: pci_lib.c
*
*  Library for accessing PCI devices, possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#if defined (__KERNEL__)
    #include "kpstdlib.h"
#else
    #include <stdio.h>
    #include <stdarg.h>
#endif
#include "wdc_defs.h"
#include "utils.h"
#include "status_strings.h"
#include "samples/shared/pci_regs.h"
#include "pci_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with your specific WinDriver license registration string and
         the driver name below with your driver's name */
#define PCI_DEFAULT_LICENSE_STRING "12345abcde1234.license"
#define PCI_DEFAULT_DRIVER_NAME "windrvr6"

/* PCI device information struct */
typedef struct {
    WD_TRANSFER      *pIntTransCmds;
    PCI_INT_HANDLER   funcDiagIntHandler;
    PCI_EVENT_HANDLER funcDiagEventHandler;
} PCI_DEV_CTX, *PPCI_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information */

/*************************************************************
  Global variables definitions
 *************************************************************/
/* String for storing last error information */
static CHAR gsPCI_LastErr[256];

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
#if !defined (__KERNEL__)
static BOOL DeviceValidate(const PWDC_DEVICE pDev);
#endif
static void DLLCALLCONV PCI_IntHandler(PVOID pData);
static void PCI_EventHandler(WD_EVENT *pEvent, PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PPCI_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsPCI_LastErr, sizeof(gsPCI_LastErr) - 1,
            "%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        ErrLog(gsPCI_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    PCI and WDC library initialize/uninit
   ----------------------------------------------- */
DWORD PCI_LibInit(void)
{
    DWORD dwStatus;

#if defined(WD_DRIVER_NAME_CHANGE)
    /* Set the driver name */
    if (!WD_DriverName(PCI_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.\n");
        return WD_SYSTEM_INTERNAL_ERROR;
    }
#endif

    /* Set WDC library's debug options 
     * (default: level TRACE, output to Debug Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        
        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, PCI_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

DWORD PCI_LibUninit(void)
{
    DWORD dwStatus;

    /* Uninit the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to uninit the WDC library. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

#if !defined(__KERNEL__)
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
WDC_DEVICE_HANDLE PCI_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo)
{
    DWORD dwStatus;
    PPCI_DEV_CTX pDevCtx = NULL;
    WDC_DEVICE_HANDLE hDev = NULL;

    /* Validate arguments */
    if (!pDeviceInfo)
    {
        ErrLog("PCI_DeviceOpen: Error - NULL device information "
            "struct pointer\n");
        return NULL;
    }

    /* Allocate memory for the PCI device context */
    pDevCtx = (PPCI_DEV_CTX)malloc(sizeof (PCI_DEV_CTX));
    if (!pDevCtx)
    {
        ErrLog("Failed allocating memory for PCI device context\n");
        return NULL;
    }

    BZERO(*pDevCtx);

    /* Open a Kernel PlugIn WDC device handle */
    dwStatus = WDC_PciDeviceOpen(&hDev, pDeviceInfo, pDevCtx, NULL,
        KP_PCI_DRIVER_NAME, &hDev);
    
    /* if failed, try opening a WDC device handle without using Kernel PlugIn */
    if(dwStatus != WD_STATUS_SUCCESS)
    {
        dwStatus = WDC_PciDeviceOpen(&hDev, pDeviceInfo, pDevCtx, NULL, NULL,
            NULL);
    }   
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
    TraceLog("PCI_DeviceOpen: Opened a PCI device (handle 0x%p)\n"
        "Device is %s using a Kernel PlugIn driver (%s)\n", hDev,
        (WDC_IS_KP(hDev))? "" : "not" , KP_PCI_DRIVER_NAME);
    return hDev;

Error:    
    if (hDev)
        PCI_DeviceClose(hDev);
    else
        free(pDevCtx);
    
    return NULL;
}

BOOL PCI_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;
    
    TraceLog("PCI_DeviceClose entered. Device handle: 0x%p\n", hDev);

    if (!hDev)
    {
        ErrLog("PCI_DeviceClose: Error - NULL device handle\n");
        return FALSE;
    }

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);
    
    /* Disable interrupts */
    if (WDC_IntIsEnabled(hDev))
    {
        dwStatus = PCI_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }

    /* Close the device */
    dwStatus = WDC_PciDeviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed closing a WDC device handle (0x%p). Error 0x%lx - %s\n",
            hDev, dwStatus, Stat2Str(dwStatus));
    }

    /* Free PCI device context memory */
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
static void DLLCALLCONV PCI_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCI_DEV_CTX pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);
    PCI_INT_RESULT intResult;

    BZERO(intResult);
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
    intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev);
    intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);
    
    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

DWORD PCI_IntEnable(WDC_DEVICE_HANDLE hDev, PCI_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;
    WDC_ADDR_DESC *pAddrDesc;
    WD_TRANSFER *pTrans = NULL;
    DWORD dwNumTransCmds = 0;
    DWORD dwOptions = 0;

    TraceLog("PCI_IntEnable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PCI_IntEnable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    /* Check if interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* In case of kernel plugin, interrupts acknowledged in kernel mode */
    if (!WDC_IS_KP(pDev))
    {
        /* TODO: Change this value, if needed */
        dwNumTransCmds = 2;

        /* This sample demonstrates how to set up two transfer commands, one for
         * reading the device's INTCSR register (as defined in gPCI_Regs) and
         * one for writing to it to acknowledge the interrupt. The transfer
         * commands will be executed by WinDriver in the kernel when an
         * interrupt occurs.
         * TODO: If PCI interrupts are level sensitive interrupts, they must be
         * acknowledged in the kernel immediately when they are received.  Since
         * the information for acknowledging the interrupts is
         * hardware-specific, YOU MUST MODIFY THE CODE below and set up transfer
         * commands in order to correctly acknowledge the interrupts on your
         * device, as dictated by your hardware's specifications.
         * If device supports both MSI and level sensitive interrupts, you must
         * set up transfer commands in order to make your code runable on
         * systems other then Windows Vista.
         * Otherwise, you might not to define the transfer commands at all, or
         * specify the commands to be performed upon interrupt generation
         * according to your needs.

         ******************************************************************   
         * NOTE: If you attempt to use this code without first modifying it in
         * order to correctly acknowledge your device's level-sensitive
         * interrupts, as explained above, the OS will HANG when an interrupt
         * occurs!
         ********************************************************************/

        /* Allocate memory for the interrupt transfer commands */
        pTrans = (WD_TRANSFER*)calloc(dwNumTransCmds, sizeof(WD_TRANSFER));
        if (!pTrans)
        {
            ErrLog("Failed allocating memory for interrupt transfer "
                "commands\n");
            return WD_INSUFFICIENT_RESOURCES;
        }

        /* Prepare the interrupt transfer commands.
         *
         * The transfer commands will be executed by WinDriver's ISR
         * which runs in kernel mode at interrupt level.
         */

        /* TODO: Change the offset of INTCSR and the PCI address space, if
         * needed */
        /* #1: Read status from the INTCSR register */
        pAddrDesc = WDC_GET_ADDR_DESC(pDev, INTCSR_ADDR_SPACE);

        pTrans[0].dwPort = pAddrDesc->kptAddr + INTCSR;
        /* read 32bit register */
        pTrans[0].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_DWORD : RP_DWORD; 
    
        /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
         * interrupt */
        pTrans[1].dwPort = pTrans[0].dwPort; /* In this example both commands
                                                access the same address
                                                (register) */
        /* write 32bit register */
        pTrans[1].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_DWORD : WP_DWORD; 
        pTrans[0].Data.Dword = ALL_INT_MASK;

        /* copy the results of "read" transfer commands back to user mode */
        dwOptions = INTERRUPT_CMD_COPY;
    }
    /* Store the diag interrupt handler routine, which will be executed by
       PCI_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;
    
    /* Enable the interrupts */
    dwStatus = WDC_IntEnable(hDev, pTrans, dwNumTransCmds, dwOptions,
        PCI_IntHandler, (PVOID)pDev, WDC_IS_KP(pDev));
        
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        
        if (pTrans)
            free(pTrans);
        
        return dwStatus;
    }

    /* Store the interrupt transfer commands in the device context */
    pDevCtx->pIntTransCmds = pTrans;

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    TraceLog("PCI_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

DWORD PCI_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;

    TraceLog("PCI_IntDisable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PCI_IntDisable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);
 
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

BOOL PCI_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_IntIsEnabled"))
        return FALSE;

    return WDC_IntIsEnabled(hDev);
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void PCI_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCI_DEV_CTX pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    TraceLog("PCI_EventHandler entered, pData 0x%p, dwAction 0x%lx\n",
        pData, pEvent->dwAction);
    
    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

DWORD PCI_EventRegister(WDC_DEVICE_HANDLE hDev,
    PCI_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h */

    TraceLog("PCI_EventRegister entered. Device handle: 0x%p\n", hDev);
    
    if (!IsValidDevice(pDev, "PCI_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    /* Check if event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * PCI_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register event */
    dwStatus = WDC_EventRegister(hDev, dwActions, PCI_EventHandler, hDev,
        WDC_IS_KP(hDev));
    
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");

    return WD_STATUS_SUCCESS;
}

DWORD PCI_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    
    TraceLog("PCI_EventUnregister entered. Device handle: 0x%p\n", hDev);
    
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_EventUnregister"))
        return WD_INVALID_PARAMETER;

    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently "
            "registered ...\n");
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

BOOL PCI_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_EventIsRegistered"))
        return FALSE;

    return WDC_EventIsRegistered(hDev);
}
#endif

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
DWORD PCI_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    
    if (!IsValidDevice(pDev, "PCI_GetNumAddrSpaces"))
        return 0;

    return pDev->dwNumAddrSpaces;
}

BOOL PCI_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    PCI_ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace;
    BOOL fIsMemory;
    
    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;

    if (dwAddrSpace > pDev->dwNumAddrSpaces - 1)
    {
        ErrLog("PCI_GetAddrSpaceInfo: Error - Address space %ld is "
            "out of range (0 - %ld)\n", dwAddrSpace, pDev->dwNumAddrSpaces - 1);
        return FALSE;
    }

    pAddrDesc = &pDev->pAddrDesc[dwAddrSpace];

    fIsMemory = WDC_ADDR_IS_MEM(pAddrDesc);
    
    snprintf(pAddrSpaceInfo->sName, MAX_NAME - 1, "BAR %ld", dwAddrSpace);
    snprintf(pAddrSpaceInfo->sType, MAX_TYPE - 1, fIsMemory ? "Memory" : "I/O");
        
    if (WDC_AddrSpaceIsActive(pDev, dwAddrSpace))
    {
        WD_ITEMS *pItem = &pDev->cardReg.Card.Item[pAddrDesc->dwItemIndex];
        DWORD dwAddr = fIsMemory ? pItem->I.Mem.dwPhysicalAddr :
            (DWORD)pItem->I.IO.dwAddr;
        
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1,
            "0x%0*lX - 0x%0*lX (0x%lx bytes)",
            (int)WDC_SIZE_32 * 2, dwAddr,
            (int)WDC_SIZE_32 * 2, dwAddr + pAddrDesc->dwBytes - 1,
            pAddrDesc->dwBytes);
    }
    else
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1, "Inactive address space");

    /* TODO: You can modify the code above to set a different address space
     * name/description */

    return TRUE;
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(gsPCI_LastErr, sizeof(gsPCI_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
#if defined (__KERNEL__)
    WDC_Err("KP PCI lib: %s", gsPCI_LastErr);
#else
    WDC_Err("PCI lib: %s", gsPCI_LastErr);
#endif
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
#if defined (__KERNEL__)
    WDC_Trace("KP PCI lib: %s", sMsg);
#else
    WDC_Trace("PCI lib: %s", sMsg);
#endif
    va_end(argp);
#endif
}

const char *PCI_GetLastErr(void)
{
    return gsPCI_LastErr;
}


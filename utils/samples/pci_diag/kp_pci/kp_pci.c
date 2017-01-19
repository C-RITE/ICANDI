/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/************************************************************************
*  File: kp_pci.c
*
*  Kernel PlugIn driver for accessing PCI devices.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#include "kpstdlib.h"
#include "wd_kp.h"
#include "utils.h"
#include "wdc_defs.h"
#include "samples/shared/bits.h"
#include "samples/shared/pci_regs.h"
#include "../pci_lib.h"

/*************************************************************
  Functions prototypes
 *************************************************************/
BOOL __cdecl KP_PCI_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext);
void __cdecl KP_PCI_Close(PVOID pDrvContext);
void __cdecl KP_PCI_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    BOOL fIsKernelMode);
BOOL __cdecl KP_PCI_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext);
void __cdecl KP_PCI_IntDisable(PVOID pIntContext);
BOOL __cdecl KP_PCI_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt);
DWORD __cdecl KP_PCI_IntAtDpc(PVOID pIntContext, DWORD dwCount);
BOOL __cdecl KP_PCI_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved);
DWORD __cdecl KP_PCI_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved);
BOOL __cdecl KP_PCI_Event(PVOID pDrvContext, WD_EVENT *wd_event);
static void KP_PCI_Err(const CHAR *sFormat, ...);
static void KP_PCI_Trace(const CHAR *sFormat, ...);

/*************************************************************
  Functions implementation
 *************************************************************/

/* KP_Init is called when the Kernel PlugIn driver is loaded.
   This function sets the name of the Kernel PlugIn driver and the driver's
   open callback function. */
BOOL __cdecl KP_Init(KP_INIT *kpInit)
{
    /* Verify that the version of the WinDriver Kernel PlugIn library
       is identical to that of the windrvr.h and wd_kp.h files */
    if (WD_VER != kpInit->dwVerWD)
    {
        /* Re-build your Kernel PlugIn driver project with the compatible
           version of the WinDriver Kernel PlugIn library (kp_nt<version>.lib)
           and windrvr.h and wd_kp.h files */

        return FALSE;
    }

    kpInit->funcOpen = KP_PCI_Open;
    strcpy (kpInit->cDriverName, KP_PCI_DRIVER_NAME);

    return TRUE;
}

/* KP_PCI_Open is called when WD_KernelPlugInOpen() is called from the user mode.
   pDrvContext will be passed to the rest of the Kernel PlugIn callback functions. */
BOOL __cdecl KP_PCI_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext)
{
    PWDC_DEVICE pDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwSize, dwStatus;
    void *temp;

    KP_PCI_Trace("KP_PCI_Open entered\n");
    
    kpOpenCall->funcClose = KP_PCI_Close;
    kpOpenCall->funcCall = KP_PCI_Call;
    kpOpenCall->funcIntEnable = KP_PCI_IntEnable;
    kpOpenCall->funcIntDisable = KP_PCI_IntDisable;
    kpOpenCall->funcIntAtIrql = KP_PCI_IntAtIrql;
    kpOpenCall->funcIntAtDpc = KP_PCI_IntAtDpc;
    kpOpenCall->funcIntAtIrqlMSI = KP_PCI_IntAtIrqlMSI; 
    kpOpenCall->funcIntAtDpcMSI = KP_PCI_IntAtDpcMSI;
    kpOpenCall->funcEvent = KP_PCI_Event;

    /* Initialize the PCI library */
    dwStatus = PCI_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_PCI_Err("KP_PCI_Open: Failed to initialize the PCI library: %s",
            PCI_GetLastErr());
        return FALSE;
    }

    /* Create a copy of device information in the driver context */
    dwSize = sizeof(WDC_DEVICE);
    pDev = malloc(dwSize);
    if (!pDev)
        goto malloc_error;

    COPY_FROM_USER(&temp, pOpenData, sizeof(void *));
    COPY_FROM_USER(pDev, temp, dwSize);

    dwSize = sizeof(WDC_ADDR_DESC) * pDev->dwNumAddrSpaces;
    pAddrDesc = malloc(dwSize);
    if (!pAddrDesc)
        goto malloc_error;
    
    COPY_FROM_USER(pAddrDesc, pDev->pAddrDesc, dwSize);
    pDev->pAddrDesc = pAddrDesc;

    *ppDrvContext = pDev;

    KP_PCI_Trace("KP_PCI_Open: Kernel PlugIn driver opened successfully\n");
    
    return TRUE;
    
malloc_error:
    KP_PCI_Err("KP_PCI_Open: Failed allocating %ld bytes\n", dwSize);
    PCI_LibUninit();
    return FALSE;
}

/* KP_PCI_Close is called when WD_KernelPlugInClose() is called from the user mode */
void __cdecl KP_PCI_Close(PVOID pDrvContext)
{
    DWORD dwStatus;

    KP_PCI_Trace("KP_PCI_Close entered\n");
    
    /* Uninit the PCI library */
    dwStatus = PCI_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_PCI_Err("KP_PCI_Close: Failed to uninit the PCI library: %s",
            PCI_GetLastErr());
    }

    /* Free the memory allocated for the driver context */
    if (pDrvContext)
    {
        free(((PWDC_DEVICE)pDrvContext)->pAddrDesc);
        free(pDrvContext);  
    }
}

/* KP_PCI_Call is called when WD_KernelPlugInCall() is called from the user mode */
void __cdecl KP_PCI_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    BOOL fIsKernelMode)
{
    KP_PCI_Trace("KP_PCI_Call entered. Message: 0x%lx\n", kpCall->dwMessage);
    
    kpCall->dwResult = KP_PCI_STATUS_OK;
    
    switch (kpCall->dwMessage)
    {
    case KP_PCI_MSG_VERSION: /* Get the version of the Kernel PlugIn driver */
        {
            DWORD dwVer = 100;
            KP_PCI_VERSION *pUserKPVer = (KP_PCI_VERSION *)(kpCall->pData);
            
            COPY_TO_USER_OR_KERNEL(&pUserKPVer->dwVer, &dwVer, sizeof(DWORD),
                fIsKernelMode);
            COPY_TO_USER_OR_KERNEL(pUserKPVer->cVer, "My Driver V1.00",
                sizeof("My Driver V1.00") + 1, fIsKernelMode);
            kpCall->dwResult = KP_PCI_STATUS_OK;
        }
        break ;
    default:
        kpCall->dwResult = KP_PCI_STATUS_MSG_NO_IMPL;
    }

    /* NOTE: You can modify the messages above and/or add your own
             Kernel PlugIn messages.
             When changing/adding messages, be sure to also update the
             messages definitions in ../pci_lib.h. */
}

/* KP_PCI_IntEnable is called when WD_IntEnable() is called from the user mode
   with a Kernel PlugIn handle.
   The interrupt context (pIntContext) will be passed to the rest of the
   Kernel PlugIn interrupt functions.
   The function returns TRUE if interrupts are enabled successfully. */
BOOL __cdecl KP_PCI_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext)
{
    KP_PCI_Trace("KP_PCI_IntEnable entered\n");
    
    /* You can allocate specific memory for each interrupt in *ppIntContext */
    
    /* In this sample we will set the interrupt context to the driver context,
       which has been set in KP_PCI_Open() to hold the device information. */
    *ppIntContext = pDrvContext;

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    return TRUE;
}

/* KP_PCI_IntDisable is called when WD_IntDisable() is called from the
   user mode with a Kernel PlugIn handle */
void __cdecl KP_PCI_IntDisable(PVOID pIntContext)
{
    /* Free any memory allocated in KP_PCI_IntEnable() here */
}

/* KP_PCI_IntAtIrql returns TRUE if deferred interrupt processing (DPC) for
   level-sensitive interrupt is needed.
   The function is called at HIGH IRQL - at physical interrupt handler.
   Most library calls are NOT allowed at this level, for example:
   NO   WDC_xxx() or WD_xxx calls, apart from the WDC read/write address or
        register functions, WDC_MultiTransfer(), WD_Transfer(),
        WD_MultiTransfer() or WD_DebugAdd().
   NO   malloc().
   NO   free().
   YES  WDC read/write address or configuration space functions, 
        WDC_MultiTransfer(), WD_Transfer(), WD_MultiTransfer() or 
        WD_DebugAdd(), or wrapper functions that call these functions.
   YES  specific kernel OS functions (such as WinDDK functions) that can
        be called from HIGH IRQL. [Note that the use of such functions may
        break the code's portability to other OSs.] */
BOOL __cdecl KP_PCI_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt)
{
    static DWORD dwIntCount = 0; /* Interrupts count */
    PWDC_DEVICE pDev = (PWDC_DEVICE)pIntContext;
    WDC_ADDR_DESC *pAddrDesc;
    /* Define the number of interrupt transfer commands to use */
    WD_TRANSFER trans[2];
    
    /*
       This sample demonstrates how to set up two transfer commands, one for
       reading the device's INTCSR register (as defined in gPCI_Regs) and one
       for writing to it to acknowledge the interrupt.
       
       TODO: PCI interrupts are level sensitive interrupts and must be
             acknowledged in the kernel immediately when they are received.
             Since the information for acknowledging the interrupts is
             hardware-specific, YOU MUST MODIFY THE CODE below and set up
             transfer commands in order to correctly acknowledge the interrupts
             on your device, as dictated by your hardware's specifications.
             
       *************************************************************************
       * NOTE: If you attempt to use this code without first modifying it in   *
       *       order to correctly acknowledge your device's interrupts, as     *
       *       explained above, the OS will HANG when an interrupt occurs!     *
       *************************************************************************
    */

    BZERO(trans);

    /* Prepare the interrupt transfer commands */

    /* #1: Read status from the INTCSR register */
    pAddrDesc = &pDev->pAddrDesc[INTCSR_ADDR_SPACE];
    trans[0].dwPort = pAddrDesc->kptAddr + INTCSR;
    /* 32bit read: */
    trans[0].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_DWORD : RP_DWORD;
    
    /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
           interrupt */
    /* In this example both commands access the same address (register): */
    trans[1].dwPort = trans[0].dwPort;
    /* 32bit write: */
    trans[1].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_DWORD : WP_DWORD;
    trans[1].Data.Dword = ALL_INT_MASK;

    /* NOTE: For memory registers you can replace the use of WDC_MultiTransfer()
       (or any other WD_xxx/WDC_xxx read/write function call) with direct
       memory access. For example, if INTCSR is a memory register, the code
       above can be replaced with the following: */
    /*
    // In the variables declaration section (above), remove the trans array
    // declaration and replace it with the following variables declarations:
    UINT32 readData;
    PVOID pData;

    // then replace the transfer code above with the following:

    pAddrDesc = &pDev->pAddrDesc[INTCSR_ADDR_SPACE];
    pData = (DWORD*)(pAddrDesc->kptAddr + INTCSR);

    // Read status from the PCI_INTCSR register
    readData = WDC_ReadMem32(pData, 0);

    // Write to the PCI_INTCSR register to acknowledge the interrupt
    WDC_WriteMem32(pData, 0, ALL_INT_MASK);
    */

    /* Execute the transfer commands */
    WDC_MultiTransfer(trans, 2);

    /* If the data read from the hardware indicates that the interrupt belongs
       to you, you must set *pfIsMyInterrupt to TRUE.
       Otherwise, set it to FALSE (this will let ISR's of other drivers be
       invoked). */
    *pfIsMyInterrupt = FALSE;

    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;
    
    return FALSE;
}

/* KP_PCI_IntAtDpc is a Deferred Procedure Call for additional level-sensitive
   interupt processing. This function is called if KP_PCI_IntAtIrql returned
   TRUE. KP_PCI_IntAtDpc returns the number of times to notify the user mode of
   the interrupt (i.e. return from WD_IntWait) */
DWORD __cdecl KP_PCI_IntAtDpc(PVOID pIntContext, DWORD dwCount)
{
    return dwCount;
}

/* KP_PCI_IntAtIrqlMSI returns TRUE if deferred interrupt processing (DPC) for
   message signalled interupt (MSI/MSI-X) is needed.
   The function is called at HIGH IRQL - at physical interrupt handler.
   Do not use dwReserver value. 
   Most library calls are NOT allowed at this level, for example:
   NO   WDC_xxx() or WD_xxx calls, apart from the WDC read/write address or
        register functions, WDC_MultiTransfer(), WD_Transfer(),
        WD_MultiTransfer() or WD_DebugAdd().
   NO   malloc().
   NO   free().
   YES  WDC read/write address or configuration space functions, 
        WDC_MultiTransfer(), WD_Transfer(), WD_MultiTransfer() or 
        WD_DebugAdd(), or wrapper functions that call these functions.
   YES  specific kernel OS functions (such as WinDDK functions) that can
        be called from HIGH IRQL. [Note that the use of such functions may
        break the code's portability to other OSs.] */
BOOL __cdecl KP_PCI_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved)
{
    static DWORD dwIntCount = 0; /* Interrupts count */

    /* There is no need to acknowledge message signalled interrupts. However,
     * you can implement here the same functionality as for KP_PCI_IntAtIrql
     * handler to read/write data from/to registeres. */

    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;
    
    return FALSE;
}

/* KP_PCI_IntAtDpcMSI is a Deferred Procedure Call for additional message
   signalled interupt (MSI/MSI-X) processing. This function is called if
   KP_PCI_IntAtIrqlMSI returned TRUE. KP_PCI_IntAtDpcMSI returns the number of
   times to notify the user mode of the interrupt (i.e. return from
   WD_IntWait) */
DWORD __cdecl KP_PCI_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved)
{
    return dwCount;
}

/* KP_PCI_Event() is called when a Plug-and-Play/power management event for
   the device is received, if EventRegister() was first called from the
   user mode with the KernelPlugin handle. */
BOOL __cdecl KP_PCI_Event(PVOID pDrvContext, WD_EVENT *wd_event)
{
    return TRUE; /* Return TRUE to notify the user mode of the event */
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void KP_PCI_Err(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Err("%s: %s", KP_PCI_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

static void KP_PCI_Trace(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("%s: %s", KP_PCI_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

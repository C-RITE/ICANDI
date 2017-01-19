/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/******************************************************************
 *
 * This is a diagnostics application for accessing the USB device.
 * The code accesses the hardware via WinDriver functions.
 * 
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "wdu_lib.h"
#include "status_strings.h"
#include "utils.h"
#include "samples/shared/usb_diag_lib.h"

#if defined(USB_DIAG_SAMPLE)

/* TODO: change the following definitions to match your device. */
#define DEFAULT_VENDOR_ID 0x1234
#define DEFAULT_PRODUCT_ID 0x5678 
#define DEFAULT_LICENSE_STRING "12345abcde1234.license"

#else

/* use in wizard's device-specific generated code */
#define DEFAULT_VENDOR_ID         %VID%
#define DEFAULT_PRODUCT_ID        %PID%
#define DEFAULT_LICENSE_STRING    "%LICENSE%"

#endif

/* TODO: change the following definition to your driver's name */
#define DEFAULT_DRIVER_NAME "windrvr6"

#define USE_DEFAULT 0xffff
#define ATTACH_EVENT_TIMEOUT 30 /* in seconds */
#define TRANSFER_TIMEOUT 30000 /* in msecs */
#if !defined(TRACE)
#define TRACE printf
#endif
#if !defined(ERR)
#define ERR printf
#endif

typedef struct DEVICE_CONTEXT
{
    struct DEVICE_CONTEXT *pNext;
    WDU_DEVICE_HANDLE hDevice;
    DWORD dwVendorId;
    DWORD dwProductId;
    DWORD dwInterfaceNum;
    DWORD dwAlternateSetting;
} DEVICE_CONTEXT;

typedef struct DRIVER_CONTEXT
{
    HANDLE hEvent;
    HANDLE hMutex;
    DWORD dwDeviceCount;
    DEVICE_CONTEXT *deviceContextList;
    DEVICE_CONTEXT *pActiveDev;
    HANDLE hDeviceUnusedEvent;
} DRIVER_CONTEXT;

static char line[250];
static WDU_DRIVER_HANDLE hDriver = 0;

static BOOL DLLCALLCONV DeviceAttach(WDU_DEVICE_HANDLE hDevice, 
    WDU_DEVICE *pDeviceInfo, PVOID pUserData)
{
    DRIVER_CONTEXT *pDrvCtx = (DRIVER_CONTEXT *)pUserData;
    DEVICE_CONTEXT *pDevCtx, **ppDevCtx;
    DWORD dwInterfaceNum = pDeviceInfo->pActiveInterface[0]->pActiveAltSetting->Descriptor.bInterfaceNumber;
    DWORD dwAlternateSetting = pDeviceInfo->pActiveInterface[0]->pActiveAltSetting->Descriptor.bAlternateSetting;
    
    /*
    // NOTE: To change the alternate setting, call WDU_SetInterface() here 
    DWORD dwAttachError;

    // TODO: replace with the requested number:
    dwAlternateSetting = %alternate_setting_number%; 

    dwAttachError = WDU_SetInterface(hDevice, dwInterfaceNum, 
        dwAlternateSetting);
    if (dwAttachError)
    {
        ERR("DeviceAttach: WDU_SetInterface failed (num. %ld, alternate %ld) "
            "device 0x%p: error 0x%lx (\"%s\")\n",
            dwInterfaceNum, dwAlternateSetting, hDevice, 
            dwAttachError, Stat2Str(dwAttachError));

        return FALSE;
    }
    */

    TRACE("\nDeviceAttach: received and accepted attach for vendor id 0x%x, "
        "product id 0x%x, interface %ld, device handle 0x%p\n",
        pDeviceInfo->Descriptor.idVendor, pDeviceInfo->Descriptor.idProduct,
        dwInterfaceNum, hDevice);
    
    /* Add our device to the device list */
    pDevCtx = (DEVICE_CONTEXT *)malloc(sizeof(DEVICE_CONTEXT));
    if (!pDevCtx)
    {
        ERR("DeviceAttach: failed allocating memory\n");
        return FALSE;
    }
    BZERO(*pDevCtx);
    pDevCtx->hDevice = hDevice;
    pDevCtx->dwInterfaceNum = dwInterfaceNum;
    pDevCtx->dwVendorId = pDeviceInfo->Descriptor.idVendor;
    pDevCtx->dwProductId = pDeviceInfo->Descriptor.idProduct;
    pDevCtx->dwAlternateSetting = dwAlternateSetting;
    
    OsMutexLock(pDrvCtx->hMutex);
    for (ppDevCtx = &pDrvCtx->deviceContextList; *ppDevCtx; 
        ppDevCtx = &((*ppDevCtx)->pNext));
    *ppDevCtx = pDevCtx;
    pDrvCtx->dwDeviceCount++;
    OsMutexUnlock(pDrvCtx->hMutex);
    
    OsEventSignal(pDrvCtx->hEvent);
    /* Accept control over this device */
    return TRUE;
}

static VOID DLLCALLCONV DeviceDetach(WDU_DEVICE_HANDLE hDevice, PVOID pUserData)
{
    DRIVER_CONTEXT *pDrvCtx = (DRIVER_CONTEXT *)pUserData;
    DEVICE_CONTEXT **pCur;
    DEVICE_CONTEXT *pTmpDev;
    BOOL bDetachActiveDev = FALSE;

    TRACE("\nDeviceDetach: received detach for device handle 0x%p\n", hDevice);

    OsMutexLock(pDrvCtx->hMutex);
    for (pCur = &pDrvCtx->deviceContextList; 
        *pCur && (*pCur)->hDevice != hDevice; 
        pCur = &((*pCur)->pNext));
    
    if (*pCur == pDrvCtx->pActiveDev)
    {
        bDetachActiveDev = TRUE;
        pDrvCtx->pActiveDev = NULL;
    }

    pTmpDev = *pCur;
    *pCur = pTmpDev->pNext;
    free(pTmpDev);
    
    pDrvCtx->dwDeviceCount--;
    OsMutexUnlock(pDrvCtx->hMutex);

    if (bDetachActiveDev)
    {
        /* When hDeviceUnusedEvent is not signaled, hDevice is possibly in use, 
         * and therefore the detach callback needs to wait on it until it is 
         * certain that it cannot be used.
         * When it is signaled - hDevice is no longer used. */
        OsEventWait(pDrvCtx->hDeviceUnusedEvent, INFINITE);
    }
}

static void DeviceDiagMenu(DRIVER_CONTEXT *pDrvCtx)
{
    DWORD cmd;
    DWORD dwPipeNum;
    DWORD dwInterfaceNumber, dwAlternateSetting;
    DWORD dwError;
    WDU_DEVICE_HANDLE hDevice;

    do {
        if (!pDrvCtx->dwDeviceCount)
        {
            printf("\n");
            printf("No Devices are currently connected.\n");
            printf("Press Enter to re check or enter EXIT to exit\n");
            fgets(line, sizeof(line), stdin);
            /* Removing the '\n' character from the end */
            line[strlen(line)-1] = '\0';

            if (!stricmp(line, "EXIT"))
                break;

            continue;
        }

        OsMutexLock(pDrvCtx->hMutex);
        if (!pDrvCtx->dwDeviceCount)
        {
            OsMutexUnlock(pDrvCtx->hMutex);
            continue;
        }
        
        if (!pDrvCtx->pActiveDev)
            pDrvCtx->pActiveDev = pDrvCtx->deviceContextList;
        
        printf("\n");
        printf("Main Menu (active Dev/Prod/Interface/Alt. Setting: "
            "0x%lx/0x%lx/%ld/%ld)\n",
            pDrvCtx->pActiveDev->dwVendorId, pDrvCtx->pActiveDev->dwProductId, 
            pDrvCtx->pActiveDev->dwInterfaceNum, 
            pDrvCtx->pActiveDev->dwAlternateSetting);
        printf("----------\n");
        printf("1. Display device configurations\n");
        printf("2. Change interface alternate setting\n");
        printf("3. Reset Pipe\n");
        printf("4. Read/Write from pipes\n");
        if (pDrvCtx->dwDeviceCount > 1)
            printf("5. Select device\n");
        printf("6. Refresh\n");
        printf("99. Exit\n");
        printf("Enter option: ");
        cmd = 0;
        OsMutexUnlock(pDrvCtx->hMutex);

        fgets(line, sizeof(line), stdin);
        sscanf(line, "%ld", &cmd);
        
        if (!pDrvCtx->pActiveDev)
            continue;
        
        OsEventReset(pDrvCtx->hDeviceUnusedEvent);
        
        OsMutexLock(pDrvCtx->hMutex);
        hDevice = pDrvCtx->pActiveDev->hDevice;
        OsMutexUnlock(pDrvCtx->hMutex);
        
        switch (cmd)
        {
        case 1:
            PrintDeviceConfigurations(hDevice);
            break;
        case 2:
            printf("Please enter the interface number (dec): ");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%ld", &dwInterfaceNumber);
            printf("Please enter the alternate setting index (dec): ");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%ld", &dwAlternateSetting);
            dwError = WDU_SetInterface(hDevice, dwInterfaceNumber, 
                dwAlternateSetting);
            if (dwError)
            {
                ERR("DeviceDiagMenu: WDU_SetInterface() failed: error 0x%lx "
                    "(\"%s\")\n", dwError, Stat2Str(dwError));
            }
            else
            {
                TRACE("DeviceDiagMenu: WDU_SetInterface() completed successfully\n");
                pDrvCtx->pActiveDev->dwInterfaceNum = dwInterfaceNumber;
                pDrvCtx->pActiveDev->dwAlternateSetting = dwAlternateSetting;
            }
            break;
        case 3:
            printf("Please enter the pipe number (hex): 0x");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%lx", &dwPipeNum);
            printf("\n");

            dwError = WDU_ResetPipe(hDevice, dwPipeNum);
            if (dwError)
            {
                ERR("DeviceDiagMenu: WDU_ResetPipe() failed: error 0x%lx "
                    "(\"%s\")\n", dwError, Stat2Str(dwError));
            }
            else
                TRACE("DeviceDiagMenu: WDU_ResetPipe() completed successfully\n");
            break;
        case 4:
            ReadWritePipesMenu(hDevice);
            break;
        case 5:
            OsMutexLock(pDrvCtx->hMutex);
            if (pDrvCtx->dwDeviceCount > 1)
            {
                DWORD dwDeviceNum, i;
                DEVICE_CONTEXT *pCur;

                for (i=1, pCur=pDrvCtx->deviceContextList; pCur; 
                    pCur=pCur->pNext, i++)
                {
                    printf("  %ld. Vendor id: 0x%lx, Product id: 0x%lx, "
                        "Interface number: %ld, Alt. Setting: %ld\n", i, 
                        pCur->dwVendorId, pCur->dwProductId, 
                        pCur->dwInterfaceNum, pCur->dwAlternateSetting);
                }

                printf("Please enter the device number (1 - %ld, dec): ", 
                    i - 1);
                fgets(line, sizeof(line), stdin);
                sscanf(line, "%ld", &dwDeviceNum);

                for (pCur=pDrvCtx->deviceContextList, i=1; 
                    pCur && i < dwDeviceNum; 
                    pCur=pCur->pNext, i++);

                pDrvCtx->pActiveDev = pCur;
            }
            OsMutexUnlock(pDrvCtx->hMutex);
            break;      
        case 6:
            break; 
        }
        
        /* finished using hDevice */
        OsEventSignal(pDrvCtx->hDeviceUnusedEvent);
    } while (cmd!=99);
}

DWORD DriverInit(WDU_MATCH_TABLE *pMatchTables, DWORD dwNumMatchTables, 
    const PCHAR sDriverName, const PCHAR sLicense, DRIVER_CONTEXT *pDrvCtx)
{
    DWORD dwError;
    WDU_EVENT_TABLE eventTable;
    
    /* Set Driver Name */
    if (!WD_DriverName(sDriverName))
    {
        ERR("Error: Could not set driver name to %s, exiting\n", 
            sDriverName);
        return WD_SYSTEM_INTERNAL_ERROR;
    }

    dwError = OsEventCreate(&pDrvCtx->hEvent);
    if (dwError)
    {
        ERR("DriverInit: OsEventCreate() failed on event 0x%p: error 0x%lx "
            "(\"%s\")\n", pDrvCtx->hEvent, dwError, Stat2Str(dwError));
        return dwError;
    }

    dwError = OsMutexCreate(&pDrvCtx->hMutex);
    if (dwError)
    {
        ERR("DriverInit: OsMutexCreate() failed on mutex 0x%p: error 0x%lx "
            "(\"%s\")\n", pDrvCtx->hMutex, dwError, Stat2Str(dwError));
        return dwError;
    }
    
    dwError = OsEventCreate(&pDrvCtx->hDeviceUnusedEvent);
    if (dwError)
    {
        ERR("DriverInit: OsEventCreate() failed on event 0x%p: error 0x%lx "
            "(\"%s\")\n", pDrvCtx->hDeviceUnusedEvent, dwError, 
            Stat2Str(dwError));
        return dwError;
    }

    OsEventSignal(pDrvCtx->hDeviceUnusedEvent);
    BZERO(eventTable);
    eventTable.pfDeviceAttach = DeviceAttach;
    eventTable.pfDeviceDetach = DeviceDetach;
    eventTable.pUserData = pDrvCtx;

    dwError = WDU_Init(&hDriver, pMatchTables, dwNumMatchTables, &eventTable, 
        sLicense, WD_ACKNOWLEDGE);
    if (dwError)
    {
        ERR("DriverInit: failed to initialize USB driver: error 0x%lx "
            "(\"%s\")\n", dwError, Stat2Str(dwError));
        return dwError;
    }

    return WD_STATUS_SUCCESS;
}

VOID DriverUninit(DRIVER_CONTEXT *pDrvCtx)
{
    DEVICE_CONTEXT *pCur, *pTmpDev;

    if (pDrvCtx->hEvent)
        OsEventClose(pDrvCtx->hEvent);
    if (pDrvCtx->hMutex)
        OsMutexClose(pDrvCtx->hMutex);
    if (pDrvCtx->hDeviceUnusedEvent)
        OsEventClose(pDrvCtx->hDeviceUnusedEvent);
    if (hDriver)
        WDU_Uninit(hDriver);

    /* Release any remaining devices */
    pCur = pDrvCtx->deviceContextList; 
    while (pCur)
    {
        pTmpDev = pCur;
        pCur = pCur->pNext;
        free(pTmpDev);
    }
}

int main()
{
    DWORD dwError;
    WORD wVendorId = 0;
    WORD wProductId = 0;
    DRIVER_CONTEXT DrvCtx;
    WDU_MATCH_TABLE matchTable;

    BZERO(DrvCtx);
    
    wVendorId = USE_DEFAULT;
    wProductId = USE_DEFAULT;

#if defined(USB_DIAG_SAMPLE)
    printf("Enter device vendor id (hex) (=0x%x):\n", DEFAULT_VENDOR_ID);
    fgets(line, sizeof(line), stdin);
    sscanf(line, "%hx", &wVendorId);
    
    printf("Enter device product id (hex) (=0x%x):\n", DEFAULT_PRODUCT_ID);
    fgets(line, sizeof(line), stdin);
    sscanf(line, "%hx", &wProductId);
#endif

    /* use defaults */
    if (wVendorId == USE_DEFAULT)
        wVendorId = DEFAULT_VENDOR_ID;
    if (wProductId == USE_DEFAULT)
        wProductId = DEFAULT_PRODUCT_ID;
    BZERO(matchTable);
    matchTable.wVendorId = wVendorId;
    matchTable.wProductId = wProductId;

    dwError = DriverInit(&matchTable, 1, DEFAULT_DRIVER_NAME, 
        DEFAULT_LICENSE_STRING, &DrvCtx);
    if (dwError)
        goto Exit;
    
    printf("Please make sure the device is attached:\n");

    /* Wait for the device to be attached */
    dwError = OsEventWait(DrvCtx.hEvent, ATTACH_EVENT_TIMEOUT);
    if (dwError)
    {
        if (dwError==WD_TIME_OUT_EXPIRED)
        {
            ERR("Timeout expired for connection with the device.\n" 
                "Check that the device is connected and try again.\n");
        }
        else
        {
            ERR("main: OsEventWait() failed on event 0x%p: error 0x%lx "
                "(\"%s\")\n", DrvCtx.hEvent, dwError, Stat2Str(dwError));
        }
        goto Exit;
    }

    DeviceDiagMenu(&DrvCtx);
Exit:
    DriverUninit(&DrvCtx);
    return dwError;
}

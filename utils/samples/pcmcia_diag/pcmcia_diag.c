/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/************************************************************************
*  File: pcmcia_diag.c
*
*  Sample user-mode diagnostics application for accessing PCMCIA
*  devices using WinDriver's API.
*************************************************************************/

#include <stdio.h>
#include "wdc_lib.h"
#include "utils.h"
#include "status_strings.h"
#include "samples/shared/diag_lib.h"
#include "samples/shared/wdc_diag_lib.h"
#include "pcmcia_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define PCMCIA_ERR printf

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
static CHAR gsInput[256];

/*************************************************************
  Static functions prototypes
 *************************************************************/
/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */
static void MenuMain(WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */
static WDC_DEVICE_HANDLE DeviceFindAndOpen(WORD wManufacturerId, WORD wDeviceId);
static BOOL DeviceFind(WORD wManufacturerId, WORD wDeviceId, WD_PCMCIA_SLOT *pSlot);
static WDC_DEVICE_HANDLE DeviceOpen(const WD_PCMCIA_SLOT *pSlot);
static void DeviceClose(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddr(WDC_DEVICE_HANDLE hDev);
static void SetAddrSpace(WDC_DEVICE_HANDLE hDev, PDWORD pdwAddrSpace);

/* -----------------------------------------------
    Read the attribute space
   ----------------------------------------------- */
static void MenuReadAttribSpace(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Set memory window
   ----------------------------------------------- */
static void MenuSetMemoryWindow(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static void MenuInterrupts(WDC_DEVICE_HANDLE hDev);
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, PCMCIA_INT_RESULT *pIntResult);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static void MenuEvents(WDC_DEVICE_HANDLE hDev);
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/*************************************************************
  Functions implementation
 *************************************************************/
int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DWORD dwStatus;

    printf("\n");
    printf("PCMCIA diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    /* Initialize the PCMCIA library */
    dwStatus = PCMCIA_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCMCIA_ERR("pcmcia_diag: Failed to initialize the PCMCIA library: %s",
            PCMCIA_GetLastErr());
        return dwStatus;
    }

    /* Find and open a PCMCIA device (by default ID) */
    if (PCMCIA_DEFAULT_MANUFACTURER_ID)
        hDev = DeviceFindAndOpen(PCMCIA_DEFAULT_MANUFACTURER_ID, PCMCIA_DEFAULT_DEVICE_ID);

    /* Display main diagnostics menu for communicating with the device */
    MenuMain(&hDev);

    /* Perform necessary cleanup before exiting the program */
    if (hDev)
        DeviceClose(hDev);
 
    dwStatus = PCMCIA_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
        PCMCIA_ERR("pcmcia_diag: Failed to uninit the PCMCIA library: %s", PCMCIA_GetLastErr());
    
    return dwStatus;
}

/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */
/* Main menu options */
enum {
    MENU_MAIN_SCAN_PCMCIA_BUS = 1,
    MENU_MAIN_FIND_AND_OPEN,
    MENU_MAIN_RW_ADDR,
    MENU_MAIN_READ_ATTRIB_SPACE,
    MENU_MAIN_SET_MEMORY_WINDOW,
    MENU_MAIN_ENABLE_DISABLE_INT,
    MENU_MAIN_EVENTS,
    MENU_MAIN_EXIT = DIAG_EXIT_MENU,
};

/* Main diagnostics menu */
static void MenuMain(WDC_DEVICE_HANDLE *phDev)
{
    DWORD option;
    
    do
    {
        printf("\n");
        printf("PCMCIA main menu\n");
        printf("-----------------\n");
        printf("%d. Scan PCMCIA bus\n", MENU_MAIN_SCAN_PCMCIA_BUS);
        printf("%d. Find and open a PCMCIA device\n", MENU_MAIN_FIND_AND_OPEN);
        if (*phDev)
        {
            printf("%d. Read/write memory and IO addresses on the device\n",
                MENU_MAIN_RW_ADDR);
            printf("%d. Read/write the PCMCIA attribute space\n",
                MENU_MAIN_READ_ATTRIB_SPACE);
            printf("%d. Set PCMCIA memory window\n",
                MENU_MAIN_SET_MEMORY_WINDOW);
            printf("%d. Enable/disable the device's interrupts\n",
                MENU_MAIN_ENABLE_DISABLE_INT);
            printf("%d. Register/unregister plug-and-play and power management "
                "events\n", MENU_MAIN_EVENTS);
        }
        printf("%d. Exit\n", MENU_MAIN_EXIT);
        
        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            *phDev ? MENU_MAIN_EVENTS : MENU_MAIN_FIND_AND_OPEN))
        {
            continue;
        }

        switch (option)
        {
        case MENU_MAIN_EXIT:
            /* Exit menu */
            break;
        case MENU_MAIN_SCAN_PCMCIA_BUS:
            /* Scan PCMCIA bus */
            WDC_DIAG_PcmciaDevicesInfoPrintAll();
            break;
        case MENU_MAIN_FIND_AND_OPEN:
            /* Find and open a PCMCIA device */
            if (*phDev)
                DeviceClose(*phDev);
            *phDev = DeviceFindAndOpen(0, 0);
            break;
        case MENU_MAIN_RW_ADDR:
            /* Read/write memory and I/O addresses */
            MenuReadWriteAddr(*phDev);
            break;
        case MENU_MAIN_READ_ATTRIB_SPACE:
            /* Read/Write the device's attribute space */
            MenuReadAttribSpace(*phDev);
            break;
        case MENU_MAIN_SET_MEMORY_WINDOW:
            MenuSetMemoryWindow(*phDev);
            break;
        case MENU_MAIN_ENABLE_DISABLE_INT:
            /* Enable/disable interrupts */
            MenuInterrupts(*phDev);
            break;
        case MENU_MAIN_EVENTS:
            /* Register/unregister plug-and-play and power management events */
            MenuEvents(*phDev);
            break;
        }
    } while (MENU_MAIN_EXIT != option);
}

/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */
/* Find and open a PCMCIA device */
static WDC_DEVICE_HANDLE DeviceFindAndOpen(WORD wManufacturerId, WORD wDeviceId)
{
    WD_PCMCIA_SLOT slot;
    
    if (!DeviceFind(wManufacturerId, wDeviceId, &slot))
        return NULL;

    return DeviceOpen(&slot);
}

/* Find a PCMCIA device */
static BOOL DeviceFind(WORD wManufacturerId, WORD wDeviceId, WD_PCMCIA_SLOT *pSlot)
{
    DWORD dwStatus;
    DWORD i, dwNumDevices;
    WDC_PCMCIA_SCAN_RESULT scanResult;

    if (wManufacturerId == 0)
    {
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&wManufacturerId,
            "Enter manufacturer ID", TRUE, 0, 0))
        {
            return FALSE;
        }

        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&wDeviceId,
            "Enter device ID", TRUE, 0, 0))
        {
            return FALSE;
        }
    }

    BZERO(scanResult);
    dwStatus = WDC_PcmciaScanDevices(wManufacturerId, wDeviceId, &scanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCMCIA_ERR("DeviceFind: Failed scanning the PCMCIA bus.\n"
            "Error: 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        return FALSE;
    }

    dwNumDevices = scanResult.dwNumDevices;
    if (!dwNumDevices)
    {
        printf("No matching PCMCIA device was found for search criteria "
            "(Manufacturer ID 0x%hX, Device ID 0x%hX)\n",
            wManufacturerId, wDeviceId);

        return FALSE;
    }
    
    printf("\n");
    printf("Found %ld matching device%s [ Manufacturer ID 0x%hX%s, Device ID 0x%hX%s ]:\n",
        dwNumDevices, dwNumDevices > 1 ? "s" : "",
        wManufacturerId, wManufacturerId ? "" : " (ALL)",
        wDeviceId, wDeviceId ? "" : " (ALL)");
    
    for (i = 0; i < dwNumDevices; i++)
    {
        printf("\n");
        printf("%2ld. Manufacturer ID: 0x%hX, Device ID: 0x%hX\n",
            i + 1,
            scanResult.deviceId[i].wManufacturerId,
            scanResult.deviceId[i].wCardId);

        WDC_DIAG_PcmciaDeviceInfoPrint(&scanResult.deviceSlot[i]);
    }
    printf("\n");

    if (dwNumDevices > 1)
    {
        sprintf(gsInput, "Select a device (1 - %ld): ", dwNumDevices);
        i = 0;
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&i,
            gsInput, FALSE, 1, dwNumDevices))
        {
            return FALSE;
        }
    }

    *pSlot = scanResult.deviceSlot[i - 1];

    return TRUE;
}

/* Open a handle to a PCMCIA device */
static WDC_DEVICE_HANDLE DeviceOpen(const WD_PCMCIA_SLOT *pSlot)
{
    WDC_DEVICE_HANDLE hDev;
    DWORD dwStatus;    
    WD_PCMCIA_CARD_INFO deviceInfo;

    /* Retrieve the device's resources information */
    BZERO(deviceInfo);
    deviceInfo.pcmciaSlot = *pSlot;
    dwStatus = WDC_PcmciaGetDeviceInfo(&deviceInfo);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCMCIA_ERR("DeviceOpen: Failed retrieving the device's resources information.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        return NULL;
    }

    /* NOTE: You can modify the device's resources information here, if
       necessary (mainly the deviceInfo.Card.Items array or the items number -
       deviceInfo.Card.dwItems) in order to register only some of the resources
       or register only a portion of a specific address space, for example. */

    /* Open a handle to the device */
    hDev = PCMCIA_DeviceOpen(&deviceInfo);
    if (!hDev)
    {
        PCMCIA_ERR("DeviceOpen: Failed opening a handle to the device: %s",
            PCMCIA_GetLastErr());
    }
    return hDev;
}

/* Close handle to a PCMCIA device */
static void DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    if (!hDev)
        return;
    
    if (!PCMCIA_DeviceClose(hDev))
    {
        PCMCIA_ERR("DeviceClose: Failed closing PCMCIA device: %s",
            PCMCIA_GetLastErr());
    }
}

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
/* Read/write address menu options */
enum {
    MENU_RW_ADDR_SET_ADDR_SPACE = 1,
    MENU_RW_ADDR_SET_MODE,
    MENU_RW_ADDR_SET_TRANS_TYPE,
    MENU_RW_ADDR_READ,
    MENU_RW_ADDR_WRITE,
    MENU_RW_ADDR_EXIT = DIAG_EXIT_MENU,
};

#define ACTIVE_ADDR_SPACE_NEEDS_INIT 0xFF

/* Read/write memory or I/O space address menu */
static void MenuReadWriteAddr(WDC_DEVICE_HANDLE hDev)
{
    DWORD option;
    static DWORD dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
    static WDC_ADDR_MODE mode = WDC_MODE_32;
    static BOOL fBlock = FALSE;

    /* Initialize active address space */
    if (ACTIVE_ADDR_SPACE_NEEDS_INIT == dwAddrSpace)
    {
        DWORD dwNumAddrSpaces = PCMCIA_GetNumAddrSpaces(hDev);
        
        /* Find the first active address space */
        for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; dwAddrSpace++)
        {
            if (WDC_AddrSpaceIsActive(hDev, dwAddrSpace))
                break;
        }
        
        /* Sanity check */
        if (dwAddrSpace == dwNumAddrSpaces)
        {
            PCMCIA_ERR("MenuReadWriteAddr: Error - no active address spaces found\n");
            dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
            return;
        }
    }

    do
    {
        printf("\n");
        printf("Read/write the device's memory and IO ranges\n");
        printf("---------------------------------------------\n");
        printf("%d. Change active address space for read/write "
            "(currently: BAR %ld)\n", MENU_RW_ADDR_SET_ADDR_SPACE, dwAddrSpace);
        printf("%d. Change active read/write mode (currently: %s)\n",
            MENU_RW_ADDR_SET_MODE,
            (WDC_MODE_8 == mode) ? "8 bit" : (WDC_MODE_16 == mode) ? "16 bit" :
            (WDC_MODE_32 == mode) ? "32 bit" : "64 bit");
        printf("%d. Toggle active transfer type (currently: %s)\n",
            MENU_RW_ADDR_SET_TRANS_TYPE,
            fBlock ? "block transfers" :
            "non-block transfers");
        printf("%d. Read from active address space\n", MENU_RW_ADDR_READ);
        printf("%d. Write to active address space\n", MENU_RW_ADDR_WRITE);
        printf("%d. Exit menu\n", MENU_RW_ADDR_EXIT);
        printf("\n");
        
        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            MENU_RW_ADDR_WRITE))
        {
            continue;
        }
        
        switch (option)
        {
        case MENU_RW_ADDR_EXIT: /* Exit menu */
            break;
        case MENU_RW_ADDR_SET_ADDR_SPACE: /* Set active address space for read/write address requests */
        {
            SetAddrSpace(hDev, &dwAddrSpace);
            break;
        }
        case MENU_RW_ADDR_SET_MODE: /* Set active mode for read/write address requests */
            WDC_DIAG_SetMode(&mode);
            break;
        case MENU_RW_ADDR_SET_TRANS_TYPE: /* Toggle active transfer type */
            fBlock = !fBlock;
            break;
        case MENU_RW_ADDR_READ:  /* Read from a memory or I/O address */
        case MENU_RW_ADDR_WRITE: /* Write to a memory or I/O address */
        {
            WDC_DIRECTION direction =
                (MENU_RW_ADDR_READ == option ? WDC_READ : WDC_WRITE);

            if (fBlock)
                WDC_DIAG_ReadWriteBlock(hDev, direction, dwAddrSpace);
            else
                WDC_DIAG_ReadWriteAddr(hDev, direction, dwAddrSpace, mode);
            
            break;
        }
        }
    } while (MENU_RW_ADDR_EXIT != option);
}

static void SetAddrSpace(WDC_DEVICE_HANDLE hDev, PDWORD pdwAddrSpace)
{
    DWORD dwAddrSpace;
    DWORD dwNumAddrSpaces = PCMCIA_GetNumAddrSpaces(hDev);
    PCMCIA_ADDR_SPACE_INFO addrSpaceInfo;
    
    printf("\n");
    printf("Select an active address space:\n");
    printf("-------------------------------\n");

    for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; dwAddrSpace++)
    {   
        BZERO(addrSpaceInfo);
        addrSpaceInfo.dwAddrSpace = dwAddrSpace;
        if (!PCMCIA_GetAddrSpaceInfo(hDev, &addrSpaceInfo))
        {
            PCMCIA_ERR("SetAddrSpace: Error - Failed to get address space information: %s",
                PCMCIA_GetLastErr());
            return;
        }

        printf("%ld. %-*s %-*s %s\n",
            dwAddrSpace + 1,
            MAX_NAME_DISPLAY, addrSpaceInfo.sName,
            MAX_TYPE - 1, addrSpaceInfo.sType,
            addrSpaceInfo.sDesc);
    }
    printf("\n");

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwAddrSpace,
        "Enter option", FALSE, 1, dwNumAddrSpaces))
    {
        return;
    }

    dwAddrSpace--;
    if (!WDC_AddrSpaceIsActive(hDev, dwAddrSpace))
    {
        printf("You have selected an inactive address space\n");
        return;
    }
            
    *pdwAddrSpace = dwAddrSpace;
}

/* -----------------------------------------------
    Read the attribute space
   ----------------------------------------------- */
/* Read the attribute space menu options */
enum {
    MENU_READ_ATTRIB_SPACE_READ_OFFSET = 1,
    MENU_READ_ATTRIB_SPACE_READ_ALL_TUPLES,
    MENU_READ_ATTRIB_SPACE_EXIT = DIAG_EXIT_MENU,
};

/* Display read attribute space menu */
static void MenuReadAttribSpace(WDC_DEVICE_HANDLE hDev)
{
    DWORD option;
  
    /* Display tuples information */
    printf("\n");
    printf("PCMCIA attribute space tuples:\n");
    printf("------------------------------\n\n");
    WDC_DIAG_PcmciaTuplesPrintAll(hDev, FALSE);
    printf("\n");
        
    do {
        printf("\n");
        printf("Read the device's attribute space\n");
        printf("----------------------------------\n");
        printf("%d. Read from a specific offset in the PCMCIA attribute space\n",
            MENU_READ_ATTRIB_SPACE_READ_OFFSET);
        printf("%d. Read all tuples in the attribute space (see list above)\n",
            MENU_READ_ATTRIB_SPACE_READ_ALL_TUPLES);
        printf("%d. Exit menu\n", MENU_READ_ATTRIB_SPACE_EXIT);
        printf("\n");

        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            MENU_READ_ATTRIB_SPACE_READ_ALL_TUPLES))
        {
            continue;
        }

        switch (option)
        {
        case MENU_READ_ATTRIB_SPACE_EXIT: /* Exit menu */
            break;
        case MENU_READ_ATTRIB_SPACE_READ_OFFSET: /* Read from a PCMCIA attribute space offset */
            WDC_DIAG_ReadWriteBlock(hDev, WDC_READ, WDC_AD_CFG_SPACE);
            break;
        case MENU_READ_ATTRIB_SPACE_READ_ALL_TUPLES:
            printf("\n");
            printf("PCMCIA attribute space tuples:\n");
            printf("------------------------------\n\n");
            WDC_DIAG_PcmciaTuplesPrintAll(hDev, TRUE);
            break;
        }
    } while (MENU_READ_ATTRIB_SPACE_EXIT != option);
}

/* -----------------------------------------------
    Set memory window
   ----------------------------------------------- */
static void MenuSetMemoryWindow(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus, speed, width, dwCardBase;
    DIAG_INPUT_RESULT res;
  
    printf("\nPCMCIA memory window:\n");
    printf("---------------------\n");

    for (;;)
    {
        res = DIAG_InputNum(&speed,
            "access speed, 0 - default, 1 - 250ns, 2 - 200ns, 3 - 150ns, " "4 - 100ns",
            FALSE, sizeof(speed), WD_PCMCIA_ACC_SPEED_DEFAULT, WD_PCMCIA_ACC_SPEED_100NS);

        if (res == DIAG_INPUT_CANCEL)
            return;
        if (res == DIAG_INPUT_FAIL)
            continue;
        break;
    }

    for (;;)
    {
        res = DIAG_InputNum(&width,
            "access width, 0 - default, 1 - 8bit, 2 - 16bit", FALSE,
            sizeof(width), WD_PCMCIA_ACC_WIDTH_DEFAULT, WD_PCMCIA_ACC_WIDTH_16BIT);

        if (res == DIAG_INPUT_CANCEL)
            return;
        if (res == DIAG_INPUT_FAIL)
            continue;
        break;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputNum(&dwCardBase,
        "card base offset: ", TRUE, sizeof(dwCardBase), 0, 0xffffffff))
    {
        return;
    }

    dwStatus = WDC_PcmciaSetWindow(hDev, speed, width, dwCardBase);
    if (dwStatus)
        printf("Failed setting PCMCIA window: %lx - %s\n", dwStatus, Stat2Str(dwStatus));
}

/* Read/write the run-time registers menu options */
enum {
    MENU_RW_REGS_READ_ALL = 1,
    MENU_RW_REGS_READ_REG,
    MENU_RW_REGS_WRITE_REG,
    MENU_RW_REGS_EXIT = DIAG_EXIT_MENU,
};

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
/* Interrupts menu options */
enum {
    MENU_INT_ENABLE_DISABLE = 1,
    MENU_INT_EXIT = DIAG_EXIT_MENU,
};

/* Enable/Disable interrupts menu */
static void MenuInterrupts(WDC_DEVICE_HANDLE hDev)
{
    DWORD option;
    BOOL fIntEnable;

    /* TODO: You can remove this message after you have modified the
             implementation of PCMCIA_IntEnable() in pcmcia_lib.c to correctly
             acknowledge the interrupts (see guidelines in PCMCIA_IntEnable()) */
    printf("\n");
    printf("WARNING!!!\n");
    printf("----------\n");
    printf("Before enabling the interrupts, you must first modify the source code\n"
        "of PCMCIA_IntEnable(), in the file pcmcia_lib.c, to correctly acknowledge\n"
        "interrupts when they occur (as dictated by the hardware's specifications)\n");

    do
    {
        fIntEnable = !PCMCIA_IntIsEnabled(hDev);
        
        printf("\n");
        printf("Interrupts\n");
        printf("-----------\n");
        printf("%d. %s interrupts\n", MENU_INT_ENABLE_DISABLE,
            fIntEnable ? "Enable" : "Disable");
        printf("%d. Exit menu\n", MENU_INT_EXIT);
        printf("\n");
        
        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            MENU_INT_ENABLE_DISABLE))
        {
            continue;
        }

        switch (option)
        {
        case MENU_INT_EXIT: /* Exit menu */
            break;
        case MENU_INT_ENABLE_DISABLE: /* Enable/disable interrupts */
            if (fIntEnable)
            {
                if (WD_STATUS_SUCCESS == PCMCIA_IntEnable(hDev, DiagIntHandler))
                    printf("Interrupts enabled\n");
                else
                    PCMCIA_ERR("Failed enabling interrupts: %s", PCMCIA_GetLastErr());
            }
            else
            {
                if (WD_STATUS_SUCCESS == PCMCIA_IntDisable(hDev))
                    printf("Interrupts disabled\n");
                else
                    PCMCIA_ERR("Failed disabling interrupts: %s", PCMCIA_GetLastErr());
            }
            break;
        }
    } while (MENU_INT_EXIT != option);
}

/* Diagnostics interrupt handler routine */
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, PCMCIA_INT_RESULT *pIntResult)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics interrupt handler routine */

    printf("Got interrupt number %ld\n", pIntResult->dwCounter);
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
/* Events menu options */
enum {
    MENU_EVENTS_REGISTER_UNREGISTER = 1,
    MENU_EVENTS_EXIT = DIAG_EXIT_MENU,
};

/* Register/unregister Plug-and-play and power management events */
static void MenuEvents(WDC_DEVICE_HANDLE hDev)
{
    DWORD option;
    BOOL fRegister;

    do
    {
        fRegister = !PCMCIA_EventIsRegistered(hDev);
        
        printf("\n");
        printf("Plug-and-play and power management events\n");
        printf("------------------------------------------\n");
        printf("%d. %s events\n", MENU_EVENTS_REGISTER_UNREGISTER,
            fRegister ? "Register" : "Unregister");
        printf("%d. Exit menu\n", MENU_EVENTS_EXIT);
        printf("\n");
        
        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            MENU_EVENTS_REGISTER_UNREGISTER))
        {
            continue;
        }

        switch (option)
        {
        case MENU_EVENTS_EXIT: /* Exit menu */
            break;
        case MENU_EVENTS_REGISTER_UNREGISTER: /* Register/unregister events */
            if (fRegister)
            {
                if (WD_STATUS_SUCCESS == PCMCIA_EventRegister(hDev, DiagEventHandler))
                    printf("Events registered\n");
                else
                    PCMCIA_ERR("Failed to register events. Last error:\n%s", PCMCIA_GetLastErr());
            }
            else
            {
                if (WD_STATUS_SUCCESS == PCMCIA_EventUnregister(hDev))
                    printf("Events unregistered\n");
                else
                    PCMCIA_ERR("Failed to unregister events. Last Error:\n%s", PCMCIA_GetLastErr());
            }
            break;
        }
    } while (MENU_EVENTS_EXIT != option);
}

/* Plug-and-play and power management events handler routine */
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics events handler routine */

    printf("\nReceived event notification (device handle 0x%p): ", hDev);
    switch (dwAction)
    {
    case WD_INSERT:
        printf("WD_INSERT\n");
        break;
    case WD_REMOVE:
        printf("WD_REMOVE\n");
        break;
    case WD_POWER_CHANGED_D0:
        printf("WD_POWER_CHANGED_D0\n");
        break;
    case WD_POWER_CHANGED_D1:
        printf("WD_POWER_CHANGED_D1\n");
        break;
    case WD_POWER_CHANGED_D2:
        printf("WD_POWER_CHANGED_D2\n");
        break;
    case WD_POWER_CHANGED_D3:
        printf("WD_POWER_CHANGED_D3\n");
        break;
    case WD_POWER_SYSTEM_WORKING:
        printf("WD_POWER_SYSTEM_WORKING\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING1:
        printf("WD_POWER_SYSTEM_SLEEPING1\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING2:
        printf("WD_POWER_SYSTEM_SLEEPING2\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING3:
        printf("WD_POWER_SYSTEM_SLEEPING3\n");
        break;
    case WD_POWER_SYSTEM_HIBERNATE:
        printf("WD_POWER_SYSTEM_HIBERNATE\n");
        break;
    case WD_POWER_SYSTEM_SHUTDOWN:
        printf("WD_POWER_SYSTEM_SHUTDOWN\n");
        break;
    default:
        printf("0x%lx\n", dwAction);
        break;
    }
}

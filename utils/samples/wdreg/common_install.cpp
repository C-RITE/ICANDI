/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/*
 * File - common_install.cpp
 *
 * Installation library.
 */

#include <stdio.h>
#include <conio.h>
#include <tchar.h> /* Make all functions UNICODE safe. */
#include <windows.h>
#include <newdev.h>
#include "common_install.h"

CCString g_sErr; /* contains the last error encountered */
BOOL g_fRebootRequired = FALSE;

typedef BOOL (*DEV_FUNC)(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);

/* retrieves Windows' error string */
CCString LastErrString()
{
    LPVOID lpMsgBuf;
    DWORD dwLastError = GetLastError();
    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),/* Default lang*/
        (LPTSTR) &lpMsgBuf, 0, NULL))
    {
        return CCString("");
    }
    else
    {
        CCString sRes((LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
        return sRes;
    }
}

CCString InstallLastErrorString()
{
    return g_sErr;
}

t_pfi_SetupGetStringField __SetupGetStringField;
t_pfi_SetupGetFieldCount __SetupGetFieldCount;
t_pfi_SetupFindNextLine __SetupFindNextLine;
t_pfi_SetupOpenInfFile __SetupOpenInfFile;
t_pfi_SetupCloseInfFile __SetupCloseInfFile;
t_pfi_SetupFindFirstLine __SetupFindFirstLine;
t_pfi_cm_locate __CM_Locate_DevNode;
t_pfi_cm_reenumerate __CM_Reenumerate_DevNode;
t_pfi_cm_get_status __CM_Get_DevNode_Status;
t_pfi_cm_get_child __CM_Get_Child;
t_pfi_cm_get_parent __CM_Get_Parent;
t_pfi_cm_get_sibling __CM_Get_Sibling;
t_pfi_cm_get_device_ida __CM_Get_Device_IDA;
t_pfi_cm_get_device_id_size __CM_Get_Device_ID_Size;
t_pfi_setup_di_open_device_infoa __SetupDiOpenDeviceInfoA;
t_pfi_updatedriver __UpdateDriverForPlugAndPlayDevices;
t_pfi_setup_di_get_class __SetupDiGetClassDevs;
t_pfi_setup_di_enum_dev __SetupDiEnumDeviceInfo;
t_pfi_setup_di_get_device_reg __SetupDiGetDeviceRegistryProperty;
t_pfi_setup_di_get_device_reg_w __SetupDiGetDeviceRegistryPropertyW;
t_pfi_setup_di_set_device_reg __SetupDiSetDeviceRegistryProperty;
t_pfi_setup_di_destroy __SetupDiDestroyDeviceInfoList;
t_pfi_setup_copy_oem_inf __SetupCopyOEMInf;
t_pfi_SetupUninstallOEMInf __SetupUninstallOEMInf;
t_pfi_SetupDiRemoveDevice __SetupDiRemoveDevice;
t_pfi_SetupDiCreateDeviceInfo __SetupDiCreateDeviceInfo;
t_pfi_SetupDiCreateDeviceInfoList __SetupDiCreateDeviceInfoList;
t_pfi_SetupDiCallClassInstaller __SetupDiCallClassInstaller;
t_pfi_SetupDiGetINFClass __SetupDiGetINFClass;
t_pfi_SetupDiClassGuidsFromName __SetupDiClassGuidsFromName;
t_pfi_SetupDiSetClassInstallParams __SetupDiSetClassInstallParams;
t_pfi_SetupDiGetDeviceInstallParams __SetupDiGetDeviceInstallParams;
t_pfi_SetupOpenFileQueue __SetupOpenFileQueue;
t_pfi_SetupInstallFilesFromInfSection __SetupInstallFilesFromInfSection;
t_pfi_SetupCommitFileQueue __SetupCommitFileQueue;
t_pfi_SetupCloseFileQueue __SetupCloseFileQueue;
t_pfi_SetupInitDefaultQueueCallback __SetupInitDefaultQueueCallback;
t_pfi_SetupDefaultQueueCallback __SetupDefaultQueueCallback;
t_pfi_SetupTermDefaultQueueCallback __SetupTermDefaultQueueCallback;
t_pfi_CMP_WaitNoPendingInstallEvents __CMP_WaitNoPendingInstallEvents;
t_pfi_SetupDiGetSelectedDriverA __SetupDiGetSelectedDriver;
t_pfi_SetupDiGetDriverInfoDetailA __SetupDiGetDriverInfoDetail;
t_pfi_SetupQueryInfOriginalFileInformationA __SetupQueryInfOriginalFileInformation;
t_pfi_SetupGetInfInformationA __SetupGetInfInformation;
t_pfi_SetupGetInfFileListA __SetupGetInfFileList;
t_pfi_DIFXAPISetLogCallbackA __DIFXAPISetLogCallback;
t_pfi_DriverPackageGetPathA __DriverPackageGetPath;
t_pfi_DriverPackageInstallA __DriverPackageInstall;
t_pfi_DriverPackagePreinstallA __DriverPackagePreinstall;
t_pfi_DriverPackageUninstallA __DriverPackageUninstall;

static BOOL GetProperty(HDEVINFO *pDeviceInfoSet,
    SP_DEVINFO_DATA *pDeviceInfoData, char **buffer, DWORD *buffersize,
    DWORD prop, BOOL *fErrorOccurred);
static BOOL MatchDevice(const char *hwid, HDEVINFO *pDeviceInfoSet,
    SP_DEVINFO_DATA *pDeviceInfoData, DWORD prop, BOOL *fErrorOccurred);
extern "C" BOOL EnumDeviceInfo(const char *enumerator, const char *hwid,
    DWORD devices_type, DEV_FUNC f, void *f_ctx, BOOL *found);
static BOOL cbUpdateDriver(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);
static BOOL cbExistsDevnode(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);
static BOOL cbDelete(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);
static BOOL cbChangeState(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);
static BOOL cbReenumDevInfo(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx);
static BOOL CopyInfFile(const char *SourceInfFilePath);
static BOOL DeleteInfCopy(const char *SourceInfFilePath);
static HINSTANCE new_dev_lib = NULL;
static HINSTANCE setup_lib = NULL;
static HINSTANCE cfgmgr32_lib = NULL;
static HINSTANCE difxapi_lib = NULL;
static int ref_count;

void unload_functions()
{
    if (ref_count)
        ref_count--;
    if (ref_count)
        return;
    if (new_dev_lib)
        FreeLibrary(new_dev_lib);
    if (setup_lib)
        FreeLibrary(setup_lib);
    if (cfgmgr32_lib)
        FreeLibrary(cfgmgr32_lib);
    if (difxapi_lib)
        FreeLibrary(difxapi_lib);

    new_dev_lib = setup_lib = cfgmgr32_lib = difxapi_lib = NULL;
}

#define GET_PROC_ADDR_MIN(ptr, type, lib, name) \
    if (lib) \
    { \
        ptr = (type)GetProcAddress(lib, name); \
        if (!ptr) \
        { \
            g_sErr.Format("%sCannot find the function %s in %s: %s\n", \
                (LPCSTR)g_sErr, \
                name, \
                lib==new_dev_lib? "newdev.dll" : "setupapi.dll", \
                (LPCSTR)LastErrString()); \
            goto Error; \
        } \
    } \
    else \
        ptr = NULL;


#define GET_PROC_ADDR(ptr, type, lib, name) \
    if (lib) \
        ptr = (type)GetProcAddress(lib, name); \
    else \
        ptr = NULL;

#define GET_PROC_ADDR_FALLBACK(ptr, type, lib, lib_fallback, name) \
    if (lib) \
    { \
        ptr = (type)GetProcAddress(lib, name); \
        if (!ptr && lib_fallback) \
            ptr = (type)GetProcAddress(lib_fallback, name); \
    } \
    else if (lib_fallback) \
        ptr = (type)GetProcAddress(lib_fallback, name); \
    else \
        ptr = NULL;

BOOL load_functions(ULONG mask)
{
    OS_TYPE iOsType = get_os_type();

    if (OS_WIN_NT_4 == iOsType)
        return FALSE;

    if (ref_count)
    {
        ref_count++;
        return TRUE;
    }

    if (mask & LOAD_FUNCTIONS_NEWDEV)
    {
        new_dev_lib = LoadLibraryEx("newdev.dll", 0, 0);
        if (!new_dev_lib)
        {
            g_sErr.Format("%sCannot load library newdev.dll: %s\n",
                (LPCSTR)g_sErr, (LPCSTR)LastErrString());
            goto Error;
        }
    }
    
    if (mask & LOAD_FUNCTIONS_SETUPAPI)
    {
        setup_lib = LoadLibraryEx("setupapi.dll", 0, 0);
        if (!setup_lib)
        {
            g_sErr.Format("%sCannot load library setupapi.dll: %s\n",
                (LPCSTR)g_sErr, (LPCSTR)LastErrString());
            goto Error;
        }
    }

    if (mask & LOAD_FUNCTIONS_CFGMGR32)
    {
        cfgmgr32_lib = LoadLibraryEx("cfgmgr32.dll", 0, 0);
        if (!cfgmgr32_lib)
        {
            g_sErr.Format("%sCannot load library cfgmgr32.dll: %s\n",
                (LPCSTR)g_sErr, (LPCSTR)LastErrString());
            goto Error;
        }
    }

    if (mask & LOAD_FUNCTIONS_DIFXAPI)
    {
        difxapi_lib = LoadLibraryEx("difxapi.dll", 0, 0);
        if (!difxapi_lib)
        {
            g_sErr.Format("%sCannot load library difxapi.dll: %s\n",
                (LPCSTR)g_sErr, (LPCSTR)LastErrString());
            goto Error;
        }
    }

    GET_PROC_ADDR_MIN(__SetupDiGetClassDevs, t_pfi_setup_di_get_class,
        setup_lib, "SetupDiGetClassDevsA");
    GET_PROC_ADDR_MIN(__SetupDiEnumDeviceInfo, t_pfi_setup_di_enum_dev,
        setup_lib, "SetupDiEnumDeviceInfo");
    GET_PROC_ADDR_MIN(__SetupDiGetDeviceRegistryProperty,
        t_pfi_setup_di_get_device_reg, setup_lib, 
        "SetupDiGetDeviceRegistryPropertyA");
    GET_PROC_ADDR_MIN(__SetupDiGetDeviceRegistryPropertyW,
        t_pfi_setup_di_get_device_reg_w, setup_lib,
        "SetupDiGetDeviceRegistryPropertyW");
    GET_PROC_ADDR_MIN(__SetupDiDestroyDeviceInfoList, t_pfi_setup_di_destroy,
        setup_lib, "SetupDiDestroyDeviceInfoList");

    GET_PROC_ADDR(__UpdateDriverForPlugAndPlayDevices, t_pfi_updatedriver,
        new_dev_lib, "UpdateDriverForPlugAndPlayDevicesA");
    GET_PROC_ADDR(__SetupDiSetDeviceRegistryProperty,
        t_pfi_setup_di_set_device_reg, setup_lib, 
        "SetupDiSetDeviceRegistryPropertyA");
    GET_PROC_ADDR(__SetupDiOpenDeviceInfoA, t_pfi_setup_di_open_device_infoa,
        setup_lib, "SetupDiOpenDeviceInfoA");
 
    GET_PROC_ADDR(__SetupCopyOEMInf, t_pfi_setup_copy_oem_inf, setup_lib,
        "SetupCopyOEMInfA");
    GET_PROC_ADDR(__SetupUninstallOEMInf, t_pfi_SetupUninstallOEMInf, setup_lib,
        "SetupUninstallOEMInfA");
    GET_PROC_ADDR(__SetupGetStringField, t_pfi_SetupGetStringField, setup_lib,
        "SetupGetStringFieldA");
    GET_PROC_ADDR(__SetupGetFieldCount, t_pfi_SetupGetFieldCount, setup_lib,
        "SetupGetFieldCount");
    GET_PROC_ADDR(__SetupFindNextLine, t_pfi_SetupFindNextLine, setup_lib,
        "SetupFindNextLine");
    GET_PROC_ADDR(__SetupOpenInfFile, t_pfi_SetupOpenInfFile, setup_lib,
        "SetupOpenInfFileA");
    GET_PROC_ADDR(__SetupCloseInfFile, t_pfi_SetupCloseInfFile, setup_lib,
        "SetupCloseInfFile");
    GET_PROC_ADDR(__SetupFindFirstLine, t_pfi_SetupFindFirstLine, setup_lib,
        "SetupFindFirstLineA");
    GET_PROC_ADDR(__SetupDiRemoveDevice, t_pfi_SetupDiRemoveDevice, setup_lib,
        "SetupDiRemoveDevice");
    GET_PROC_ADDR(__SetupDiCreateDeviceInfoList,
        t_pfi_SetupDiCreateDeviceInfoList, setup_lib, 
        "SetupDiCreateDeviceInfoList");
    GET_PROC_ADDR(__SetupDiCreateDeviceInfo, t_pfi_SetupDiCreateDeviceInfo,
        setup_lib, "SetupDiCreateDeviceInfoA");
    GET_PROC_ADDR(__SetupDiCallClassInstaller, t_pfi_SetupDiCallClassInstaller,
        setup_lib, "SetupDiCallClassInstaller");
    GET_PROC_ADDR(__SetupDiGetINFClass, t_pfi_SetupDiGetINFClass, setup_lib,
        "SetupDiGetINFClassA");
    GET_PROC_ADDR(__SetupDiClassGuidsFromName, t_pfi_SetupDiClassGuidsFromName,
        setup_lib, "SetupDiClassGuidsFromNameA");
    GET_PROC_ADDR(__SetupDiSetClassInstallParams,
        t_pfi_SetupDiSetClassInstallParams, setup_lib, 
        "SetupDiSetClassInstallParamsA");
    GET_PROC_ADDR(__SetupDiGetDeviceInstallParams,
        t_pfi_SetupDiGetDeviceInstallParams, setup_lib, 
        "SetupDiGetDeviceInstallParamsA");
    GET_PROC_ADDR(__SetupInstallFilesFromInfSection,
        t_pfi_SetupInstallFilesFromInfSection, setup_lib,
        "SetupInstallFilesFromInfSectionA");
    GET_PROC_ADDR(__SetupCommitFileQueue, t_pfi_SetupCommitFileQueue, setup_lib,
        "SetupCommitFileQueueA");
    GET_PROC_ADDR( __SetupOpenFileQueue, t_pfi_SetupOpenFileQueue, setup_lib, 
        "SetupOpenFileQueue");
    GET_PROC_ADDR(__SetupCloseFileQueue, t_pfi_SetupCloseFileQueue, setup_lib,
        "SetupCloseFileQueue");
    GET_PROC_ADDR(__SetupInitDefaultQueueCallback,
        t_pfi_SetupInitDefaultQueueCallback, setup_lib,
        "SetupInitDefaultQueueCallback");
    GET_PROC_ADDR(__SetupDefaultQueueCallback, t_pfi_SetupDefaultQueueCallback,
        setup_lib, "SetupDefaultQueueCallbackA");
    GET_PROC_ADDR(__SetupTermDefaultQueueCallback,
        t_pfi_SetupTermDefaultQueueCallback, setup_lib,
        "SetupTermDefaultQueueCallback");
    GET_PROC_ADDR(__CMP_WaitNoPendingInstallEvents,
        t_pfi_CMP_WaitNoPendingInstallEvents, setup_lib,
        "CMP_WaitNoPendingInstallEvents");
    GET_PROC_ADDR(__SetupDiGetDriverInfoDetail,
        t_pfi_SetupDiGetDriverInfoDetailA, setup_lib,
        "SetupDiGetDriverInfoDetailA");
    GET_PROC_ADDR(__SetupDiGetSelectedDriver, t_pfi_SetupDiGetSelectedDriverA, 
        setup_lib, "SetupDiGetSelectedDriverA");
    GET_PROC_ADDR(__SetupQueryInfOriginalFileInformation,
        t_pfi_SetupQueryInfOriginalFileInformationA, setup_lib,
        "SetupQueryInfOriginalFileInformationA");
    GET_PROC_ADDR(__SetupDiGetDriverInfoDetail,
        t_pfi_SetupDiGetDriverInfoDetailA, setup_lib,
        "SetupDiGetDriverInfoDetailA");
    GET_PROC_ADDR(__SetupGetInfInformation, t_pfi_SetupGetInfInformationA,
        setup_lib, "SetupGetInfInformationA");
    GET_PROC_ADDR(__SetupGetInfFileList, t_pfi_SetupGetInfFileListA,
        setup_lib, "SetupGetInfFileListA");
    GET_PROC_ADDR(__DIFXAPISetLogCallback, t_pfi_DIFXAPISetLogCallbackA,
        difxapi_lib, "DIFXAPISetLogCallbackA");
    GET_PROC_ADDR(__DriverPackageGetPath, t_pfi_DriverPackageGetPathA,
        difxapi_lib, "DriverPackageGetPathA");
    GET_PROC_ADDR(__DriverPackageInstall, t_pfi_DriverPackageInstallA,
        difxapi_lib, "DriverPackageInstallA");
    GET_PROC_ADDR(__DriverPackagePreinstall, t_pfi_DriverPackagePreinstallA,
        difxapi_lib, "DriverPackagePreinstallA");
    GET_PROC_ADDR(__DriverPackageUninstall, t_pfi_DriverPackageUninstallA,
        difxapi_lib, "DriverPackageUninstallA");

    /*
     * CM_ interface resides in cfgmgr32.dll on Win9x and in setupapi.dll on NT
     * So we load CM_ functions from setupapi.dll and from cfgmgr32.dll as a
     * fallback.
     */
    GET_PROC_ADDR_FALLBACK(__CM_Locate_DevNode, t_pfi_cm_locate, setup_lib,
        cfgmgr32_lib, "CM_Locate_DevNodeA");
    GET_PROC_ADDR_FALLBACK(__CM_Reenumerate_DevNode, t_pfi_cm_reenumerate,
        setup_lib, cfgmgr32_lib, "CM_Reenumerate_DevNode");
    GET_PROC_ADDR_FALLBACK(__CM_Get_DevNode_Status, t_pfi_cm_get_status,
        setup_lib, cfgmgr32_lib, "CM_Get_DevNode_Status");
    GET_PROC_ADDR_FALLBACK(__CM_Get_Child, t_pfi_cm_get_child, setup_lib,
        cfgmgr32_lib, "CM_Get_Child");
    GET_PROC_ADDR_FALLBACK(__CM_Get_Parent, t_pfi_cm_get_parent, setup_lib,
        cfgmgr32_lib, "CM_Get_Parent");
    GET_PROC_ADDR_FALLBACK(__CM_Get_Sibling, t_pfi_cm_get_sibling, setup_lib,
        cfgmgr32_lib, "CM_Get_Sibling");
    GET_PROC_ADDR_FALLBACK(__CM_Get_Device_IDA, t_pfi_cm_get_device_ida,
        setup_lib, cfgmgr32_lib, "CM_Get_Device_IDA");
    GET_PROC_ADDR_FALLBACK(__CM_Get_Device_ID_Size, t_pfi_cm_get_device_id_size,
        setup_lib, cfgmgr32_lib, "CM_Get_Device_ID_Size");
    
    ref_count++;
    return TRUE;
Error:
    unload_functions();
    return FALSE;
}

static BOOL CopyInfFile(const char *SourceInfFilePath)
{
    CCString sInfSrc, sInfDst, sPnfSrc, sPnfDst;
    char buffer[MAX_PATH];

    if (!__SetupCopyOEMInf((char *)SourceInfFilePath, NULL, SPOST_NONE, 0,
        buffer, MAX_PATH, 0, NULL))
    {
        g_sErr.Format("%sWarning: cannot copy INF file %s to the INF "
            "directory: %s\n", (LPCSTR)g_sErr, SourceInfFilePath,
            (LPCSTR)LastErrString());
        return FALSE;
    }

    return TRUE;
}

/* The function modifies sPath */
static void replace_slashes(char *sPath, BOOL dos_to_unix)
{
    int i;
    for (i=0; sPath[i]; i++)
    {
        if (dos_to_unix && sPath[i]=='\\')
            sPath[i] = '/';
        else if (!dos_to_unix && sPath[i]=='/')
            sPath[i] = '\\';
    }
}

BOOL install_inf_section(const char *file_name_with_path)
{
    UINT line;
    CCString file_name = file_name_with_path;
    replace_slashes((PSTR)file_name, FALSE);
    BOOL ret = FALSE;
    HSPFILEQ fq = NULL;
    HINF hInf = __SetupOpenInfFile(file_name, NULL, INF_STYLE_WIN4, &line);
    void *context;

    if (hInf == INVALID_HANDLE_VALUE)
    {
        g_sErr.Format("%sFailed opening INF file %s line %d: %s\n", 
            (LPCSTR)g_sErr, (LPCSTR)file_name, line, (LPCSTR)LastErrString());
        return FALSE;
    }
    
    fq = __SetupOpenFileQueue();
    BOOL rc = __SetupInstallFilesFromInfSection(hInf, NULL, fq, "DriverInstall",
        NULL, SP_COPY_FORCE_NEWER);
    context = __SetupInitDefaultQueueCallback(NULL); 
    __SetupCommitFileQueue(NULL,fq, __SetupDefaultQueueCallback, context);
    __SetupTermDefaultQueueCallback(context);
    __SetupCloseFileQueue(fq);
    __SetupCloseInfFile(hInf);
    
    return TRUE;
}
 
BOOL uninstall_inf(const char *hwid, const char *file_name_with_path)
{
    BOOL fReturn = FALSE;
    CCString file_name = file_name_with_path;
    replace_slashes((PSTR)file_name, FALSE);

    if (get_os_type() == OS_WIN_98)
    {
        g_sErr.Format("%sCannot uninstall INF files from DriverWizard on "
            "Win98/Me\n", (LPCSTR)g_sErr);
        return FALSE;
    }

    if (!load_functions(LOAD_FUNCTIONS_NT))
        return FALSE;

    /* delete the INF file that was copied during the install */
    if (!DeleteInfCopy(file_name))
        goto Exit;

    if (!UnInstallDeviceInfo(hwid))
        goto Exit;

    fReturn = TRUE;

Exit:
    unload_functions();
    return fReturn;
}

BOOL RescanBus(const char *bus)
{
    BOOL fReturn = FALSE;
    if (!load_functions(LOAD_FUNCTIONS_NT_COMPAT))    
        return FALSE;
    fReturn = EnumDeviceInfo(bus, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES,
        cbReenumDevInfo, NULL, NULL);
    unload_functions();
    return fReturn;
}

#define INSTALL_PENDING_TIMEOUT 18000 // in milliseconds
static BOOL rescan_device_tree(BOOL wait_no_pending_install_events)
{
    DEVINST  devInst;
    DWORD cr_status;

    /* Refresh device manager */
    cr_status = __CM_Locate_DevNode(&devInst, NULL, 0);
    if (cr_status != CR_SUCCESS)
    {
        g_sErr.Format("%sCannot locate top device: %s\n", (LPCSTR)g_sErr,
            (LPCSTR)LastErrString());
        return FALSE;
    }

    cr_status = __CM_Reenumerate_DevNode(devInst, CM_REENUMERATE_SYNCHRONOUS);
    if (cr_status != CR_SUCCESS)
    {
        g_sErr.Format("%sCannot reenumerate device tree (CR_ERRCODE 0x%lx)\n",
            (LPCSTR)g_sErr, cr_status);
        return FALSE;
    }

    if (!wait_no_pending_install_events)
        return TRUE;

    /* Wait for pending installation events to complete */
    DWORD dwWaitStatus =
        __CMP_WaitNoPendingInstallEvents(INSTALL_PENDING_TIMEOUT);
    if (dwWaitStatus != WAIT_OBJECT_0) // Wait failed or timed-out
    {
        g_sErr.Format("%sWarning: Failed waiting for install events to "
            "complete:\n", (LPCSTR)g_sErr);

        if (dwWaitStatus == WAIT_TIMEOUT)
        {
            g_sErr.Format("%sTimeout (%ld ms) expired before install events "
                "completed.\n", (LPCSTR)g_sErr, INSTALL_PENDING_TIMEOUT);
        }
        else /* dwWaitStatus == WAIT_FAILED */
            g_sErr.Format("%s%s\n", (LPCSTR)g_sErr, (LPCSTR)LastErrString());
    }

    return TRUE;
}

BOOL install_inf(const char *hwid, const char *file_name_with_path)
{
    CCString file_name = file_name_with_path;
    replace_slashes((PSTR)file_name, FALSE);
    BOOL fReturn = FALSE;
    OS_TYPE iOsType = get_os_type();
    BOOL Found;

    if (iOsType == OS_WIN_98)
    {
        g_sErr.Format("%sCannot install INF files from DriverWizard on "
            "Win98/Me\n", (LPCSTR)g_sErr);
        return FALSE;
    }

    if (!load_functions(LOAD_FUNCTIONS_NT))
        return FALSE;

    if (!CopyInfFile(file_name))
    {
        g_sErr.Format("%sWarning: cannot copy the INF file for device "
            "(hwid:%s): %s\n", (LPCSTR)g_sErr, hwid, (LPCSTR)LastErrString());
    }
  
    /* Try updating the driver */
    if (!EnumDeviceInfo(NULL, hwid, DIGCF_PRESENT | DIGCF_ALLCLASSES,
        &cbUpdateDriver, (void *)file_name, &Found))
    {
        goto Exit;
    }

    if (!Found)
    {
        if (!rescan_device_tree(FALSE))
            goto Exit;
        
        /* Retry updating the driver */
        if (!EnumDeviceInfo(NULL, hwid, DIGCF_PRESENT | DIGCF_ALLCLASSES,
            &cbUpdateDriver, (void *)file_name, &Found))
        {
            goto Exit;
        }

        if (!Found)
        {
            g_sErr.Format("%sWarning: the device (hwid:%s) is not "
                "plugged-in.\n", (LPCSTR)g_sErr, hwid);
        }
    }

    fReturn = TRUE;
    
Exit:
    unload_functions();
    return fReturn;
}

static BOOL cbReenumDevInfo(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx)
{
    DWORD cr_status = __CM_Reenumerate_DevNode(DeviceInfo.DevInst,
        CM_REENUMERATE_SYNCHRONOUS);
    if (cr_status != CR_SUCCESS)
    {
        g_sErr.Format("%sCannot reenumerate device tree (CR_ERRCODE 0x%lx)\n",
            (LPCSTR)g_sErr, cr_status);
        return FALSE;
    }
    return TRUE;
}

static BOOL cbUpdateDriver(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx)
{
    BOOL fRebootRequired = FALSE;
    char *file_name = (char *)((CCString *)f_ctx);

    /* Device exists, initialize installation process */
    if (!__UpdateDriverForPlugAndPlayDevices(NULL, hwid, (char *)file_name,
        INSTALLFLAG_FORCE, &fRebootRequired))
    {
        DWORD dwLastErr;
        dwLastErr = GetLastError();

        g_sErr.Format("%sError updating the driver (hwid:%s) with the INF "
            "file: %s\n", (LPCSTR)g_sErr, hwid,
            (dwLastErr == ERROR_SHARING_VIOLATION) ? 
            "\nVerify that the device is connected and the INF file is not "
            "currently being\nused and retry installing the INF file."
            : (LPCSTR)LastErrString());
        return FALSE;
    }
    
    if (fRebootRequired)
        g_fRebootRequired = TRUE;

    return TRUE;
}

BOOL ExistsDeviceInfo(const char *hwid, BOOL wait_no_pending_install_events,
    BOOL *devnode_exists, BOOL *devnode_found, BOOL do_rescan)
{
    int pass;

    /*
     * Since the device tree may contain phantom device nodes. In order to
     * avoid redundant rescans, we check the existance of a device in two
     * passes:
     *
     * Pass 1: just check, if check fails then rescan.
     * Pass 2: check again.
     *
     * The function returns TRUE if node exists and is configured.
     * If the node does not exist at all, or exists but is not configured
     * (phantom) we return FALSE.
     */
    for (pass = 1; pass<=2; pass++)
    {    
        EnumDeviceInfo(NULL, hwid, DIGCF_ALLCLASSES, cbExistsDevnode,
            devnode_exists, devnode_found);

        if (*devnode_found && *devnode_exists)
            return TRUE;

        if (pass == 2 || !do_rescan)
            return FALSE;

        if (do_rescan) 
            rescan_device_tree(wait_no_pending_install_events);
    }

    /* unreachable code */
    return TRUE;
}

BOOL UnInstallDeviceInfo(const char *hwid)
{
    BOOL Found;

    if (!EnumDeviceInfo(NULL, hwid, DIGCF_ALLCLASSES, cbDelete, NULL, &Found))
        return FALSE;

    if (!Found)
    {
        g_sErr.Format("%sWarning: the device (hwid:%s) is not installed.\n", 
            (LPCSTR)g_sErr, hwid);
    }

    return TRUE;
}

static BOOL cbExistsDevnode(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx)
{
    DWORD problem_number, status, *devnode_exists;
    
    devnode_exists = (DWORD *)f_ctx;
    if (*devnode_exists)
        return TRUE;

    if (CR_NO_SUCH_DEVNODE != __CM_Get_DevNode_Status(&status, &problem_number,
        DeviceInfo.DevInst, 0))
    {
        *devnode_exists = TRUE;
    }
    return TRUE;
}

static BOOL cbDelete(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx)
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    SP_DEVINSTALL_PARAMS devParams;
  
    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;

    /*remove the device */
    if(!__SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfo,
        &rmdParams.ClassInstallHeader, sizeof(rmdParams)))
    {
        g_sErr.Format("%sError setting install params for removing device: "
            "%s\n", (LPCSTR)g_sErr, (LPCSTR)LastErrString()); 
        return FALSE;
    }

    if (!__SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfoSet, &DeviceInfo))
    {
        g_sErr.Format("%sError removing device: %s\n", (LPCSTR)g_sErr,
            (LPCSTR)LastErrString()); 
        return FALSE;
    }

    /* check if reboot is required */
    devParams.cbSize = sizeof(devParams);
    if (!__SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfo,
        &devParams))
    {
        g_sErr.Format("%sError getting install params for removed device: "
            "%s\n", (LPCSTR)g_sErr, (LPCSTR)LastErrString()); 
        return FALSE;
    }

    if (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))
        g_fRebootRequired = TRUE;

    return TRUE;
}

BOOL ChangeStateDeviceInfo(const char *hwid, DWORD dwNewState)
{
    BOOL Found;
    
    if (!EnumDeviceInfo(NULL, hwid, DIGCF_PRESENT | DIGCF_ALLCLASSES,
        cbChangeState, (void*)dwNewState, &Found))
    {
        return FALSE;
    }
    
    if (!Found)
    {
        g_sErr.Format("%sError finding the device %s\n", (LPCSTR)g_sErr, hwid);
        return FALSE;
    }

    return TRUE;
}

static BOOL cbChangeState(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA DeviceInfo,
    const char *hwid, void *f_ctx)
{
    DWORD dwNewState = (DWORD)f_ctx;

    SP_PROPCHANGE_PARAMS PropChangeParams = {sizeof(SP_CLASSINSTALL_HEADER)};
    SP_DEVINSTALL_PARAMS devParams;

    PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    PropChangeParams.Scope = DICS_FLAG_GLOBAL;
    PropChangeParams.StateChange = dwNewState; 
    PropChangeParams.HwProfile = 0;

    if (!__SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfo,
        (SP_CLASSINSTALL_HEADER *)&PropChangeParams, sizeof(PropChangeParams)))
    {
        g_sErr.Format("%sError setting the install parameters for device %s "
            "(GLOBAL): %s\n", (LPCSTR)g_sErr, hwid, (LPCSTR)LastErrString()); 
        return FALSE;
    }
    PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    PropChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    PropChangeParams.StateChange = dwNewState; 
    PropChangeParams.HwProfile = 0;

    if (!__SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfo, 
        (SP_CLASSINSTALL_HEADER *) &PropChangeParams, sizeof(PropChangeParams)))
    {
        g_sErr.Format("%sError setting the install parameters for device %s "
            "(SPECIFIC): %s\n", (LPCSTR)g_sErr, hwid, (LPCSTR)LastErrString()); 
        return FALSE;
    }
    if (!__SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet,
        &DeviceInfo))
    {
        g_sErr.Format("%sError changing the device status for device %s: %s\n",
            (LPCSTR)g_sErr, hwid, (LPCSTR)LastErrString()); 
        return FALSE;
    }
    
    /* see if device needs reboot */
    devParams.cbSize = sizeof(devParams);
    if (!__SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfo,
        &devParams))
    {
        g_sErr.Format("%sError getting the install parameters for device %s: "
            "%s\n", (LPCSTR)g_sErr, hwid, (LPCSTR)LastErrString()); 
        return FALSE;
    } 
    if (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))
        g_fRebootRequired = TRUE;
    
    return TRUE;
}

static BOOL GetProperty(HDEVINFO *pDeviceInfoSet,
    SP_DEVINFO_DATA *pDeviceInfoData, char **buffer, DWORD *buffersize,
    DWORD prop, BOOL *pfErrorOccurred)
{
    BOOL fReturn;
    DWORD DataT, dwLastError;

    /*
     * Call function with null to begin with, then use the returned buffer size
     * to Alloc the buffer. Keep calling until success or an unknown failure.
     * */

    while (!(fReturn = __SetupDiGetDeviceRegistryProperty(*pDeviceInfoSet,
        pDeviceInfoData, prop, &DataT, (PBYTE)*buffer, *buffersize,
        buffersize)))
    {
        dwLastError = GetLastError();
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            // Change the buffer size.
            if (*buffer) LocalFree(*buffer);
            *buffer = (char *)LocalAlloc(LPTR, *buffersize);
        }
        else
        {
            if (dwLastError != ERROR_INVALID_DATA)
                *pfErrorOccurred = TRUE;
            break;
        }
    }

    return fReturn;
}
  
static BOOL MatchDevice(const char *hwid, HDEVINFO *pDeviceInfoSet,
    SP_DEVINFO_DATA *pDeviceInfoData, DWORD prop, BOOL *pfErrorOccurred)
{
    BOOL fReturn = FALSE;
    LPTSTR hwids_buf = NULL;
    DWORD dwLength = 0;
    LPTSTR p;

    /* Compare each entry in the buffer multi-sz lists with our HardwareID. */
    if (!GetProperty(pDeviceInfoSet, pDeviceInfoData, &hwids_buf, &dwLength,
        prop, pfErrorOccurred))
    {
        goto Exit;
    }

    for (p = hwids_buf; *p && p < hwids_buf + dwLength;
        p += lstrlen(p) + sizeof(TCHAR))
    {
        if (!_tcsicmp(hwid, p))
        {
            fReturn = TRUE;
            break;
        }
    }

Exit:
    if (hwids_buf)
        LocalFree(hwids_buf);

    return fReturn;
}

/**
 * Function name:  EnumDeviceInfo
 * Description:    
 * Parameters:
 *     @enumerator: Which enumerator to use: NULL, "root", "pci", "acpi", "usb"
 *         etc.
 *     @hwid: hardware id to match the device, e.g. "PCI\\VEN_1172&DEV_0006"
 *     @devices_type: e.g. DIGCF_PRESENT | DIGCF_ALLCLASSES
 *     @cb: callback to call when a matching device is found
 *     @f_ctx: callback context
 *     @found: was any match found 
 *
 * Return value: On error return FALSE, on success return the return
 *     value of the callback.
 * Scope: Global
 **/
BOOL EnumDeviceInfo(const char *enumerator, const char *hwid,
    DWORD devices_type, DEV_FUNC cb, void *f_ctx, BOOL *found)
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i, errorCode;
    BOOL fErrorOccurred = FALSE;
    BOOL fReturn = TRUE;
    BOOL match_hwid, status;

    if (found)
        *found = FALSE;

    DeviceInfoSet = __SetupDiGetClassDevs(NULL, enumerator, NULL, devices_type);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        g_sErr.Format("%sSetupDiGetClassDevs failed with error: %d - %s\n",
            (LPCSTR)g_sErr, GetLastError(), (LPCSTR)LastErrString());
        return FALSE;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0; ; i++)
    {
        status = __SetupDiEnumDeviceInfo(DeviceInfoSet, i, &DeviceInfoData);

        if (status == FALSE)
        {
            errorCode = GetLastError();
            if (errorCode != ERROR_NO_MORE_ITEMS)
            {
                g_sErr.Format("%sSetupDiEnumDeviceInfo failed with "
                    "error: %d - %s\n", (LPCSTR)g_sErr, errorCode,
                    (LPCSTR)LastErrString());
            }
            break;
        }
        
        match_hwid = !hwid || !strlen(hwid) ? TRUE :
            MatchDevice(hwid, &DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID,
            &fErrorOccurred) || MatchDevice(hwid, &DeviceInfoSet,
            &DeviceInfoData, SPDRP_COMPATIBLEIDS, &fErrorOccurred);

        if (match_hwid)
        {
            if (found)
                *found = TRUE;

            if (cb)
                fReturn = cb(DeviceInfoSet, DeviceInfoData, hwid, f_ctx);
        }
    }

    if (fErrorOccurred)
    {
        g_sErr.Format("%sCouldn't get the hardware IDs of all the devices\n",
            (LPCSTR)g_sErr);
    }

    __SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return fReturn;
}

BOOL InstallDeviceInfo(const char *hwid, const char *classname, GUID *pGUID)
{
    BOOL fReturn = FALSE;
    PBYTE bsHwid = NULL;
    DWORD size;

    /* Create an empty device info set to act as a container for the one device
     * information element we'll create */

    HDEVINFO hInfoList = __SetupDiCreateDeviceInfoList(pGUID, NULL);
    if (hInfoList == INVALID_HANDLE_VALUE)
    {
        g_sErr.Format("%sError creating empty device info list: %s\n",
            (LPCSTR)g_sErr, (LPCSTR)LastErrString());
        goto Exit;
    }

    /* Create a device info element. This step creates the hardware key in the
     * registry and sets the designated hardware id. The way we call
     * SetupDiCreateDeviceInfo, the key will be a subkey of the class key and
     * have an autogenerated name. (Note that the DDK doc is wrong when it says
     * you supply a device identifier for the name parameter.) */

    SP_DEVINFO_DATA DevInfo; /* information element for one device */
    memset(&DevInfo, 0, sizeof(DevInfo));
    DevInfo.cbSize = sizeof(DevInfo);
    if (!__SetupDiCreateDeviceInfo(hInfoList, classname, pGUID, NULL, NULL,
        DICD_GENERATE_ID, &DevInfo))
    {
        g_sErr.Format("%sError creating device info element: %s\n",
            (LPCSTR)g_sErr, (LPCSTR)LastErrString());
        goto Exit;
    }

    size = (DWORD)((strlen(hwid) + 2) * sizeof(TCHAR));
    bsHwid = new BYTE[size];
    memset(bsHwid, 0, size);
    memcpy(bsHwid, (LPCTSTR)hwid, size - 2 * sizeof(TCHAR));

    if (!__SetupDiSetDeviceRegistryProperty(hInfoList, &DevInfo,
        SPDRP_HARDWAREID, bsHwid, size))
    {
        g_sErr.Format("%sError setting device hardware id property: hwid %s, "
            "error %lx\nMake sure that the system permits addition of new "
            "devices under \"%s\" class\n", (LPCSTR)g_sErr, bsHwid,
            GetLastError(), classname);
        goto Exit;
    };

    /* Register the device with PnP. This step creates a DEVNODE, but there's
     * still no driver associated with the device at this point */
    if (!__SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hInfoList, &DevInfo))
    {
        g_sErr.Format("%sError calling class installer register: %s\n",
            (LPCSTR)g_sErr, (LPCSTR)LastErrString()); 
        goto Exit;
    }

    fReturn = TRUE;

Exit:
    if (hInfoList != INVALID_HANDLE_VALUE)
        __SetupDiDestroyDeviceInfoList(hInfoList);
    if (bsHwid)
        delete[] bsHwid;
    return fReturn;
}

static BOOL CompareInf(const char *DestInfFilePath,
    const char *SourceInfFilePath)
{
    DWORD src_bytes, dest_bytes;
    HANDLE h_dest, h_src;
    static char src_buf[128], dest_buf[128];
    DWORD src_bytes_read, dest_bytes_read;
    DWORD error = -1;

    h_src = CreateFile(SourceInfFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, 0, NULL);
    h_dest = CreateFile(DestInfFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, 0, NULL);
    if (h_src == INVALID_HANDLE_VALUE || h_dest == INVALID_HANDLE_VALUE)
    {
        g_sErr.Format("%sINF error opening file: %s\n", (LPCSTR)g_sErr,
            (LPCSTR)LastErrString());  
        return FALSE;
    }

    /* Compare sizes */
    src_bytes = GetFileSize(h_src, NULL);
    dest_bytes = GetFileSize(h_dest, NULL);
    if (src_bytes != dest_bytes)
        goto Exit;

    /* Binary compare the contents */
    for (; src_bytes>0; src_bytes-=src_bytes_read)
    {
        if (!ReadFile(h_src, src_buf, sizeof(src_buf), &src_bytes_read, NULL)
            || !ReadFile(h_dest, dest_buf, sizeof(dest_buf), &dest_bytes_read,
            NULL))
        {
            error = GetLastError();
            goto Exit;
        }

        if (0 != memcmp(src_buf, dest_buf, src_bytes_read))
            goto Exit;
    }

    error = 0;
Exit:
    CloseHandle(h_src);
    CloseHandle(h_dest);

    if (error && (error != ERROR_HANDLE_EOF))
        return FALSE;

    return TRUE;
}

#if !defined(SUOI_FORCEDELETE)
#define SUOI_FORCEDELETE 0x00000001 // defined in setupapi.h for WinXP and later
#endif

static BOOL DeleteInfCopy(const char *SourceInfFilePath)
{  
    CCString PnfCopyPath, DestInfFilePath;
    DWORD RequiredSize = 0;
    char *ReturnBuffer;
    const char *DestInf;
    DWORD found_inf = 0;
    LPCSTR last_error = NULL;
    static char win_dir[MAX_PATH]; // define static to save space on stack

    /* GetWindowsDirectory is equivalent to %WINDIR% */
    if (!GetWindowsDirectory(win_dir, MAX_PATH-1))
    {  
        g_sErr.Format("%sINF error getting windows directory: %s\n",
            (LPCSTR)g_sErr, (LPCSTR)LastErrString());  
        return FALSE;  
    }

    if (!__SetupGetInfFileList(NULL, INF_STYLE_WIN4, NULL, 0, &RequiredSize)
        || !RequiredSize)
    {
        g_sErr.Format("%sINF cannot get a list of INF files: %s\n",
            (LPCSTR)g_sErr, (LPCSTR)LastErrString());  
        return FALSE;
    }

    ReturnBuffer = (char *)malloc(RequiredSize);
    if (!ReturnBuffer)
    {
        g_sErr.Format("%sINF failed allocating %ld bytes\n", (LPCSTR)g_sErr,
            RequiredSize);
        return FALSE;
    }
            
    /* Get a list of INF files in %windir%\inf */
    __SetupGetInfFileList(NULL, INF_STYLE_WIN4, ReturnBuffer, RequiredSize,
        NULL);
    for (DestInf = ReturnBuffer; *DestInf; DestInf += strlen(DestInf) + 1)
    {
        DestInfFilePath = CCString(win_dir) + "\\INF\\" + DestInf;
        
        if (!CompareInf(DestInfFilePath, SourceInfFilePath))
            continue;

        found_inf ++;
        if (__SetupUninstallOEMInf) /* The function exists since WinXP */
        {
            /* Delete INF and any associated PNF and CAT */
            if (!__SetupUninstallOEMInf(DestInf, SUOI_FORCEDELETE, NULL))
            {
                last_error = (LPCSTR)LastErrString();
                break;
            }
        }
        else
        {
            /* Delete INF and PNF */
            if (!DeleteFile((LPCSTR)DestInfFilePath))
            {  
                last_error = (LPCSTR)LastErrString();
                break;
            }  
            PnfCopyPath = DestInfFilePath.Mid(0, DestInfFilePath.Length()-3) +
                "PNF";  
            DeleteFile((LPCSTR)PnfCopyPath);  
        }
    }

    if (last_error)
    {
        g_sErr.Format("%sINF copy %%WINDIR%%\\%s cannot be deleted: %s\n",   
            (LPCSTR)g_sErr, DestInf, (LPCSTR)LastErrString());
        free(ReturnBuffer);
        return FALSE;
    }

    if (!found_inf)
    {
        g_sErr.Format("%sWarning: INF copy for %s not found => not deleted.\n", 
            (LPCSTR)g_sErr, SourceInfFilePath);
    }
    free(ReturnBuffer);
    return TRUE;
}  

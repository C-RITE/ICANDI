/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - common_install.h
//
// A header file for the Installation library.
////////////////////////////////////////////////////////////////

#ifndef _COMMON_INSTALL_H_
#define _COMMON_INSTALL_H_

#include <windows.h>
#include <setupapi.h>
#include <difxapi.h>
#include <cfgmgr32.h>
#include "cstring.h"
#include "utils.h"
#include "windrvr.h"

extern BOOL g_fRebootRequired;
CCString LastErrString();
CCString InstallLastErrorString();

// Installation Functions

#define LOAD_FUNCTIONS_NEWDEV   0x01
#define LOAD_FUNCTIONS_SETUPAPI 0x02
#define LOAD_FUNCTIONS_CFGMGR32 0x04
#define LOAD_FUNCTIONS_DIFXAPI  0x08

#define LOAD_FUNCTIONS_NT \
    (LOAD_FUNCTIONS_NEWDEV | LOAD_FUNCTIONS_SETUPAPI | LOAD_FUNCTIONS_DIFXAPI)
#define LOAD_FUNCTIONS_NT_COMPAT \
    (LOAD_FUNCTIONS_NEWDEV | LOAD_FUNCTIONS_SETUPAPI)
#define LOAD_FUNCTIONS_WIN9X \
    (LOAD_FUNCTIONS_NEWDEV | LOAD_FUNCTIONS_SETUPAPI | LOAD_FUNCTIONS_CFGMGR32)

extern "C" BOOL load_functions(ULONG mask);
extern "C" void unload_functions();
BOOL install_inf(const char *hid, const char *file_name_with_path);
BOOL uninstall_inf(const char *hid, const char *file_name_with_path);
BOOL install_inf_section(const char *file_name_with_path);
extern "C" BOOL RescanBus(const char *bus);

// Device Info functions prototypes

BOOL ExistsDeviceInfo(const char *hid, BOOL wait_no_pending_install_events,
    BOOL *devnode_exists, BOOL *devnode_found, BOOL do_rescan);
BOOL RemoveDeviceInfo(const char *hid);
BOOL ChangeStateDeviceInfo(const char *hid, DWORD dwNewState);
BOOL InstallDeviceInfo(const char *hid, const char *classname, GUID *pGUID);
BOOL UnInstallDeviceInfo(const char *hid);

// Dynamic Functions Definitions

typedef BOOL (WINAPI *t_pfi_SetupGetStringField)(PINFCONTEXT, DWORD, PTSTR,
  DWORD, PDWORD);
typedef DWORD (WINAPI *t_pfi_SetupGetFieldCount)(PINFCONTEXT);
typedef BOOL (WINAPI *t_pfi_SetupFindNextLine)(PINFCONTEXT, PINFCONTEXT);
typedef HINF (WINAPI *t_pfi_SetupOpenInfFile)(PCTSTR, PCTSTR, DWORD, PUINT);
typedef void (WINAPI *t_pfi_SetupCloseInfFile)(HINF);
typedef BOOL (WINAPI *t_pfi_SetupFindFirstLine)(HINF, PCTSTR, PCTSTR,
    PINFCONTEXT);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_locate)(PDEVINST, DEVINSTID, ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_reenumerate)(DEVINST , ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_status)(PULONG, PULONG, DEVINST,
    ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_child)(PDEVINST, DEVINST, ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_parent)(PDEVINST, DEVINST, ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_sibling)(PDEVINST, DEVINST,
    ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_device_ida)(DEVINST, PCHAR, ULONG,
    ULONG);
typedef CMAPI CONFIGRET (WINAPI *t_pfi_cm_get_device_id_size)(PULONG,DEVINST,
    ULONG);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_open_device_infoa)(HDEVINFO, 
    PCSTR, HWND, DWORD, PSP_DEVINFO_DATA);

typedef BOOL (WINAPI *t_pfi_updatedriver)(HWND, LPCSTR, LPCSTR, DWORD, PBOOL);
typedef HDEVINFO (WINAPI *t_pfi_setup_di_get_class)(LPGUID, PCTSTR, HWND, DWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_enum_dev)(HDEVINFO, DWORD,
    PSP_DEVINFO_DATA);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_get_device_reg)(HDEVINFO,
    PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_get_device_reg_w)(HDEVINFO,
    PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_set_device_reg)(HDEVINFO,
    PSP_DEVINFO_DATA, DWORD, CONST BYTE *, DWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_setup_di_destroy)(HDEVINFO);
typedef BOOL (WINAPI *t_pfi_setup_copy_oem_inf)(PCTSTR, PCTSTR, DWORD,
  DWORD, PTSTR, DWORD, PDWORD, PTSTR);
typedef BOOL (WINAPI *t_pfi_SetupUninstallOEMInf)(PCTSTR, DWORD, PVOID);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiRemoveDevice)(HDEVINFO,
    PSP_DEVINFO_DATA);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiCreateDeviceInfo)
    (HDEVINFO, PCTSTR, LPGUID, PCTSTR, HWND, DWORD, PSP_DEVINFO_DATA);
typedef WINSETUPAPI HDEVINFO (WINAPI *t_pfi_SetupDiCreateDeviceInfoList)(LPGUID,
    HWND);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiCallClassInstaller)
    (DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiGetINFClass)
    (PCTSTR, LPGUID, PTSTR, DWORD, PDWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiClassGuidsFromName)
    (PCTSTR, LPGUID, DWORD, PDWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiSetClassInstallParams)(HDEVINFO,
    PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD);
typedef WINSETUPAPI BOOL (WINAPI *t_pfi_SetupDiGetDeviceInstallParams)(HDEVINFO,
    PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
typedef HSPFILEQ (WINAPI *t_pfi_SetupOpenFileQueue)(VOID);
typedef BOOL (WINAPI * t_pfi_SetupInstallFilesFromInfSection)(HINF InfHandle, 
    HINF LayoutInfHandle, HSPFILEQ FileQueue, PCTSTR SectionName,
    PCTSTR SourceRootPath, UINT CopyFlags);
typedef BOOL (WINAPI * t_pfi_SetupCommitFileQueue)(HWND Owner, HSPFILEQ QueueHandle,
    PSP_FILE_CALLBACK MsgHandler, PVOID Context);
typedef void (WINAPI * t_pfi_SetupCloseFileQueue)(HSPFILEQ QueueHandle);
typedef PVOID (WINAPI * t_pfi_SetupInitDefaultQueueCallback)(HWND OwnerWindow);
typedef WINSETUPAPI UINT (WINAPI *t_pfi_SetupDefaultQueueCallback)(PVOID Context,
    UINT Notification, UINT_PTR Param1, UINT_PTR Param2);
typedef void (WINAPI * t_pfi_SetupTermDefaultQueueCallback)(PVOID Context);
typedef DWORD (WINAPI * t_pfi_CMP_WaitNoPendingInstallEvents)(DWORD dwTimeout);
typedef BOOL (WINAPI * t_pfi_SetupDiGetSelectedDriverA)(IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData, OPTIONAL OUT PSP_DRVINFO_DATA_A DriverInfoData);
typedef BOOL (WINAPI * t_pfi_SetupDiGetDriverInfoDetailA)(IN  HDEVINFO                  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA          DeviceInfoData,           OPTIONAL
    IN  PSP_DRVINFO_DATA_A        DriverInfoData,
    OUT PSP_DRVINFO_DETAIL_DATA_A DriverInfoDetailData,     OPTIONAL
    IN  DWORD                     DriverInfoDetailDataSize,
    OUT PDWORD                    RequiredSize              OPTIONAL
    );
typedef BOOL (WINAPI * t_pfi_SetupGetInfInformationA) (IN  LPCVOID             InfSpec,
    IN  DWORD               SearchControl,
    OUT PSP_INF_INFORMATION ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    );
typedef BOOL (WINAPI * t_pfi_SetupGetInfFileListA) (
    IN  PCTSTR  DirectoryPath,
    IN  DWORD   InfStyle,
    OUT PTSTR   ReturnBuffer,
    IN  DWORD   ReturnBufferSize,
    OUT PDWORD  RequiredSize     OPTIONAL
    );
typedef BOOL (WINAPI * t_pfi_SetupQueryInfOriginalFileInformationA) (IN  PSP_INF_INFORMATION      InfInformation,
    IN  UINT                     InfIndex,
    IN  PSP_ALTPLATFORM_INFO     AlternatePlatformInfo, OPTIONAL
    OUT PSP_ORIGINAL_FILE_INFO_A OriginalFileInfo
    );
typedef VOID (WINAPI * t_pfi_DIFXAPISetLogCallbackA) (
    IN DIFXAPILOGCALLBACK_A LogCallback,
    IN PVOID CallbackContext
    );
typedef DWORD (WINAPI * t_pfi_DriverPackageGetPathA) (
    IN PCSTR DriverPackageInfPath,
    OUT PSTR pDestInfPath, 
    OUT DWORD * pNumOfChars
    );
typedef DWORD (WINAPI * t_pfi_DriverPackageInstallA) (
    IN PCSTR DriverPackageInfPath, 
    IN DWORD Flags,
    IN PCINSTALLERINFO_A pInstallerInfo,
    OUT BOOL * pNeedReboot 
    );
typedef DWORD (WINAPI * t_pfi_DriverPackagePreinstallA) (
    IN PCSTR DriverPackageInfPath,
    IN DWORD Flags
    );
typedef DWORD (WINAPI * t_pfi_DriverPackageUninstallA) (
    IN PCSTR DriverPackageInfPath,
    IN DWORD Flags,
    IN PCINSTALLERINFO_A pInstallerInfo,
    OUT BOOL * pNeedReboot    
    );

extern "C"
{
extern t_pfi_SetupTermDefaultQueueCallback __SetupTermDefaultQueueCallback;
extern t_pfi_SetupDefaultQueueCallback __SetupDefaultQueueCallback;
extern t_pfi_SetupInitDefaultQueueCallback __SetupInitDefaultQueueCallback;
extern t_pfi_SetupOpenFileQueue __SetupOpenFileQueue;
extern t_pfi_SetupInstallFilesFromInfSection __SetupInstallFilesFromInfSection;
extern t_pfi_SetupCommitFileQueue __SetupCommitFileQueue;
extern t_pfi_SetupCloseFileQueue __SetupCloseFileQueue;
extern t_pfi_SetupGetStringField __SetupGetStringField;
extern t_pfi_SetupGetFieldCount __SetupGetFieldCount;
extern t_pfi_SetupFindNextLine __SetupFindNextLine;
extern t_pfi_SetupOpenInfFile __SetupOpenInfFile;
extern t_pfi_SetupCloseInfFile __SetupCloseInfFile;
extern t_pfi_SetupFindFirstLine __SetupFindFirstLine;
extern t_pfi_cm_locate __CM_Locate_DevNode;
extern t_pfi_cm_reenumerate __CM_Reenumerate_DevNode;
extern t_pfi_cm_get_status __CM_Get_DevNode_Status;
extern t_pfi_cm_get_child __CM_Get_Child;
extern t_pfi_cm_get_parent __CM_Get_Parent;
extern t_pfi_cm_get_sibling __CM_Get_Sibling;
extern t_pfi_cm_get_device_ida __CM_Get_Device_IDA;
extern t_pfi_cm_get_device_id_size __CM_Get_Device_ID_Size;
extern t_pfi_setup_di_open_device_infoa __SetupDiOpenDeviceInfoA;
extern t_pfi_SetupDiCreateDeviceInfoList __SetupDiCreateDeviceInfoList;
extern t_pfi_updatedriver __UpdateDriverForPlugAndPlayDevices;
extern t_pfi_setup_di_get_class __SetupDiGetClassDevs;
extern t_pfi_setup_di_enum_dev __SetupDiEnumDeviceInfo;
extern t_pfi_setup_di_get_device_reg __SetupDiGetDeviceRegistryProperty;
extern t_pfi_setup_di_get_device_reg_w __SetupDiGetDeviceRegistryPropertyW;
extern t_pfi_setup_di_set_device_reg __SetupDiSetDeviceRegistryProperty;
extern t_pfi_setup_di_destroy __SetupDiDestroyDeviceInfoList;
extern t_pfi_setup_copy_oem_inf __SetupCopyOEMInf;
extern t_pfi_SetupUninstallOEMInf __SetupUninstallOEMInf;
extern t_pfi_SetupDiRemoveDevice __SetupDiRemoveDevice;
extern t_pfi_SetupDiGetINFClass __SetupDiGetINFClass;
extern t_pfi_SetupDiClassGuidsFromName __SetupDiClassGuidsFromName;
extern t_pfi_SetupDiSetClassInstallParams __SetupDiSetClassInstallParams;
extern t_pfi_SetupDiGetDeviceInstallParams __SetupDiGetDeviceInstallParams;
extern t_pfi_CMP_WaitNoPendingInstallEvents __CMP_WaitNoPendingInstallEvents;
extern t_pfi_SetupDiGetSelectedDriverA __SetupDiGetSelectedDriver;
extern t_pfi_SetupDiGetDriverInfoDetailA __SetupDiGetDriverInfoDetail;
extern t_pfi_SetupGetInfInformationA __SetupGetInfInformation;
extern t_pfi_SetupGetInfFileListA __SetupGetInfFileList;
extern t_pfi_SetupQueryInfOriginalFileInformationA __SetupQueryInfOriginalFileInformation;
extern t_pfi_SetupDiGetDriverInfoDetailA __SetupDiGetDriverInfoDetail;
extern t_pfi_DIFXAPISetLogCallbackA __DIFXAPISetLogCallback;
extern t_pfi_DriverPackageGetPathA __DriverPackageGetPath;
extern t_pfi_DriverPackageInstallA __DriverPackageInstall;
extern t_pfi_DriverPackagePreinstallA __DriverPackagePreinstall;
extern t_pfi_DriverPackageUninstallA __DriverPackageUninstall;
}

#endif


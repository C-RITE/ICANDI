/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - DriverVXD.cpp
//
// Implementation of WDREG's actions for Windows95/98 VXD drivers.
////////////////////////////////////////////////////////////////

#include "common_install.h"
#include "Driver.h"
#include "ui.h"

// VXDLDR error codes
#define VXDLDR_ERR_OUT_OF_MEMORY    1
#define VXDLDR_ERR_IN_DOS       2
#define VXDLDR_ERR_FILE_OPEN_ERROR  3
#define VXDLDR_ERR_FILE_READ        4
#define VXDLDR_ERR_DUPLICATE_DEVICE 5
#define VXDLDR_ERR_BAD_DEVICE_FILE  6
#define VXDLDR_ERR_DEVICE_REFUSED   7
#define VXDLDR_ERR_NO_SUCH_DEVICE   8
#define VXDLDR_ERR_DEVICE_UNLOADABLE    9
#define VXDLDR_ERR_ALLOC_V86_AREA   10
#define VXDLDR_ERR_BAD_API_FUNCTION 11

#define VXDLDR_ERR_MAX          11

// VXDLDR driver functions
#define VXDLDR_APIFUNC_GETVERSION   0
#define VXDLDR_APIFUNC_LOADDEVICE   1
#define VXDLDR_APIFUNC_UNLOADDEVICE 2

#define VXD_REG_SERVICES "System\\CurrentControlSet\\Services\\VxD\\"

#define REG_SET_VAL(hKey, lpValueName, Reserved, dwType, lpData, cbData) \
    { \
        LONG rc = RegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData); \
        if (rc != ERROR_SUCCESS) \
        { \
            DisplayError("Cannot set registry value \"%s\": %s\n", \
                (LPCSTR)(sKey+"\\"+(lpValueName)), (LPCSTR)LastErrString()); \
            RegCloseKey(hKey); \
            return FALSE; \
        } \
    }

CDriverVXD::CDriverVXD(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel) :
    CDriverNonWDM(sDriverName, sDriverFile, dwStartLevel)
{
    char sSystemPath[1024];
    GetSystemDirectory(sSystemPath, sizeof(sSystemPath));
    if (sSystemPath[strlen(sSystemPath)-1] != '\\' ) 
        strcat(sSystemPath,"\\");
    m_sDriverPath.Format("%sVmm32\\%s.VXD", sSystemPath, (LPCSTR) m_sDriverFile);
}

CDriverVXD::~CDriverVXD()
{
    if (m_hSCM)
        CloseHandle(m_hSCM);
}

BOOL CDriverVXD::Init()
{
    m_hSCM = (SC_HANDLE)CreateFile(
        "\\\\.\\VXDLDR",                // device driver name
        0,                              // access
        0,                              // share mode
        0,                              // security attributes
        CREATE_NEW,                     // create options
        FILE_FLAG_DELETE_ON_CLOSE,      // flags
        0);                             // template

    if (m_hSCM == INVALID_HANDLE_VALUE)
    {
        DisplayError("Cannot open service control manager\n");
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverVXD::Create()
{
    return CreateRegTree();
}

BOOL CDriverVXD::Delete()
{
    return DeleteRegTree();
}

BOOL CDriverVXD::Start()
{
    ULONG nRet;

    if (!DeviceIoControl(
        m_hSCM,                         // handle
        VXDLDR_APIFUNC_LOADDEVICE,      // control code (see vxdldr.h in DDK)
        (void *)(LPCSTR)m_sDriverPath,  // path of vxd file (No \\.\ prefix)
        m_sDriverPath.Length()+1,       // input size
        NULL,                           // output buffer
        0,                              // output buffer size
        &nRet,                          // receives count returned
        NULL))
    {
        DisplayError("DeviceIoControl failed for %s: %s\n", (LPCSTR)m_sDriverPath, (LPCSTR)LastErrString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverVXD::Stop()
{
    typedef struct {
            LONG    vxdid;       // device ID of VxD to unload, or -1 to use name
            CHAR    vxdname[256];// null terminated, case sensitive name of VxD 
                                 //   to unload if VxD ID is -1
    } UNLOAD_INFO;
    int i;
            
    UNLOAD_INFO unloadInfo;
    ULONG status, nRet;

    unloadInfo.vxdid = -1;
    strcpy(unloadInfo.vxdname, (LPCSTR)m_sDriverFile);
    for (i=0; unloadInfo.vxdname[i]; i++)
        unloadInfo.vxdname[i] = toupper(unloadInfo.vxdname[i]);

    for (i=0; i<1000; i++)
    {
        status = DeviceIoControl(
            m_hSCM,                     // handle
            VXDLDR_APIFUNC_UNLOADDEVICE,// control code (see vxdldr.h in DDK)
            (void *) &unloadInfo,       // path of vxd file (No \\.\ prefix)
            sizeof (unloadInfo),        // input size
            NULL,                       // output buffer
            0,                          // output buffer size
            &nRet,                      // receives count returned
            NULL);
        if (status!=1)
            break;
    }
    if (i==0)
    {
        DisplayError("Could not unload driver, or driver was previously loaded \n");
        return FALSE;
    }
    return TRUE; 
}
BOOL CDriverVXD::DeleteRegTree()
{
    CCString sKey(VXD_REG_SERVICES);
    sKey += m_sDriverName;

    LONG rc = RegDeleteKey(HKEY_LOCAL_MACHINE, sKey);
    if (rc != ERROR_SUCCESS)
    {
        DisplayError("Cannot delete registry key \"%s\": %s\n", (LPCSTR)sKey, (LPCSTR)LastErrString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverVXD::CreateRegTree()
{
    HKEY hKey;
    DWORD dwDisposition;

    CCString sKey(VXD_REG_SERVICES);
    sKey += m_sDriverName;

    DWORD rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE, sKey, 
        0, "", 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
    if (rc != ERROR_SUCCESS)
    {
        DisplayError("Cannot create registry key \"%s\": %s\n", (LPCSTR)sKey, (LPCSTR)LastErrString());
        return FALSE;
    }

    if (dwDisposition == REG_OPENED_EXISTING_KEY)
        DisplayMessage("warning - %s already exists in registry\n", (LPCSTR)m_sDriverName);
    REG_SET_VAL(hKey, "Start",     0, REG_BINARY, (PBYTE) "\0", 1);
    REG_SET_VAL(hKey, "StaticVxD", 0, REG_SZ, (PBYTE) (LPCSTR)(CCString("*")+m_sDriverFile), 9);

    RegCloseKey (hKey);

    return TRUE;
}

BOOL CDriverVXD::SetRegTreeStartLevel(DWORD dwStartLevel)
{
    HKEY hKey;

    CCString sKey(VXD_REG_SERVICES);
    sKey += m_sDriverName;

    LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sKey, 0, KEY_ALL_ACCESS, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        DisplayError("Cannot open registry key \"%s\": %s\n", (LPCSTR)sKey, (LPCSTR)LastErrString());
        return FALSE;
    }

    REG_SET_VAL(hKey, "Start", 0, REG_BINARY, (PBYTE) &dwStartLevel, 4);
    RegCloseKey(hKey);
    return TRUE;
}


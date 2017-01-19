/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - Driver98.cpp
//
// Implementation of WDREG's actions for Windows98/Me non-WDM drivers 
// (16-bit).
////////////////////////////////////////////////////////////////
#include "common16.h"
#include "ui.h"
#include "Driver.h"
#include <winerror.h>

#define REG_SET_VAL(hKey, lpValueName, Reserved, dwType, lpData, cbData) \
    { \
        LONG rc = SURegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData); \
        if (rc != ERROR_SUCCESS) \
        { \
            DisplayError("Cannot set registry value \"%s\": %s\n", \
                (PCSTR)(sKey+"\\"+(lpValueName)), (LPCSTR)LastErrString()); \
            RegCloseKey(hKey); \
            return FALSE; \
        } \
    }

CDriver98::CDriver98(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel) :
    CDriverNonWDM(sDriverName, sDriverFile, dwStartLevel)
{
    m_sDriverPath.Format("\\SystemRoot\\System32\\drivers\\%s.SYS", sDriverFile);
}

CDriver98::~CDriver98()
{
}

BOOL CDriver98::Create()
{
    // Set WinDriver to be loaded at boot time
    if (m_sDriverName == CCString(WD_DISPLAY_NAME))
    {
        DisplayError("Use -inf option for " WD_DISPLAY_NAME ".\n");
        return FALSE;
    }

    if (!CreateRegTree())
        return FALSE;

    return TRUE;
}

BOOL CDriver98::Start()
{
    // wdreg16 cannot start dynamically drivers (kernel plugins)
    g_fRebootRequired = TRUE;
    return TRUE;
}

BOOL CDriver98::Stop()
{
    // wdreg16 cannot drivers dynamically unload drivers (Kernel PlugIn-s)
    g_fRebootRequired = TRUE;
    return TRUE;
}

BOOL CDriver98::Delete()
{
    if (m_sDriverName == CCString(WD_DISPLAY_NAME))
    {
        DisplayError("Use -inf option for " WD_DISPLAY_NAME ".\n");
        return FALSE;
    }

    return DeleteRegTree();
}

BOOL CDriver98::DeleteRegTree()
{
    CCString sKey("System\\CurrentControlSet\\Services\\");
    sKey += m_sDriverName;

    LONG rc = RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCSTR) (PCSTR) sKey);
    if (rc != ERROR_SUCCESS)
    {
        DisplayError("Cannot delete registry key \"%s\": %s\n", 
            (PCSTR)sKey, (PCSTR)LastErrString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriver98::CreateRegTree()
{
    HKEY hKey;

    CCString sKey("System\\CurrentControlSet\\Services\\");
    sKey += m_sDriverName;

    DWORD rc = RegCreateKey (HKEY_LOCAL_MACHINE, (LPCSTR)sKey, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        DisplayError("Cannot create registry key \"%s\": %s\n", 
            (PCSTR)sKey, (PCSTR)LastErrString());
        return FALSE;
    }

    DWORD dwVal = 1;
    REG_SET_VAL(hKey, "Start", 0, REG_BINARY, (PBYTE) &m_dwStartLevel, 4);
    REG_SET_VAL(hKey, "Type", 0, REG_BINARY, (PBYTE) &dwVal, 4);
    REG_SET_VAL(hKey, "ErrorControl", 0, REG_BINARY, (PBYTE) &dwVal, 4);
    REG_SET_VAL(hKey, "ImagePath", 0, REG_SZ, (PBYTE) (PCSTR)m_sDriverPath, 41);
    REG_SET_VAL(hKey, "DisplayName", 0, REG_SZ, (PBYTE) (PCSTR)m_sDriverName, 10);

    RegCloseKey (hKey);
    return TRUE;
}


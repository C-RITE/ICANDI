/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/*
 * File - DriverWDM.cpp
 *
 * Implementation of WDREG's actions for Windows 2000/XP/2003/Vista WDM drivers.
 */

#include <malloc.h>
#include "common_install.h"
#include "difx_install.h"
#include "Driver.h"
#include <cguid.h>

#include "ui.h"

#include <newdev.h>

#ifndef arraysize
        #define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif

static const char *get_platform()
{
    SYSTEM_INFO sysInfo;
    static char *nt_platform = NULL;

    if (nt_platform)
        return nt_platform;

    GetSystemInfo(&sysInfo); 
    switch (sysInfo.wProcessorArchitecture)
    {
        case 0:
            nt_platform = "ntx86";
            break; 
        case 6:
            nt_platform = "ia64";
            break; 
        case 9:
        case 10:
            nt_platform = "ntamd64";
            break;
        default:
            nt_platform = "unknown";
            break;
    }

    return nt_platform;
}

CDriverWDM::CDriverWDM(const char *sInfName, BOOL difxapi,
    BOOL dont_create_virtual) :
    m_sInfName(sInfName ? sInfName : ""),
    m_difxapi(difxapi),
    m_dont_create_virtual(dont_create_virtual)
{
}

CDriverWDM::~CDriverWDM()
{
    unload_functions();
}

BOOL CDriverWDM::Init()
{
    BOOL rc;

    if (!CDriver::Init())
        return FALSE;

    /* Try a privileged operation. It will fail if we run from a non-admin
     * account */
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM)
    {
        DisplayError("Cannot open service control manager.\n"
            "Make sure you are running with Administrator privileges\n");
        return FALSE;
    }

    CloseServiceHandle(hSCM);

    /* We have the sufficient privileges, proceed with loading functions */
    if (m_difxapi)
        rc = load_functions(LOAD_FUNCTIONS_NT);
    else
        rc = load_functions(LOAD_FUNCTIONS_NT_COMPAT);

    if (!rc)
    {
        DisplayError("Cannot load dynamic functions\n%s",
            (LPCSTR)InstallLastErrorString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverWDM::InstallDevice(const char *hwid)
{
    BOOL rc, devnode_exists, devnode_found;
    BOOL is_pseudo_device = !CDriver::is_physical_device(hwid);

    if (is_pseudo_device)
    {
        if (!WaitForUserCloseWdApps(hwid))
        {
            /* Not all WinDriver handles were closed - old driver is loaded */
            g_fRebootRequired = TRUE;
            return install_inf_section(m_sInfName);
        }

        /* Create the fake device if one does not exist */
        if (!m_dont_create_virtual &&
            !ExistsDeviceInfo(hwid, FALSE, &devnode_exists, &devnode_found, FALSE))
        {
            LogMessage("Trying to create WinDriver fake device\n");
            if (!InstallDeviceInfo(hwid, m_classname, &m_guid))
            {
                DisplayError("Failed to install WinDriver's kernel module\n%s",
                    (LPCSTR)InstallLastErrorString());
                return FALSE;
            }
        }
    }

    DisplayMessage("Installing a %s driver package\n",
        m_signed ? "signed" : "non-signed");

    if (m_difxapi)
    {
        if (!m_preinstall && !is_pseudo_device)
        {
            m_preinstall = !ExistsDeviceInfo(hwid, FALSE, &devnode_exists,
                &devnode_found, TRUE);

            DisplayMessage("Device node (hwid:%s): %s and is %sconfigured. %s.\n",
                hwid, devnode_found ? "exists" : "does not exist",
                devnode_exists ? "" : "not ",
                m_preinstall ? "Pre-installing" : "Installing");
        }

        if (m_preinstall)
            rc = difx_preinstall_inf(m_sInfName, m_signed);
        else
            rc = difx_install_inf(m_sInfName, m_signed);
    }
    else
        rc = install_inf(hwid, m_sInfName);
    
    if (!rc)
    {
        DisplayError("Failed to install the INF file (%s)\n%s",
            (LPCSTR)m_sInfName, (LPCSTR)InstallLastErrorString());
        if (is_pseudo_device)
            UnInstallDeviceInfo(hwid); /* Delete the WinDriver fake device */
    }
    else
        DisplayMessage("%s", (LPCSTR)InstallLastErrorString());

    return rc;
}

BOOL CDriverWDM::UninstallDevice(const char *hwid)
{
    BOOL rc, driver_in_store, devnode_exists, devnode_found;
    BOOL is_pseudo_device = !CDriver::is_physical_device(hwid);

    /* If WinDriver: closed all WD apps */
    if (is_pseudo_device && !WaitForUserCloseWdApps(hwid))
        return FALSE;

    driver_in_store = FALSE;
    if (m_difxapi)
        rc = difx_uninstall_inf(m_sInfName, &driver_in_store);

    if (!m_difxapi || !driver_in_store || ExistsDeviceInfo(hwid, TRUE,
        &devnode_exists, &devnode_found, FALSE))
    {
        rc = uninstall_inf(hwid, m_sInfName);
    }

    if (!rc)
    {
        DisplayError("Failed to uninstall the INF file (%s)\n%s",
            (LPCSTR)m_sInfName, (LPCSTR)InstallLastErrorString());
    }
    else
    {
        DisplayMessage("%s", (LPCSTR)InstallLastErrorString());

        if (is_pseudo_device)
            UnInstallDeviceInfo(hwid); /* delete the WinDriver fake device */
    }

    return rc;
}

BOOL CDriverWDM::EnableDevice(const char *hwid)
{
    if (!ChangeStateDeviceInfo(hwid, DICS_ENABLE))
    {
        DisplayError("Failed to enable device (%s)\n%s", hwid,
            (LPCSTR)InstallLastErrorString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverWDM::DisableDevice(const char *hwid)
{
    /* If WinDriver: close all WD apps */
    if (!CDriver::is_physical_device(hwid) && !WaitForUserCloseWdApps(hwid))
        return FALSE;

    if (!ChangeStateDeviceInfo(hwid, DICS_DISABLE))
    {
        DisplayError("Failed to disable device (%s)\n%s", hwid,
            (LPCSTR)InstallLastErrorString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverWDM::Uninstall()
{
    return ParseINF((INF_DEVICE_CALLBACK)&CDriverWDM::UninstallDevice);
}

BOOL CDriverWDM::Install()
{
    m_preinstall = FALSE;
    return ParseINF((INF_DEVICE_CALLBACK)&CDriverWDM::InstallDevice);
}

BOOL CDriverWDM::Preinstall()
{
    m_preinstall = TRUE;
    return ParseINF((INF_DEVICE_CALLBACK)&CDriverWDM::InstallDevice);
}

BOOL CDriverWDM::Enable()
{
    return ParseINF((INF_DEVICE_CALLBACK)&CDriverWDM::EnableDevice);
}

BOOL CDriverWDM::Disable()
{
    return ParseINF((INF_DEVICE_CALLBACK)&CDriverWDM::DisableDevice);
}

static BOOL inf_get_field(PINFCONTEXT ic, DWORD index, CCString *psField)
{
    DWORD need;

    if (!__SetupGetStringField(ic, index, NULL, 0, &need))
    {
        DisplayError("Failed to retrieve size of INF field: %s\n",
            (LPCSTR)LastErrString());
        return FALSE;
    }

    char *szField = (char *)malloc(need);
    if (!szField)
    {
        DisplayError("Failed to allocate %d bytes for INF field\n", need);
        return FALSE;
    }
    if (!__SetupGetStringField(ic, index, szField, need, NULL))
    {
        DisplayError("Failed to retrieve value of INF field: %s\n",
            (LPCSTR)LastErrString());
        free(szField);
        return FALSE;
    }

    *psField = (CCString)szField; 
    free(szField); 
    return TRUE;
}

static BOOL inf_get_model_section(PINFCONTEXT ic, CCString *sModelSection)
{
    CCString sModel, sPlatform;
    int i, field_count;

    /* Get Model field */
    if (!inf_get_field(ic, 1, &sModel))
    {
        DisplayError("Failed retrieving model field %s\n", 
            (LPCSTR)LastErrString());
        return FALSE;
    }

    *sModelSection = sModel;

    /* Get Platform field.
     * Iterate through all platforms supported by this INF and look for one
     * that matches the current system */
    field_count = __SetupGetFieldCount(ic);
    for (i = 2; i <= field_count; i ++)
    {
        if (!inf_get_field(ic, i, &sPlatform))
        {
            DisplayError("Failed retrieving platform field %d for model %s: "
                "%s\n", i, (LPCSTR)sModel, (LPCSTR)LastErrString());
            continue;
        }

        sPlatform.tolower();
        if (sPlatform == get_platform())
        {
            /* Match found - decorate with platform extension */
            *sModelSection += "." + sPlatform;
            return TRUE;
        }
    }

    /* No match is ok for x86 platform, the ModelSection remains undecorated */
    if ((CCString)get_platform() == (CCString)"ntx86")
        return TRUE;

    return FALSE;
}

BOOL CDriverWDM::ParseINF(INF_DEVICE_CALLBACK pfDevCB)
{
    INFCONTEXT mfg, mod, version;
    CCString sModelSection, sHWID; 
    UINT uiLineNo;

    HINF hInf = __SetupOpenInfFile(m_sInfName, NULL, INF_STYLE_WIN4, &uiLineNo);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DisplayError("Failed opening INF file %s line %d: %s\n", 
            (LPCSTR)m_sInfName, uiLineNo, (LPCSTR)LastErrString());
        return FALSE;
    }

    TCHAR classname[64];
    if (!__SetupDiGetINFClass(m_sInfName, &m_guid, classname,
        arraysize(classname), NULL))
    {
        DisplayError("Failed getting device class from INF file %s: %s\n",
            (LPCSTR)m_sInfName, (LPCSTR)LastErrString());
        return FALSE;
    }

    m_classname = classname;
    DWORD junk;
    if (m_guid == GUID_NULL && !__SetupDiClassGuidsFromName(classname, &m_guid,
        1, &junk))
    {
        DisplayError("Failed getting device class GUID from class name %s: "
            "%s\n", classname, (LPCSTR)LastErrString());
        return FALSE;
    }

    if (!__SetupFindFirstLine(hInf, "Version", "CatalogFile", &version))
        m_signed = FALSE;
    else
        m_signed = TRUE;

    if (!__SetupFindFirstLine(hInf, "Manufacturer", NULL, &mfg))
    {
        DisplayError("Failed locating Manufacturer section in INF file %s: "
            "%s\n", (LPCSTR)m_sInfName, (LPCSTR)LastErrString());
        __SetupCloseInfFile(hInf);
        return FALSE;
    }

    do { /* For each manufacturer */

        /*
         * The manufacturer section is in the following format
         * [Manufacturer]
         * %ManufacturerName% = ModelSection,ntamd64,ntia64
         *
         * The Models section for 64bit platforms must be decorated with
         * a platform extension:
         * [ModelSection.ntamd64]
         * [ModelSection.ntia64]
         *
         * The Models section for x86 platform can be without the platform
         * extension:
         * [ModelSection]
         *
         * For more details refer MSDN article:
         * "Creating INF Files for Multiple Platforms and Operating Systems"
         */

        /* get ModelSection (with or without platform extension) */
        if (!inf_get_model_section(&mfg, &sModelSection))
        {
            DisplayError("Failed retrieving manufacturer field from INF file "
                "%s: %s\n", (LPCSTR)m_sInfName, (LPCSTR)LastErrString());
            continue;
        }

        if (!__SetupFindFirstLine(hInf, sModelSection, NULL, &mod))
        {
            DisplayError("Failed retrieving manufacturer %s section from INF "
                "file %s: %s\n", (LPCSTR)sModelSection, (LPCSTR)m_sInfName,
                (LPCSTR)LastErrString());
            __SetupCloseInfFile(hInf);
            return FALSE;
        }
        do { /* For each device */
            if (!inf_get_field(&mod, 2, &sHWID))
            {
                DisplayError("Failed retrieving hardware ID field for "
                    "manufacturer %s: %s\n", (LPCSTR)sModelSection,
                    (LPCSTR)LastErrString());
                continue;
            }
            BOOL rc = (this->*pfDevCB)(sHWID);
            if (!rc)
            {
                __SetupCloseInfFile(hInf);
                return FALSE;
            }

        } while (__SetupFindNextLine(&mod, &mod));

    } while (__SetupFindNextLine(&mfg, &mfg));   

    __SetupCloseInfFile(hInf);
    
    return TRUE;
}


/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - DriverWDM16.cpp
//
// Implementation of WDREG's actions for Windows98/Me WDM drivers (16-bit).
////////////////////////////////////////////////////////////////

#include "common16.h"
#include "ui.h"
#include "Driver.h"

#ifndef arraysize
        #define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif

#define _T(str) str

CDriverWDM::CDriverWDM(const char *sInfName, BOOL difxapi, 
    BOOL dont_create_virtual) :
    m_sInfName(sInfName ? sInfName : ""), 
    m_difxapi(difxapi),
    m_dont_create_virtual(dont_create_virtual)
{
}

CDriverWDM::~CDriverWDM()
{
}

BOOL CDriverWDM::Init()
{
    return TRUE;
}

BOOL CDriverWDM::InstallDevice(const char *hid)
{
    if (CDriver::is_physical_device(hid))
    {
        DisplayError("WDREG16 does not support driver installation other than " 
            WD_PROD_NAME ". To install other drivers use Control Panel -> Add "
            "New Hardware wizard, "
            "or copy the files to system / INF directories and reboot\n");
        return FALSE;
    }

    if (!install_inf(hid, m_sInfName, m_classname, &m_atom))
    {
        DisplayError("Failed to install the INF file (%s)\n%s", (PCSTR)m_sInfName, (PCSTR)InstallLastErrorString());
        return FALSE;
    }

    g_fRebootRequired = TRUE;
    return TRUE;
}

BOOL CDriverWDM::UninstallDevice(const char *hid)
{
    if (!uninstall_inf(hid))
    {
        DisplayError("Failed to uninstall the device (%s)\n%s", hid, (PCSTR)InstallLastErrorString());
        return FALSE;
    }

    return TRUE;
}

BOOL CDriverWDM::EnableDevice(const char *hid)
{
    if (!ChangeStateDeviceInfo(hid, DICS_ENABLE))
    {
        DisplayError("Failed to enable device (%s)\n%s", hid, (PCSTR)InstallLastErrorString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverWDM::DisableDevice(const char *hid)
{
    if (!ChangeStateDeviceInfo(hid, DICS_DISABLE))
    {
        DisplayError("Failed to disable device (%s)\n%s", hid, (PCSTR)InstallLastErrorString());
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverWDM::Uninstall()
{
    return ParseINF(UninstallDevice);
}

BOOL CDriverWDM::Install()
{
    return ParseINF(InstallDevice);
}

BOOL CDriverWDM::Preinstall()
{
    return FALSE;
}

BOOL CDriverWDM::Enable()
{
    return ParseINF(EnableDevice);
}
BOOL CDriverWDM::Disable()
{
    return ParseINF(DisableDevice);
}

static BOOL inf_get_field(HINF hInf, HINFLINE hLine, DWORD index, CCString *psField)
{
    int need;
    UINT rc;

    char buffer[256];
    rc = IpGetStringField(hInf, hLine, (int)index, buffer, arraysize(buffer), &need);
    if (rc != 0)
    {
        DisplayError("Failed to retrieve value of INF field: %s\n", (PCSTR)LastErrString());
        return FALSE;
    }

    if (buffer[0] == '%')
    {                                           // possible string reference
        CCString stringname = buffer;
        stringname = stringname.Mid(1, stringname.Length() - 2);
        HINFLINE hString;
        rc = IpFindFirstLine(hInf, "Strings", (PCSTR)stringname, &hString);
        if (rc != 0)
        {
            DisplayError("Cannot find string table in INF file (looking for %s): %s\n", 
                (PCSTR)stringname, (PCSTR)LastErrString());
            return FALSE;
        }

        rc = IpGetStringField(hInf, hString, 1, buffer, arraysize(buffer), &need);
        if (rc != 0)
        {
            DisplayError("Cannot retrieve INF field value from string table: %s\n", (PCSTR)LastErrString());
            return FALSE;
        }
    }                                           // possible string reference

    *psField = (CCString)buffer; 
    return TRUE;
}       

BOOL CDriverWDM::ParseINF(INF_DEVICE_CALLBACK pfDevCB)
{
    HINFLINE mfg, mod;
    CCString sManufacturer, sHWID;
    HINF hInf;

    UINT rc = IpOpen((PCSTR)m_sInfName, &hInf);
    if (rc != 0)
    {
        DisplayError("Failed opening INF file %s: %s\n", 
            (PCSTR)m_sInfName, (PCSTR)LastErrString());
        goto Error;
    }

    // Create a global atom to hold the filename.
    m_atom = GlobalAddAtom((PCSTR)m_sInfName);

    char classname[256];
    rc = DiGetINFClass((LPSTR)(PCSTR) m_sInfName, INFTYPE_TEXT, classname, sizeof(classname));
    if (rc != 0)
    {
        DisplayError("Failed getting device class from INF file %s: %s\n",
            (PCSTR)m_sInfName, (PCSTR)LastErrString());
        goto Error;
    }
    m_classname = classname;

    rc = IpFindFirstLine(hInf, _T("Manufacturer"), NULL, &mfg);
    if (rc != 0)
    {
        DisplayError("Failed locating Manufacturer section in INF file %s: %s\n", 
            (PCSTR)m_sInfName, (PCSTR)LastErrString());
        IpClose(hInf);
        return FALSE;    
    }

    do { // for each manufacturer
        if (!inf_get_field(hInf, mfg, 1, &sManufacturer))
        {
            DisplayError("Failed retrieving manufacturer field from INF file %s: %s\n", 
                (PCSTR)m_sInfName, (PCSTR)LastErrString()); 
            continue;
        }
        rc = IpFindFirstLine(hInf, (PCSTR)sManufacturer, NULL, &mod);
        if (rc != 0)
        {
            DisplayError("Failed retrieving manufacturer %s section from INF file %s: %s\n", 
                (PCSTR)sManufacturer, (PCSTR)m_sInfName, (PCSTR)LastErrString());
            IpClose(hInf);
            return FALSE;
        }
        do { // for each device
            if (!inf_get_field(hInf, mod, 2, &sHWID))
            {
                DisplayError("Failed retrieving hardware ID field for manufacturer %s: %s\n", 
                    (PCSTR)sManufacturer, (PCSTR)LastErrString());
                continue;
            }
            BOOL rc = (this->*pfDevCB)(sHWID);
            if (!rc)
            {
                IpClose(hInf);
                goto Error;
            }

        } while (IpFindNextLine(hInf, &mod) == 0);

    } while (IpFindNextLine(hInf, &mfg) == 0);

    IpClose(hInf);
    return TRUE;
Error:
    return FALSE;
}


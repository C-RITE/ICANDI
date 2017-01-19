/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

//////////////////////////////////////////////////////////////////////
// File - common16.cpp
//
// Installation library (16-bit).
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <tchar.h> // Make all functions UNICODE safe.
#include <windows.h>
#include <string.h>
#include "common16.h"

BOOL FindDeviceInfo(const char *hid, LPDEVICE_INFO *ppDeviceList, LPDEVICE_INFO *ppDeviceFound = NULL);

CCString g_sErr(""); // contains the last error encounterred
BOOL g_fRebootRequired = FALSE;

CCString LastErrString()
{
    return "";
}

CCString InstallLastErrorString()
{
    return g_sErr;
}

static BOOL does_device_exist(const char *hid);
static HINSTANCE new_dev_lib = NULL;
static HINSTANCE setup_lib = NULL;
static int ref_count;

BOOL ExistsDeviceInfo(const char *hid)
{
    LPDEVICE_INFO lpdi;

    BOOL Found = FindDeviceInfo(hid, &lpdi);
    DiDestroyDeviceInfoList(lpdi);
    return Found;
}

BOOL uninstall_inf(const char *hid)
{
    BOOL fReturn = FALSE;
    LPDEVICE_INFO lpdi, lpdiFound;
    RETERR rc;

    BOOL Found = FindDeviceInfo(hid, &lpdi, &lpdiFound);
    if (!Found)
    {
        g_sErr.Format("%sError finding the device %s\n", (PCSTR)g_sErr, hid); 
        goto Exit;
    }

    rc = DiRemoveDevice(lpdiFound);
    if (rc != OK)
    {
        g_sErr.Format("%sError removing device (error code: %d)\n", (PCSTR)g_sErr, rc);
        goto Exit;
    }

    if (lpdi->Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))
        // reboot required
        g_fRebootRequired = TRUE;
    
    fReturn = TRUE;
Exit:
    DiDestroyDeviceInfoList(lpdi);
    return fReturn;
}

BOOL ChangeStateDeviceInfo(const char *hid, DWORD dwNewState)
{
    BOOL fReturn = FALSE;
    LPDEVICE_INFO lpdi, lpdiFound;
    RETERR rc;

    BOOL Found = FindDeviceInfo(hid, &lpdi, &lpdiFound);
    if (!Found)
    {
        g_sErr.Format("%sError finding the device %s\n", (PCSTR)g_sErr, hid); 
        goto Exit;
    }

    rc = DiChangeState(lpdiFound, dwNewState, DICS_FLAG_GLOBAL, NULL);
    if (rc != OK)
    {
        g_sErr.Format("%sError changing the device status for device %s (error code: %d)\n", 
            (PCSTR)g_sErr, hid, rc);
        goto Exit;
    }

    if (lpdi->Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))
        // reboot required
        g_fRebootRequired = TRUE;
    
    fReturn = TRUE;
Exit:
    DiDestroyDeviceInfoList(lpdi);
    return fReturn;
}

static BOOL FindDeviceInfo(const char *hid, LPDEVICE_INFO *lplpdiList, LPDEVICE_INFO *lplpdiFound) 
{
    BOOL Found, fErrorOccurred = FALSE;
    LPDEVICE_INFO lpdi;
    PSTR p,buffer = NULL;
    RETERR rc;
    int iFlags = DIGCF_ALLCLASSES & (~DIGCF_PRESENT); 
    int iLen;
    char *pcComma;

    *lplpdiList = NULL;
    if (lplpdiFound)
        *lplpdiFound = NULL;

    // Create a Device Information Set with all present devices.
    rc = DiGetClassDevs(&lpdi, 
        0, 0,
        iFlags);
    if (rc != OK)
    {
        g_sErr.Format("%sError getting list of all the devices (error code: %d)\n", (PCSTR)g_sErr, rc);
        return FALSE;
    }

    // return the whole list to the caller (for him to delete it)
    *lplpdiList = lpdi;

    Found = FALSE;

    while (lpdi)
    {
        char  buffer[MAX_DEVNODE_ID_LEN+1] = "";
        HKEY  hDevKey, hDrvKey;
        DWORD dwSize = sizeof( buffer );

        // We need to get a key handle from the device node.
        // If we get a valid handle we will look for the HardwareID.

        rc = DiOpenDevRegKey(lpdi, &hDrvKey, DIREG_DRV );
        if ( rc != OK )
        {
            lpdi = lpdi->lpNextDi;                      
            continue;
        }

        rc = DiOpenDevRegKey(lpdi, &hDevKey, DIREG_DEV );
        if ( rc != OK )
        {
            SURegCloseKey( hDrvKey );
            continue;
        }

        //  Get Data value from szHardwareID subkey

        SURegQueryValueEx(hDevKey, 
            "HardwareID", 
            NULL, 
            NULL, 
            (LPSTR)buffer, 
            &dwSize );

        SURegCloseKey( hDevKey );                

        // Compare each entry in the buffer multi-sz list with our HardwareID.
        // WIN 98/Me: multi string is separated by commas
        for (p=buffer; *p&&(p<&buffer[dwSize]); p+=iLen+sizeof(char))
        {
            pcComma = strchr(p, ',');
            if (pcComma)
                iLen = pcComma - p;
            else
                iLen = lstrlen(p);

            if (!_strnicmp(hid, p, iLen))
            {
                Found = TRUE;
                break;
            }
        }

        SURegCloseKey( hDrvKey );

        if (Found) 
        {
            // return the found dev to the caller
            *lplpdiFound = lpdi;
            break;
        }

        lpdi = lpdi->lpNextDi;
    }                                   

    return Found;
}

BOOL install_inf(const char *hid, const char *file_name, const char *classname, ATOM *atom)
{
    BOOL fReturn = FALSE;
    RETERR rc = OK;
    LPDEVICE_INFO lpdList, dip;

    if (!FindDeviceInfo(hid, &lpdList, &dip))
    {
        /* in case the device doesn't exist */
        rc = DiCreateDeviceInfo(&dip, classname, 0, HKEY_LOCAL_MACHINE, NULL, classname, NULL);
        if (rc != OK)
        {
            g_sErr.Format("%sError creating empty device info list (error code: %d)\n", (PCSTR)g_sErr, rc);
            goto Exit;
        }
    }

    LPDRIVER_NODE dnp;
    dip->atDriverPath = *atom;
    dip->Flags |= DI_ENUMSINGLEINF | DI_QUIETINSTALL;

    // Create a list of drivers in this INF file

    rc = DiBuildClassDrvList(dip);
    if (rc != OK)
    {
        g_sErr.Format("%sError building driver list (error code: %d)\n", 
            (PCSTR)g_sErr, rc);
        goto Exit;
    }
    
    // Select the driver for the specified hardware id

    dnp = dip->lpClassDrvList;
    while (dnp)
    {                                   // select driver
        if (!lstrcmp(dnp->lpszHardwareID, (LPCSTR)hid))
            break;
        dnp = dnp->lpNextDN;
    }

    if (!dnp)
        dnp = dip->lpClassDrvList;
    dip->lpSelectedDriver = dnp;

    // Change the LDID_SRCPATH path to the directory containing the INF. This is a hack
    // that makes relative pathnames in the INF work correctly.

    char drive[_MAX_DRIVE], dir[_MAX_DIR], infdir[_MAX_PATH];
    _splitpath(file_name, drive, dir, NULL, NULL);
    _makepath(infdir, drive, dir, NULL, NULL);
    CtlSetLddPath(LDID_SRCPATH, infdir);
   
    // Install the device and the driver

    dip->Flags |= DI_QUIETINSTALL;
    rc = DiCallClassInstaller(DIF_INSTALLDEVICE, dip);
    if (rc != OK)
    {
        g_sErr.Format("%sError calling class installer register (error code: %d)\n", (PCSTR)g_sErr, rc);
        goto Exit;
    }

    fReturn = TRUE;
Exit:
    if (dip)
        DiDestroyDeviceInfoList(dip);
    return fReturn;
}


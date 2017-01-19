/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - DriverNT4.cpp
//
// Implementation of WDREG's actions for WindowsNT4 drivers.
////////////////////////////////////////////////////////////////

#include "common_install.h"
#include "Driver.h"
#include "ui.h"

CDriverNT4::CDriverNT4(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel) :
    CDriverNonWDM(sDriverName, sDriverFile, dwStartLevel)
{
    m_sDriverPath.Format("\\SystemRoot\\System32\\drivers\\%s.SYS", sDriverFile);
}

CDriverNT4::~CDriverNT4()
{
    if (m_hSCM)
        CloseServiceHandle((SC_HANDLE)m_hSCM);
}

BOOL CDriverNT4::Init()
{
    if (!CDriver::Init())
        return FALSE;
        
    m_hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!m_hSCM)
    {
        DisplayError("Cannot open service control manager.\n"
            "Make sure you are running with Administrator privileges\n");
        return FALSE;
    }
    return TRUE;
}

BOOL CDriverNT4::Create()
{
    SC_HANDLE  sr;

    // Check if the service is already installed
    sr = OpenService((SC_HANDLE)m_hSCM ,m_sDriverName, SERVICE_ALL_ACCESS);
    if (sr)
    {
        CloseServiceHandle(sr);                     
        return TRUE;
    }
    else if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
    {
        DisplayError("Error trying to open service %s (0x%lx): %s\n", (LPCSTR)m_sDriverName, 
            m_hSCM, (LPCSTR)LastErrString());
        return FALSE;
    }

    sr = CreateService(m_hSCM, m_sDriverName, m_sDriverName, SERVICE_ALL_ACCESS,
         SERVICE_KERNEL_DRIVER, m_dwStartLevel, SERVICE_ERROR_NORMAL, 
         m_sDriverPath, NULL, NULL, NULL, NULL, NULL);  
    if (!sr)
    {
        DisplayError("Failed creating service %s: %s\n", (LPCSTR)m_sDriverName, (LPCSTR)LastErrString());
        CloseServiceHandle(sr);
        return FALSE;
    }

    CloseServiceHandle(sr);
    return TRUE;
}

BOOL CDriverNT4::Delete()
{
    SC_HANDLE sr;
    BOOL fReturn;
    SERVICE_STATUS service_status;
    DWORD count = 0; 

    sr = OpenService(m_hSCM, m_sDriverName, SERVICE_ALL_ACCESS);
    if (!sr)
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            DisplayMessage("Service %s already deleted\n", (LPCSTR)m_sDriverName);
            return TRUE;
        }
        else
        {
            DisplayError("Error trying to open service %s for delete: %s\n",
                (LPCSTR)m_sDriverName, (LPCSTR)LastErrString());
            return FALSE;
        }
    }

    if (!DeleteService(sr))
    {
        switch(GetLastError())
        {
        // The specified handle was not opened with DELETE access.
        case ERROR_ACCESS_DENIED:
            DisplayError("Cannot remove the service - access denied\n");
            break;
        // The specified service has already been marked for deletion
        case ERROR_SERVICE_MARKED_FOR_DELETE:
            DisplayMessage("The removal will take effect after the system reboots.\n");
            break;
        default:
            DisplayError("Cannot delete the service: %s\n", (LPCSTR)LastErrString());
        }
        fReturn = FALSE;
        goto Exit;
    }
    
    // Requests the service to update immediately its current status
    // information to the service control manager
    if (!ControlService(sr, SERVICE_CONTROL_INTERROGATE, &service_status))
    {
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
        {
            DisplayError("ControlService failed: %s\n", (LPCSTR)LastErrString());
            goto Exit;
        }
    }
    
    // Loop until the service will be deleted
    do {
        CloseServiceHandle(sr);
        count++;
        if (count==120)
        {
            DisplayError("The system is busy. Please reboot the machine\n"
                "and try again.\n");
            sr = NULL;
            fReturn = FALSE;
            goto Exit;
        }
        Sleep(1000);
        sr = OpenService(m_hSCM, m_sDriverName, SERVICE_ALL_ACCESS);
    } while (sr);

    fReturn = TRUE;
Exit:
    if (sr)
        CloseServiceHandle(sr);
    return fReturn;
}

BOOL CDriverNT4::Start()
{
    SC_HANDLE sr = OpenService(m_hSCM ,m_sDriverName, SERVICE_ALL_ACCESS);
    if (!sr)
    {
        DisplayError("Error opening the service %s: %s\n", (LPCSTR)m_sDriverName, (LPCSTR)LastErrString());
        return FALSE;
    }   

    if (!StartService(sr, 0, NULL))
    {
        DisplayError("Error starting the service %s: %s\n", (LPCSTR)m_sDriverName, (LPCSTR)LastErrString());
        CloseServiceHandle(sr);         
        return FALSE;
    }
    CloseServiceHandle(sr);             
    return TRUE;
}

BOOL CDriverNT4::Stop()
{
    BOOL fReturn;
    if (m_sDriverName == WD_DISPLAY_NAME && !WaitForUserCloseWdApps(WD_INF_FAKE_HWID))
        return FALSE;

    SC_HANDLE sr = OpenService(m_hSCM ,m_sDriverName ,SERVICE_ALL_ACCESS);
    if (!sr) 
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            // If the service does not exist we return TRUE
            DisplayMessage("Nothing to stop: service %s does not exist\n", (LPCSTR)m_sDriverName);
            fReturn = TRUE;
            goto Exit;
        }
        else
        {
            DisplayError("Cannot open service: %s\n", (LPCSTR)LastErrString()); 
            fReturn = FALSE;
            goto Exit;
        }
    }

    SERVICE_STATUS serviceStatus;
    if (!ControlService(sr ,SERVICE_CONTROL_STOP ,&serviceStatus))
    {
        if (GetLastError() == ERROR_SERVICE_NOT_ACTIVE)
        {
            DisplayMessage("Nothing to stop: service %s is not active\n", (LPCSTR)m_sDriverName);
            fReturn = TRUE;
        }
        else
        {
            DisplayError(" Sending stop request to service: %s\n", (LPCSTR)LastErrString());
            fReturn = FALSE;
        }
        goto Exit;
    }

    if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
    {
        DisplayError("Cannot stop service: %s\n", (LPCSTR)LastErrString());
        fReturn = FALSE;
        goto Exit;
    }

    fReturn = TRUE;
Exit:
    if (sr)
        CloseServiceHandle(sr);
    return fReturn;
}


/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - Driver.h
//
// A header file for WDREG's classes and actions.
////////////////////////////////////////////////////////////////

#ifndef _WDREG_H_
#define _WDREG_H_

#include <windows.h>
#include <string.h>
#include "utils.h"
#include "ui.h"

#if defined(WIN16)

typedef enum 
{
    OS_NOT_SET = 0,
    OS_CAN_NOT_DETECT,
    OS_WIN_95,   /* Windows 95 */
    OS_WIN_98,   /* Windows 98, ME */
    OS_WIN_NT_4, /* Windows NT 4.0 */
    OS_WIN_NT_5, /* Windows 2000, XP, Server 2003 */
    OS_WIN_NT_6, /* Windows Vista */
    OS_LINUX,
    OS_SOLARIS,
    OS_VXWORKS,
    OS_OS2,
    OS_WINCE_4,  /* Windows CE 4 and 5 */
    OS_WINCE_6   /* Windows CE 6 */
} OS_TYPE;

#define get_os_type() OS_WIN_98

#endif

#define WD_INF_FAKE_HWID "WINDRVR6" // fake device's hwid as defined in WD's inf file
#define WD_DISPLAY_NAME "windrvr6"

#if !defined(WIN32)
#define WD_PROD_NAME "WinDriver"
#endif

#define WD_SUCCESS_UNSUPPORTED  2

class CDriver
{
public:
    virtual BOOL Init() 
        {
            if (is_wow64())
            {
                DisplayError("Cannot run an x86 build of this utility on x64 platform.\n");
                return FALSE;
            }
            return TRUE;
        }
    virtual BOOL Install()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Preinstall()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Uninstall()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Enable()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Disable()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Create()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Delete()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Start()
        { return WD_SUCCESS_UNSUPPORTED; }
    virtual BOOL Stop()
        { return WD_SUCCESS_UNSUPPORTED; }
        //{ DisplayError("Unsupported action\n"); return FALSE; };
protected:
    static BOOL is_physical_device(const char *hwid)
    {
        /* if the device's enumerator is not PCI, USB or PCMCIA it is 
         * regarded as a virtual device */
        return (!strncmp(hwid, "PCI\\", sizeof("PCI\\") - 1) || 
                !strncmp(hwid, "PCMCIA\\", sizeof("PCMCIA\\") - 1) || 
            !strncmp(hwid, "USB\\", sizeof("USB\\") - 1));
    }
private:
    BOOL is_wow64(void)
    {
#if !defined(WIN16)
        if (getenv("ProgramW6432"))
            return TRUE;
#endif
        return FALSE;
    }
};

class CDriverNonWDM : public CDriver
{
public:
    CDriverNonWDM(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel) :
        m_sDriverName(sDriverName), m_sDriverFile(sDriverFile), m_dwStartLevel(dwStartLevel)
        {};

    virtual ~CDriverNonWDM() {};

    virtual BOOL Install() 
    {
#if !defined(WIN16)
        HANDLE hWD;
        if (m_sDriverName == WD_DISPLAY_NAME && 
            (hWD = WD_Open()) != INVALID_HANDLE_VALUE)
        {
            // driver update
            WD_Close(hWD); 
            Uninstall();
        }
#endif
        return (Create() && Start());
    };

    BOOL Uninstall()
    {
        return (Stop() && Delete());
    };

protected:
    CCString m_sDriverName;
    CCString m_sDriverFile;
    DWORD m_dwStartLevel;
    CCString m_sDriverPath;
#if defined(WIN16)
    HANDLE m_hSCM;
#else
    SC_HANDLE m_hSCM;
#endif
};

#if defined(WIN16)

class CDriver98 : public CDriverNonWDM
{
public:
    CDriver98(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel);
    ~CDriver98();
    virtual BOOL Create();
    virtual BOOL Delete();
    virtual BOOL Start();
    virtual BOOL Stop();

private:
    BOOL DeleteRegTree();
    BOOL CreateRegTree();
    BOOL SetRegTreeStartLevel(DWORD dwStartLevel);
};

#else

class CDriverVXD : public CDriverNonWDM
{
public:
    CDriverVXD(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel);
    ~CDriverVXD();
    virtual BOOL Init();
    virtual BOOL Create();
    virtual BOOL Delete();
    virtual BOOL Start();
    virtual BOOL Stop();

private:
    BOOL SetRegTreeStartLevel(DWORD dwStartLevel);
    BOOL CreateRegTree();
    BOOL DeleteRegTree();
};

class CDriverNT4 : public CDriverNonWDM
{
public:
    CDriverNT4(const char *sDriverName, const char *sDriverFile, const DWORD dwStartLevel);
    ~CDriverNT4();
    virtual BOOL Init();
    virtual BOOL Create();
    virtual BOOL Delete();
    virtual BOOL Start();
    virtual BOOL Stop();

private:
    // Service Control Manager members
    BOOL CreateDriverService();
    BOOL DeleteDriverService();
    BOOL StartDriverService();
    BOOL StopDriverService();
};
#endif // defined(WIN16)

class CDriverWDM;

typedef BOOL (CDriverWDM::*INF_DEVICE_CALLBACK) (const char *szHWID);

// supports Win2000/XP. Win98/Me have differnet binary compiled to 16-bit.
class CDriverWDM : public CDriver
{
public:
    CDriverWDM(const char *sInfName, BOOL difxapi = FALSE, BOOL dont_create_virtual = FALSE);
    ~CDriverWDM();
    virtual BOOL Init();
    virtual BOOL Install();
    virtual BOOL Preinstall();
    virtual BOOL Uninstall();
    virtual BOOL Enable();
    virtual BOOL Disable();

private:
    BOOL ParseINF(INF_DEVICE_CALLBACK pfDevCB);
    BOOL InstallDevice(const char *szHWID);
    BOOL UninstallDevice(const char *szHWID);
    BOOL EnableDevice(const char *szHWID);
    BOOL DisableDevice(const char *szHWID);

    CCString m_sInfName;
    CCString m_classname;
    BOOL m_difxapi;
    BOOL m_dont_create_virtual;
    BOOL m_signed;
    BOOL m_preinstall;
#if defined(WIN16) 
    ATOM m_atom; 
#else
    GUID m_guid;  
#endif // defined(WIN16) 
};

#endif _WDREG_H_


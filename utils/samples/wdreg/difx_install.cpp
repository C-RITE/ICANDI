/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

#include <windows.h>
#include <setupapi.h> // setup api error codes
#include "difx_install.h"
#include "common_install.h"
#include "cstring.h"
#include "utils.h"

extern CCString g_sErr;
extern BOOL g_fRebootRequired;
static DWORD last_error;
static DIFXAPI_LOG last_event;

static const char *event_code2str(DIFXAPI_LOG event)
{
    switch (event)
    {
    case DIFXAPI_SUCCESS:
        return "SUCCESS";
    case DIFXAPI_INFO:
        return "INFO";
    case DIFXAPI_WARNING:
        return "WARNING";
    case DIFXAPI_ERROR:
        return "ERROR";
    }
    return "UNKNOWN";
}

static void __cdecl log_call_back(DIFXAPI_LOG event, DWORD error,
    const char *event_desc, void *context)
{
    if (!error)
    {
        g_sErr.Format("%sLOG ok: %lx, %s\n", (LPCSTR)g_sErr, event,
            event_desc);
    }
    else
    {
        g_sErr.Format("%sLOG %s(%lx): error %lx, %s\n", (LPCSTR)g_sErr,
            event_code2str(event), event, error, event_desc);
        last_error = error;
        last_event = event;
    }
}

static BOOL difx_install_preinstall_inf(const char *file_name_with_path,
    BOOL signed_install, BOOL preinstall)
{
    DWORD status;
    DWORD flags;
    const char *err_msg = "";
 
    flags =  DRIVER_PACKAGE_FORCE |
         (preinstall ? 0 : DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT) |
         (signed_install ? 0 : DRIVER_PACKAGE_LEGACY_MODE);

    last_error = 0;
    last_event = DIFXAPI_SUCCESS;

    __DIFXAPISetLogCallback(log_call_back, NULL);

    if (preinstall)
        status = __DriverPackagePreinstall(file_name_with_path, flags);
    else
    {
        status = __DriverPackageInstall(file_name_with_path, flags, NULL,
            &g_fRebootRequired);
    }

    if (status == ERROR_NO_MORE_ITEMS)
    {
        err_msg = "The specified driver is not a better match than the current"
            " driver for present devices.";
    }
    else if (status == TRUST_E_NOSIGNATURE)
        err_msg = "Driver package is not signed.";
    else if (status == ERROR_NO_SUCH_DEVINST)
    {
        err_msg = "The DeviceID's contained in the INF do no match any live "
            "devnode.";
    }
    else if (status == ERROR_ACCESS_DENIED)
        err_msg = "Make sure you are running with Administrator privileges.";
    else if (status == ERROR_DRIVER_NONNATIVE)
    {
        if (signed_install)
        {
            err_msg = "The driver does not match the platform or the catalog "
                "file is invalid/missing.";
        }
        else
            err_msg = "The driver does not match the platform.";
    }
    else if (status == ERROR_ALREADY_EXISTS) /* 0xb7 */
        err_msg = "The driver package is already pre-installed";

    if (status)
    {
        /* This is not necessarily an error */
        g_sErr.Format("%sStatus: %lx. %s\n", (LPCSTR)g_sErr, status, err_msg);
    }

    return last_event == DIFXAPI_ERROR ? FALSE : TRUE;
}

BOOL difx_install_inf(const char *file_name_with_path, BOOL signed_install)
{
    return difx_install_preinstall_inf(file_name_with_path, signed_install,
        FALSE);
}

BOOL difx_preinstall_inf(const char *file_name_with_path, BOOL signed_install)
{
    return difx_install_preinstall_inf(file_name_with_path, signed_install,
        TRUE);
}

BOOL difx_uninstall_inf(const char *file_name_with_path, BOOL *driver_in_store)
{
    DWORD status, n, flags = 0;

    status = __DriverPackageGetPath(file_name_with_path, NULL, &n);
    *driver_in_store = (status != ERROR_DRIVER_PACKAGE_NOT_IN_STORE);

    last_error = 0;
    last_event = DIFXAPI_SUCCESS;

    __DIFXAPISetLogCallback(log_call_back, NULL);

    if (*driver_in_store)
    {
        status = __DriverPackageUninstall(file_name_with_path, flags, NULL,
            &g_fRebootRequired);
    }

    return last_event == DIFXAPI_ERROR ? FALSE : TRUE;
}


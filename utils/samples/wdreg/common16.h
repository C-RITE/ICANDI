/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - common16.h 
//
// A header file for the Installation library (16-bit).
////////////////////////////////////////////////////////////////

#ifndef _COMMON16_H
#define _COMMON16_H

#include <windows.h>
#include "cstring.h"
#include "utils.h" 

#if defined(__cplusplus)
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#include <setupx.h>

#if defined(__cplusplus)
}                       /* End of extern "C" { */ 
#endif /* __cplusplus */ 

extern BOOL g_fRebootRequired;
CCString LastErrString();
CCString InstallLastErrorString();

BOOL ExistsDeviceInfo(const char *hid);
BOOL ChangeStateDeviceInfo(const char *hid, DWORD dwNewState);
BOOL uninstall_inf(const char *hid);
BOOL install_inf(const char *hid, const char *file_name, const char *classname, ATOM *atom);

#endif // _COMMON16_H


/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - ui.h
//
// A header file for the User Interface library.
////////////////////////////////////////////////////////////////

#ifndef _UTIL_H
#define _UTIL_H

#include <windows.h>
#include "cstring.h" 
#include "utils.h" 

// Display Functions

extern BOOL g_fSilent;
extern char *sLogFileName;

#if defined(_CONSOLE)
#define DisplayUsage DisplayMessage
#else
extern CCString g_sAppName;
void DisplayUsage(char *szFormat, ...);
#endif

void DisplayError(char *szFormat, ...);
void DisplayMessage(char *szFormat, ...);
BOOL DisplayRetryOrCancel(char *szFormat, ...);

BOOL LogMessage(char *msg);

BOOL WaitForUserCloseWdApps(const char *sDriverName);

#endif


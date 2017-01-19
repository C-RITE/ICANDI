/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - ui.cpp
//
// User Interface library.
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <conio.h>
#include <tchar.h> // Make all functions UNICODE safe.
#include <string.h>
#include "ui.h"

BOOL g_fSilent = FALSE;
char *sLogFileName = NULL;

char sBuf[4096];

BOOL LogMessage(char *msg)
{
    FILE *fLog;

    if (!sLogFileName)
        return TRUE;
    if (!(fLog = fopen(sLogFileName, "a+t")))
        return FALSE;

    fprintf(fLog, "%s", msg);
    fclose(fLog);
    return TRUE;
}

#ifdef _CONSOLE
BOOL DisplayRetryOrCancel(char *szFormat, ...)
{
    BOOL ret = FALSE;
    char ch;
    va_list marker;

    va_start( marker, szFormat );
    if (vsprintf( sBuf, szFormat, marker ))
    {
        strcat(sBuf, "\nPlease press 'R' to retry or 'C' to cancel...\n");
        if (!g_fSilent)
            printf("%s", sBuf);

        LogMessage(sBuf);

        while (!g_fSilent)
        {
            flushall();
            ch = getche();
            printf("\n");
            if (ch=='R' || ch=='r')
            {
                ret = TRUE;
                LogMessage("RETRY\n");
                break;
            }
            else if (ch=='C' || ch=='c')
            {
                ret = FALSE;
                LogMessage("CANCEL\n");
                break;
            }
        }
    }
    va_end( marker );
    return ret;
}

void DisplayError(char *szFormat, ...)
{
    va_list marker;
    int header_len;

    va_start( marker, szFormat );
    header_len = sprintf(sBuf, "Error: ");
    vsprintf(sBuf + header_len, szFormat, marker);
    if (!g_fSilent)
        fprintf(stderr, "%s", sBuf);
    LogMessage(sBuf);
    va_end( marker );
    return;
}

void DisplayMessage(char *szFormat, ...)
{
    va_list marker;

    va_start( marker, szFormat );
    vsprintf(sBuf, szFormat, marker);
    if (!g_fSilent)
        printf("%s", sBuf);
    LogMessage(sBuf);
    va_end( marker );
    return;
}

#else 

CCString g_sAppName;

void DisplayError(char *szFormat, ...)
{
    va_list marker;
    int header_len, len;

    va_start( marker, szFormat );
    header_len = sprintf(sBuf, "Error: ");
    len = vsprintf(sBuf + header_len, szFormat, marker);
    if (!g_fSilent && len)
        MessageBox(NULL, sBuf + header_len, (LPCSTR)(PCSTR)(g_sAppName+" Error"), MB_OK | MB_ICONERROR);
    LogMessage(sBuf);
    va_end( marker );
    return;
}

void DisplayMessage(char *szFormat, ...)
{
    va_list marker;

    va_start( marker, szFormat );
    vsprintf(sBuf, szFormat, marker);
    LogMessage(sBuf);
    va_end( marker );
    return;
}

void DisplayUsage(char *szFormat, ...)
{
    va_list marker;

    if (g_fSilent)
        return;
    va_start( marker, szFormat );
    if (vsprintf( sBuf, szFormat, marker ))
    {
        MessageBox(NULL, sBuf, (LPCSTR)(PCSTR)(g_sAppName+" Error"), 
            MB_OK | MB_ICONINFORMATION);
    }
    va_end( marker );
    return;
    
}

BOOL DisplayRetryOrCancel(char *szFormat, ...)
{
    DWORD ret;
    va_list marker;
    int header_len;

    va_start( marker, szFormat );
    header_len = vsprintf( sBuf, szFormat, marker );
    if (header_len && !g_fSilent)
        ret = MessageBox(NULL, sBuf, "WDREG Error", MB_RETRYCANCEL | MB_ICONINFORMATION);

    strcat(sBuf, "\nPlease press 'R' to retry or 'C' to cancel...\n");
    LogMessage(sBuf);

    va_end( marker );

    if (!g_fSilent && ret==IDRETRY)
    {
        LogMessage("RETRY\n");
        return TRUE;
    }
    else
    {
        LogMessage("CANCEL\n");
        return FALSE;
    }
}
#endif
                        
#if defined(WIN32)                        
BOOL WaitForUserCloseWdApps(const char *sDriverName)
{
    HANDLE hWD;
    WD_USAGE wd_usage;
    BOOL fRetry = TRUE, single;

    if (!WD_DriverName(sDriverName))
    {
        DisplayError("Cannot set driver name");
        return FALSE;
    }
    do // retry until canceled
    {
        BZERO(wd_usage);
        hWD = WD_Open();
        if (hWD != INVALID_HANDLE_VALUE)
        {
            WD_Usage(hWD, &wd_usage);
            WD_Close(hWD); // hWD must be closed before deleting the virtual device
        }

        // There should be no handles to WinDriver (beside the one we just opened)
        if (wd_usage.applications_num > 1)
        {
            single = wd_usage.applications_num==2;
            fRetry = DisplayRetryOrCancel("There %s currently %d open application%s using WinDriver.\n"
                "Please close all applications and press Retry.\n" 
                "To reload WinDriver, press Cancel and reboot.\n", single ? "is" : "are",
                wd_usage.applications_num - 1, single ? "" : "s");
        }
        else if (wd_usage.devices_num)
        {
            single = wd_usage.devices_num==1;
            fRetry = DisplayRetryOrCancel("There %s currently %d connected device%s using WinDriver.\n"
                "Please disconnect or uninstall all connected devices from the Device Manager\n"
                "and press Retry.\n"
                "To reload WinDriver, press Cancel and reboot.\n", single ? "is" : "are",
                wd_usage.devices_num, single ? "" : "s");
        }
        else
            break;
    } while (fRetry);

    return fRetry; // are all applications closed
}
#endif


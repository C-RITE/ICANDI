/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

//////////////////////////////////////////////////////////////////////
// File - wdreg.cpp
//
// The main file which parses the command line, validates the parameters
// and carries out WDREG's actions.
// For 16-bit: WIN16 should be defined
//////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#if defined(WIN32)   
#include <Setupapi.h>
#include "common_install.h"
#else
#include <stdlib.h>
#include <string.h>
#include "common16.h"
#include "wd_ver.h"
#endif

#include "Driver.h"
#include "ui.h"

#define EXIT_REBOOT 2
                
#if defined(WIN32) && !defined(_CONSOLE)
int main (int argc, char *argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int iCmdShow)
{
    char argv0[256];
    
    char *argv_all;
    char *argv[256];
    int ret_val;
    int argc = 0;
    
    BOOL fInArg = FALSE;
    BOOL fInQuote;
    int i,j;
    
    GetModuleFileName(hInstance, argv0, sizeof (argv0));
    g_sAppName = argv0;
    argv[argc++] = argv0;
    
    argv_all = (char *) malloc (strlen(szCmdLine)+1);
    
    j = 0;
    for (i=0; (char) szCmdLine[i]; i++)
    {
        char ch = (char) szCmdLine[i];
        if (fInArg)
        {
            if (fInQuote && ch=='\"' || !fInQuote && (ch==' ' || ch=='\t'))
            {
                fInArg = FALSE;
                argv_all[j++] = '\0';
            }
            else
            {
                argv_all[j++] = ch;
            }
        }
        else
        {
            if (ch!=' ' && ch!='\t')
            {
                fInArg = TRUE;
                fInQuote = ch=='\"';
                if (!fInQuote)
                    i--;
                argv[argc] = argv_all + j;
                argc++;
            }
        }
    }
    
    if (fInArg)
        argv_all[j++] = '\0';
    
    argv[argc] = NULL;
    
    ret_val = main(argc, argv);

    free (argv_all);

    return ret_val;
}
#endif // defined(WIN32) && !defined(_CONSOLE)

#if defined(WIN16)
#define SERVICE_BOOT_START 0
#define SERVICE_SYSTEM_START 1
#define SERVICE_AUTO_START 2
#define SERVICE_DEMAND_START 3
#define SERVICE_DISABLED 4
#endif

#define WDREG_VERSION_STR \
    "WDREG utility v" WD_VERSION_STR ". Build " __DATE__ " " __TIME__
    
void PrintUsage(char *exe)
{
    DisplayUsage(WDREG_VERSION_STR "\n"
    "Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com\n\n"
    "Command usage:\n"
#if defined(WIN32)
    "non-WDM Drivers: (KernelPlugin Win2000/XP/Server 2003/Vista; .SYS drivers on WinNT4;\n"
    "                  .VXD drivers on Win95/98/Me)\n"
#else
    "non-WDM Drivers: (KernelPlugin on Win98/Me)\n"
#endif
    "\n"
    "%s [Options ...] Action [Action ...]\n"
    "Options:\n"
    "  -startup <level>: Set startup level: boot, system, automatic,\n"
    "                    demand, disabled (default: automatic).\n"
    "  -name <name>:     Set then name of the driver (default: " WD_DISPLAY_NAME ").\n"
    "  -file <filename>: Set the file name of the driver.\n"
#if defined(WIN32)
    "  -vxd:             Load a .vxd driver(Win95/98/Me only)\n"
#endif
    "  -silent:          Suppress all messages.\n"
    "  -log <filename>:  Log all messages to filename.\n"
    "Actions:\n"
    "  create:    create a registry entry for the driver.\n"
    "  delete:    delete the registry entry for the driver.\n"
#if defined(WIN32)
    "  start:     start the driver.\n"
    "  stop:      stop the driver.\n"
    "  install:   stop + start (if an older version exists)\n"
    "             or create + start (otherwise).\n"
    "  uninstall: stop + delete.\n"
#else
    "  install:   same as the \"create\" action.\n"
    "  uninstall: same as the \"delete\" action.\n"
#endif

    "\n"

#if defined(WIN32)
    "WDM Drivers: (Win2000/XP/Server 2003/Vista)\n"
#else
    "WDM Drivers: (Win98/Me)\n"
#endif
    "\n"
    "%s -inf <filename> [-silent] [-log <filename>] Action [Action ...]\n"
    "Options:\n"
    "  -inf <filename>: Full path to an INF file to be dynamically loaded.\n"
    "  -silent:         Suppress display of all messages.\n"
    "  -log <filename>: Log all messages to filename.\n"
#if defined(WIN32) // Win2000/XP/Server 2003/Vista
    "  -compat:         Use traditional SetupDi API instead of DIFxAPI.\n"
    "  -dont_create_virtual: Don't create virtual device if one does not already exist.\n"
#endif
    "Actions:\n"
    "  install:         Install the INF file (if needed, reloads new driver file).\n"
#if defined(WIN32) // Win2000/XP/Server 2003/Vista
    "  preinstall:      Pre-install the INF file for a non-present device.\n"
#endif
    "  uninstall:       Uninstall the INF file.\n"
    "  enable:          Enable the driver.\n"
    "  disable:         Disable the driver.\n"
    "\n"
#if defined(WIN32)
    "%s -rescan <enumerator> [-silent] [-log <filename>]\n"
    "Options:\n"
    "  -rescan <enumerator>: Rescan enumerator (ROOT, ACPI, PCI, USB etc.) for\n"
    "                        hardware changes. Only one enumerator can be specified.\n"
    "  -silent:              Suppress all messages.\n"
    "  -log <filename>:      Log all messages to filename.\n"
    "\n"
#endif
    "For more information, please refer to " WD_PROD_NAME " manual.\n"
    "\n"
   ,exe
   ,exe
#if defined(WIN32)
   ,exe
#endif
   );
}

int main (int argc, char *argv[])
{
    char *sDriverName = NULL;
    char *sInfName = NULL;
    char sFullInfName[MAX_PATH];
#if !defined(WIN16)
    char *sDummy;
#endif
    char *sDriverFile = NULL;
    char *sBus = NULL;
    BOOL fRescan = FALSE;
    BOOL fSuccess = FALSE;
    BOOL fVxd = FALSE;
    BOOL fDifxapi = FALSE;
    BOOL fDontCreateVirtual = FALSE;
    DWORD dwStartLevel;
    int i, iReturn;
    OS_TYPE iOsType = get_os_type();
    CDriver *pDriver = NULL;
    char *sTmpLogFileName = NULL;

    // this option maybe changed by user by specifying 
    // -startup or specifying -inf option will change startup mode 
    // to on demand ( -inf mean that this will be wdm driver)
    dwStartLevel = (DWORD)-1;
    if (argc==1)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    DisplayMessage(WDREG_VERSION_STR "\n");

    if (iOsType == OS_WIN_NT_5 || iOsType == OS_WIN_NT_6)
        fDifxapi = TRUE;

    // read flags
    for (i=1; i<argc; i++)
    {
        if (argv[i][0]!='-')
            break;

        if (!stricmp(argv[i],"-silent"))
        {
            g_fSilent = TRUE;
            continue;
        }
        if (!stricmp(argv[i],"-log"))
        {
            i++;
            if (i==argc)
            {
                DisplayError("Please specify a filename after the '-log' option\n");
                return EXIT_FAILURE;
            }
            sTmpLogFileName = argv[i];
            continue;
        }
        if (!stricmp(argv[i],"-vxd"))
        {
            fVxd = TRUE;
            continue;
        }
        if (!stricmp(argv[i],"-inf"))
        {
            i++;
            if (i==argc)
            {
                DisplayError("Please specify a filename after the '-inf' option\n");
                return EXIT_FAILURE;
            }
            sInfName = argv[i];
            continue;
        }            
        if (!stricmp(argv[i],"-name"))
        {
            i ++;
            if (i==argc)
            {
                DisplayError("Please specify a name after the '-name' option\n");
                return EXIT_FAILURE;
            }
            sDriverName = argv[i];
            continue;
        }
        if (!stricmp(argv[i],"-startup"))
        {
            i ++;
            if (i==argc)
            {
                DisplayError("Please specify a startup level after the '-startup' option\n");
                return EXIT_FAILURE;
            }
            if (!stricmp(argv[i],"boot"))
                dwStartLevel = SERVICE_BOOT_START;
            else if (!stricmp(argv[i],"system"))
                dwStartLevel = SERVICE_SYSTEM_START;
            else if (!stricmp(argv[i],"automatic"))
                dwStartLevel = SERVICE_AUTO_START;
            else if (!stricmp(argv[i],"demand"))
                dwStartLevel = SERVICE_DEMAND_START;
            else if (!stricmp(argv[i],"disabled"))
                dwStartLevel = SERVICE_DISABLED;
            else
            {
                DisplayError("Please specify one of those values after the '-startup' option\n"
                    "  boot, system, automatic, demand, disabled\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        else if (!stricmp(argv[i],"-file"))
        {
            i++;
            if (i==argc)
            {
                DisplayError("Please specify a filename after the '-file' option\n");
                return EXIT_FAILURE;
            }
            sDriverFile = argv[i];
            continue;
        }
#if defined(WIN32)
        else if (!stricmp(argv[i],"-rescan"))
        {
            i++;
            if (i==argc)
            {
                DisplayError("Please specify an enumerator after the '-rescan' option\n");
                return EXIT_FAILURE;
            }
            else
                sBus = argv[i]; // a user specified bus will be scanned

            fRescan = TRUE;
            continue;
        }
        else if (!stricmp(argv[i],"-compat"))
        {
            fDifxapi = FALSE;
            continue;
        }
        else if (!stricmp(argv[i],"-dont_create_virtual"))
        {
            fDontCreateVirtual = TRUE;
            continue;
        }
#endif
        else
        {
            DisplayError("unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (sTmpLogFileName)
    {
        char *buf;
        time_t now;
        int j;

        buf = (char *) malloc(512);
        sLogFileName = sTmpLogFileName;
        time(&now);
        sprintf(buf, "\n" WDREG_VERSION_STR "\nLog from %s\n", ctime(&now));
        if (!LogMessage(buf))
            DisplayError("Cannot open log file %s\n", sLogFileName);
        strcpy(buf, "Command line:");
        for (j = 0; j < argc; j++)
        {
            strcat(buf, " ");
            strcat(buf, argv[j]);
        }
        strcat(buf, "\n");
        LogMessage(buf);

        free(buf);
    }

    // default flags
    if (iOsType == OS_WIN_95)
        fVxd = TRUE;

    // sanity checks
    if (iOsType != OS_WIN_95 &&
        iOsType != OS_WIN_98 &&
        iOsType != OS_WIN_NT_4 &&
        iOsType != OS_WIN_NT_5 &&
        iOsType != OS_WIN_NT_6)
    {
        DisplayError("Unsupported operating system\n");
        goto Exit;
    }

#if defined(WIN16)
    if (fVxd)
    {
        DisplayError("WDREG16 does not install VXD drivers, please use WDREG.EXE (the 32-bit version)\n");
        goto Exit;
    }
#else 
    if (!fVxd && iOsType == OS_WIN_98)
    { 
        DisplayError("Use WDREG16 for installing WDM drivers on Windows 95/98/Me system\n");
        goto Exit;
    }
    if (fVxd && iOsType != OS_WIN_95 && iOsType != OS_WIN_98)
    {
        DisplayError("Cannot use vxd for this operating system, use \".sys\" file\n");
        goto Exit;
    } 
#endif

    if (sInfName && (iOsType == OS_WIN_95 || iOsType == OS_WIN_NT_4))
    {
        DisplayError("Cannot use -inf for this operating system\n");
        goto Exit;
    }
    
    if (sInfName && (fRescan || fVxd || dwStartLevel != (DWORD)-1 || sDriverName || sDriverFile))
    {
        DisplayError("Cannot use -inf with other flags (except -silent and -log).\n"
            "Run without parameters to see the correct usage.\n");
        goto Exit;
    }

    // assign default names
    if (!sInfName && !fRescan)
    {
        // non-WDM drivers
        if (dwStartLevel == (DWORD)-1)
            dwStartLevel = SERVICE_AUTO_START;
        if (!sDriverName && !sDriverFile)
        {
            if (!fVxd && (iOsType == OS_WIN_98 || iOsType == OS_WIN_NT_5))
            {
                DisplayError("You need to use one of the following flags: -inf / -name / -vxd.\n"
                    "For detailed usage information, run \"wdreg\".\n");
                goto Exit;
            }
            sDriverName = WD_DISPLAY_NAME;
            sDriverFile = "WINDRVR6";
        }
        else if (!sDriverName)
            sDriverName = sDriverFile;
        else if (!sDriverFile)
            sDriverFile = sDriverName;
    }

    // read actions

    if (i==argc && !fRescan)
    {
        DisplayMessage("no action specified: nothing to do!\n");
        goto Exit;
    }

    if (fRescan)
        pDriver = new CDriverWDM(sInfName, fDifxapi, fDontCreateVirtual);
    else
    {
        if (sInfName)
        {
            // WDM Drivers
#if defined(WIN16)
            if (!_fullpath(sFullInfName, sInfName, MAX_PATH))
#else
            if (!GetFullPathName(sInfName, MAX_PATH, sFullInfName, &sDummy))
#endif
            {
                DisplayMessage("Warning: failed getting full path for %s, using "
                    "it as is\n", sInfName);
            }
            else
            {
                sInfName = sFullInfName;
            }

            pDriver = new CDriverWDM(sInfName, fDifxapi, fDontCreateVirtual);
        }
        else 
        { 
#if defined(WIN16) 
            // 16-bit non-WDM Drivers 
            pDriver = (CDriver *) new CDriver98(sDriverName, sDriverFile, dwStartLevel);
#else
            // non-WDM Drivers
            pDriver = fVxd ?
                (CDriver *) new CDriverVXD(sDriverName, sDriverFile, dwStartLevel) :
                    (CDriver *) new CDriverNT4(sDriverName, sDriverFile, dwStartLevel);
#endif
        }
    }

    if (!pDriver)
    {
        DisplayError("Creating driver failed\n");
        goto Exit;
    }
    if (!pDriver->Init())
    {
        DisplayError("Initializing driver failed\n");
        goto Exit;
    }
#if defined(WIN32)
    if (fRescan)
    {
        fSuccess = RescanBus(sBus);
        goto Exit;
    }
#endif
        
    for (; i<argc; i++)
    {
        if (!stricmp(argv[i],"install"))
            fSuccess = pDriver->Install();
        else if (!stricmp(argv[i],"preinstall"))
            fSuccess = pDriver->Preinstall();
        else if (!stricmp(argv[i],"uninstall"))
            fSuccess = pDriver->Uninstall();
        else if (!stricmp(argv[i],"enable"))
            fSuccess = pDriver->Enable();
        else if (!stricmp(argv[i],"disable"))
            fSuccess = pDriver->Disable();
        else if (!stricmp(argv[i],"create"))
            fSuccess = pDriver->Create();
        else if (!stricmp(argv[i],"delete"))
            fSuccess = pDriver->Delete();
        else if (!stricmp(argv[i],"start"))
            fSuccess = pDriver->Start();
        else if (!stricmp(argv[i],"stop"))
            fSuccess = pDriver->Stop();
        else
        {
            DisplayError("Invalid parameter %s\n", argv[i]);
            continue;
        }

        if (fSuccess)
            if (fSuccess == WD_SUCCESS_UNSUPPORTED)
                DisplayError("%s: unsupported action for this driver\n", argv[i]);
            else
                DisplayMessage("%s: completed successfully\n", argv[i]);
        else
        {
            DisplayError("Failed trying to %s the driver\n", argv[i]);
            break;
        }
    }

Exit:
    if (pDriver)
        delete pDriver;

    if (g_fRebootRequired)
    {
        DisplayMessage("Please reboot the computer in order to "
            "complete the action\n");
        iReturn = EXIT_REBOOT;
        LogMessage("STATUS_REBOOT_REQUIRED\n");
    }
    else if (!fSuccess)
    {
        iReturn = EXIT_FAILURE;
        LogMessage("STATUS_FAILURE\n");
    }
    else
    {
        iReturn = 0;
        LogMessage("STATUS_SUCCESS\n");
    }

    return iReturn;
}


/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/*
 * File - WDDEBUG.C
 *
 * A utility that turns WinDriver's debug mode on and off.
 * Debug mode checks every IO and memory transfer command,
 * making sure it fits in with the card's registered
 * resources. If an illegal command is given, WinDriver
 * will ignore the command and show a warning message on
 * screen. Debug mode slows down transfer operations, 
 * therefore it should be used only in the development process.
 * Running this command without parameters will print the
 * version of WinDriver installed.
 * 
 * If debug mode is set, this utility also enables you to print
 * out debug messages from the kernel, by the dump command.
 */

#include "windrvr.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(UNIX)
    #include <unistd.h>
    #include <fcntl.h>
    #if !defined(VXWORKS)
        #include <sys/utsname.h> 
    #endif
#elif !defined(WINCE)
    #include <conio.h>
#endif
#if !defined(WINCE)
    #include <time.h>
#else
    extern time_t time();
#endif
#if defined(UNIX)
#define stricmp strcasecmp
#elif defined(WINCE)
#define stricmp _stricmp
#endif
    
#if defined(UNIX)
    void Sleep(int msec)
    {
    #if defined(VXWORKS)
        taskDelay(1);
    #else
        usleep(msec * 1000);
    #endif
    }
#endif

static const char *sDriverName = WD_PROD_NAME;

static void Usage(void)
{
    printf(
"This program sets debugging mode of WinDriver on/off\n"
"WDDEBUG [drivername] off      - set debugging mode OFF.\n"
"WDDEBUG [drivername] on       - set debugging mode ON at ERROR level, for\n"
"                                 all sections.\n"
"WDDEBUG [drivername] dbg_on [level] [sections...] - log WinDriver debug\n"
"                                 messages to OS's kernel logger and set\n"
"                                 debugging mode on for specific sections.\n"
"WDDEBUG [drivername] dbg_off  - stop logging WinDriver debug messages to\n"
"                                 OS's kernel logger.\n"
"WDDEBUG [drivername] on [level] [sections...] - set debugging mode on for\n"
"                                 specific sections.\n"
"WDDEBUG [drivername] status   - print the current version of WinDriver and\n"
"                                 debug status.\n"
"WDDEBUG [drivername] dump     - continuously print out debug messages.\n"
"\n"
"    level - ERROR, WARN, INFO, TRACE\n"
"    sections - ALL, IO, MEM, INT, PCI, PCMCIA, ISAPNP, \n"
"               DMA, KER_PLUG, MISC, LICENSE, CARD_REG\n"
"               PCMCIA, KER_PLUG, KER_DRV, EVENT\n"
"\n"
"Example: Turn on and view debugging for PCI and DMA routines, at info level\n"
"    WDDEBUG on info \"pci dma\"\n"
"    WDDEBUG dump\n"
"Example: For \"my_driver\", log to kernel logger at TRACE level for ALL sections\n"
"    WDDEBUG my_driver dbg_on trace all\n");
}

static BOOL LevelToDword(const char *sLevel, DWORD *pdwLevel)
{
    DWORD dwLevel;

    if (stricmp(sLevel,"ERROR")==0)
        dwLevel = D_ERROR;
    else if (stricmp(sLevel,"WARN")==0)
        dwLevel = D_WARN;
    else if (stricmp(sLevel,"INFO")==0)
        dwLevel = D_INFO;
    else if (stricmp(sLevel,"TRACE")==0)
        dwLevel = D_TRACE;
    else if (stricmp(sLevel,"OFF")==0)
        dwLevel = D_OFF;
    else
        return FALSE;

    *pdwLevel = dwLevel;
    return TRUE;
}

static const char *LevelToString(DWORD dwLevel)
{
    if (dwLevel==D_OFF)
        return "OFF";
    if (dwLevel==D_ERROR)
        return "ERROR";
    if (dwLevel==D_WARN)
        return "WARN";
    if (dwLevel==D_INFO)
        return "INFO";
    if (dwLevel==D_TRACE)
        return "TRACE";
    return "";
}

static BOOL SectionToDword(const char *sSection, DWORD *pdwSection)
{
    char tokBuf[1024];
    char *tok;
    *pdwSection = 0;
    strcpy (tokBuf, sSection);
    for (tok = strtok(tokBuf, " "); tok; tok = strtok(NULL, " "))
    {
        if (stricmp(tok, "ALL")==0)
            *pdwSection |= S_ALL;
        else if (stricmp(tok, "IO")==0)
            *pdwSection |= S_IO;
        else if (stricmp(tok, "MEM")==0)
            *pdwSection |= S_MEM;
        else if (stricmp(tok, "INT")==0)
            *pdwSection |= S_INT;
        else if (stricmp(tok, "PCI")==0)
            *pdwSection |= S_PCI;
        else if (stricmp(tok, "DMA")==0)
            *pdwSection |= S_DMA;
        else if (stricmp(tok, "ISAPNP")==0)
            *pdwSection |= S_ISAPNP;
        else if (stricmp(tok, "PCMCIA")==0)
            *pdwSection |= S_PCMCIA;
        else if (stricmp(tok, "KER_PLUG")==0)
            *pdwSection |= S_KER_PLUG;
        else if (stricmp(tok, "MISC")==0)
            *pdwSection |= S_MISC;
        else if (stricmp(tok, "LICENSE")==0)
            *pdwSection |= S_LICENSE;
        else if (stricmp(tok, "PCMCIA")==0)
            *pdwSection |= S_PCMCIA;
        else if (stricmp(tok, "KER_PLUG")==0)
            *pdwSection |= S_KER_PLUG;
        else if (stricmp(tok, "CARD_REG")==0)
            *pdwSection |= S_CARD_REG;
        else if (stricmp(tok, "KER_DRV")==0)
            *pdwSection |= S_KER_DRV;
        else if (stricmp(tok, "EVENT")==0)
            *pdwSection |= S_EVENT;
        else if (stricmp(tok, "PNP")==0)
            *pdwSection |= S_PNP;
        else if (tok[0]=='0' && toupper(tok[1])=='x')
        {
            DWORD dwSection;
            sscanf(tok+2, "%lx", &dwSection);
            *pdwSection |= dwSection;
        }
        else
            return FALSE;
    }

    return TRUE;
}

char *SectionToString(DWORD dwSection)
{
    static char sSection[1024];

    sSection[0] = '\0';
    if (dwSection==(DWORD)S_ALL) 
    {
        strcat (sSection, "ALL ");
        return sSection;
    }
    if (dwSection & S_IO)       { strcat (sSection, "IO ");       dwSection &= ~S_IO; }
    if (dwSection & S_MEM)      { strcat (sSection, "MEM ");      dwSection &= ~S_MEM; }
    if (dwSection & S_INT)      { strcat (sSection, "INT ");      dwSection &= ~S_INT; }
    if (dwSection & S_PCI)      { strcat (sSection, "PCI ");      dwSection &= ~S_PCI; }
    if (dwSection & S_DMA)      { strcat (sSection, "DMA ");      dwSection &= ~S_DMA; }
    if (dwSection & S_ISAPNP)   { strcat (sSection, "ISAPNP ");   dwSection &= ~S_ISAPNP; }
    if (dwSection & S_PCMCIA)   { strcat (sSection, "PCMCIA ");   dwSection &= ~S_PCMCIA; }
    if (dwSection & S_KER_PLUG) { strcat (sSection, "KER_PLUG "); dwSection &= ~S_KER_PLUG; }
    if (dwSection & S_MISC)     { strcat (sSection, "MISC ");     dwSection &= ~S_MISC; }
    if (dwSection & S_LICENSE)  { strcat (sSection, "LICENSE ");  dwSection &= ~S_LICENSE; }
    if (dwSection & S_PCMCIA)   { strcat (sSection, "PCMCIA ");   dwSection &= ~S_PCMCIA; }
    if (dwSection & S_KER_PLUG) { strcat (sSection, "KER_PLUG "); dwSection &= ~S_KER_PLUG; }
    if (dwSection & S_CARD_REG) { strcat (sSection, "CARD_REG "); dwSection &= ~S_CARD_REG; }
    if (dwSection & S_KER_DRV)  { strcat (sSection, "KER_DRV ");  dwSection &= ~S_KER_DRV; }
    if (dwSection & S_EVENT)    { strcat (sSection, "EVENT ");    dwSection &= ~S_EVENT; }
    if (dwSection & S_PNP)    { strcat (sSection, "PNP ");    dwSection &= ~S_PNP; }

    if (dwSection)
        sprintf (sSection+strlen(sSection), "0x%08lx", dwSection);
    return sSection;
}

void Print_version(WD_VERSION *pVer)
{
    printf ("%s v%ld.%02ld installed (%s)\n", sDriverName, 
        pVer->dwVer / 100, pVer->dwVer % 100, pVer->cVer);
}

void Print_status(HANDLE hWD)
{
    WD_DEBUG debug;
    BZERO(debug);
    debug.dwCmd = DEBUG_STATUS;
    WD_Debug (hWD, &debug);
    printf("Debug level (%ld) %s, Debug sections (0x%08x) %s, Buffer size %ld\n", 
        debug.dwLevel, LevelToString(debug.dwLevel), (UINT32)debug.dwSection,
        SectionToString(debug.dwSection), debug.dwBufferSize);
}

#if defined(VXWORKS)
    int main(char *arg1, char *arg2, char *arg3 )
#else
    int main(int argc, char *argv[]) 
#endif
{
    WD_VERSION verBuf;
    HANDLE hWD;
    WD_DEBUG debug;
    BOOL fDriverNameChange = FALSE;
    
    #if defined(VXWORKS)
        int argc=1;
        char *argv[4];
        
        argv[0] = "wddebug_main";
        
        if(arg1 != NULL)
        {
            argv[1] = arg1;
            argc++;
        }
        if(arg2 != NULL)
        {
            argv[2] = arg2;
            argc++;
        }
        if(arg3 != NULL)
        {
            argv[3] = arg3;
            argc++;
        }
    #endif
    
    BZERO(debug);    

    /* Check [drivername] option */
    if (argc > 2 && (stricmp(argv[1], "off")!=0 && stricmp(argv[1], "on")!=0 &&
        stricmp(argv[1], "dbg_on")!=0 && stricmp(argv[1], "dbg_off")!=0 &&
        stricmp(argv[1], "status")!=0 && stricmp(argv[1], "dump")!=0))
    {
        sDriverName = argv[1];
        /* When using the [drivername] option, shift argc and argv */
        argc --;
        argv ++;

        /* Set Driver Name */
        if (!WD_DriverName(sDriverName))
        {
            printf("Error: Cannot set driver name to %s.\n", sDriverName);
            return EXIT_FAILURE;
        }
    }

    hWD = WD_Open();    
    if (hWD==INVALID_HANDLE_VALUE)
    {
        printf("Error: %s device not installed.\n", sDriverName);
        return EXIT_FAILURE;
    }

    BZERO(verBuf);
    WD_Version(hWD, &verBuf);
    if (argc<2)
    {
        Print_version(&verBuf);
        printf("\n");
        Usage();
        return EXIT_SUCCESS;
    }

    if (verBuf.dwVer<WD_VER)
    {
        Print_version(&verBuf);
        printf ("Please update the %s installed to v%d.%02d, or newer.\n", 
            sDriverName, WD_VER / 100, WD_VER % 100);
        WD_Close(hWD);
        return EXIT_FAILURE;
    }

    if (stricmp(argv[1], "on")==0 || stricmp(argv[1], "dbg_on")==0)
    {
        debug.dwCmd = DEBUG_SET_FILTER;
        debug.dwLevel = D_ERROR;
        debug.dwSection = (DWORD) S_ALL;

        if (argc>2)
        {
            if (argc>4)
            {
                printf ("Too many arguments\n");
                Usage();
                return EXIT_FAILURE;
            }

            if (!LevelToDword(argv[2], &debug.dwLevel))
            {
                printf ("invalid level name (%s)\n", argv[2]);
                Usage();
                return EXIT_FAILURE;
            }

            if (argc==4 && !SectionToDword(argv[3], &debug.dwSection))
            {
                printf ("invalid section name (%s)\n", argv[3]);
                Usage();
                return EXIT_FAILURE;
            }
        }
        
        if (argc > 2 || stricmp(argv[1], "on") == 0)
            WD_Debug (hWD, &debug);
        Print_status (hWD);
        
        if (stricmp(argv[1],"dbg_on")==0)
        {
            debug.dwCmd = KERNEL_DEBUGGER_ON;
            WD_Debug(hWD, &debug);
            printf("WinDriver's debug messages are directed to kernel debugger.\n");
        }
    }
    else if (stricmp(argv[1],"off")==0)
    {
        if (argc>2)
        {
            printf ("Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_SET_FILTER;
        debug.dwLevel = D_OFF;
        debug.dwSection = 0;
        WD_Debug (hWD, &debug);
    }
    else if (stricmp(argv[1],"status")==0)
    {
        if (argc>2)
        {
            printf ("Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        Print_version (&verBuf);
        Print_status (hWD);
    }
    else if (stricmp(argv[1],"dump")==0)
    {
        WD_DEBUG_DUMP debugDump;
        char buf[2048];
        time_t ltime;

#if defined(UNIX)
        int stdin_fileno = 
    #if defined(VXWORKS)
            STD_IN;
    #else
        STDIN_FILENO;
    #endif
#elif defined(WIN32)
        OSVERSIONINFO lVerInfo;
        const char *sOSName;
#endif

        if (argc>2)
        {
            printf ("Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        time(&ltime);
#if defined(WIN32)
        lVerInfo.dwOSVersionInfoSize = sizeof (lVerInfo);
        GetVersionEx (&lVerInfo);
#endif

        printf ("WDDEBUG v%d.%02d Debug Monitor.\n", WD_VER / 100, 
                        WD_VER % 100);
        printf ("Running %s\n", verBuf.cVer);
#if !defined(WINCE)
        printf ("Time: %s", ctime (&ltime));
#endif
#if defined(WIN32)
        switch (lVerInfo.dwPlatformId)
        {
        case VER_PLATFORM_WIN32_NT:
            sOSName = "NT";
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            if (lVerInfo.dwMinorVersion)
                sOSName = "98";
            else
                sOSName = "95";
            break;
#if defined(VER_PLATFORM_WIN32_CE)
        case VER_PLATFORM_WIN32_CE:
            sOSName = "CE";
            break;
#endif
        default:
            sOSName = "";
        }
        printf ("OS: Windows %s %ld.%ld Build %ld.%ld.%ld %s\n",
            sOSName,
            lVerInfo.dwMajorVersion,
            lVerInfo.dwMinorVersion,
            lVerInfo.dwBuildNumber >> 24 & 0xff,
            lVerInfo.dwBuildNumber >> 16 & 0xff,
            lVerInfo.dwBuildNumber & 0xffff,
            lVerInfo.szCSDVersion);
#elif defined(VXWORKS)
        printf("OS: VxWorks\n");
#elif defined(LINUX) || defined(SOLARIS)
        {
            struct utsname ver;
            if (uname(&ver))
            {
#if defined(LINUX)
                printf("OS: Linux\n");
#else
                printf("OS: Solaris\n");
#endif
            }
            else
            {
                printf("OS: %s %s %s %s\n", ver.sysname, ver.release,
                    ver.version, ver.machine);
            }
        }
#else
        printf("OS: Unknown\n");
#endif
#if defined(UNIX)
#if defined(VXWORKS)
        printf("Press CTRL-BREAK to exit\n");
#else
        printf("Press enter to exit\n");
#endif
#else
        printf("Press ESC to exit\n");
#endif
        printf ("\n");
        BZERO (debugDump);
        debugDump.pcBuffer = buf;
        debugDump.dwSize = sizeof (buf);
#if defined(UNIX) && !defined(VXWORKS)
        fcntl(stdin_fileno, F_SETFL, fcntl(stdin_fileno, F_GETFL, 0) |
            O_NONBLOCK);
#endif
        for (;;)
        {
#if defined(UNIX) && !defined(VXWORKS)
            char buf[4];
            int nRead = read(stdin_fileno, buf, 4);
            if (nRead>0)
                break;
#elif defined(VXWORKS)
            // Will be implemented      
#elif defined(WIN32) && !defined(WINCE)
            if (kbhit() && getch()==27) break;
#endif
            do
            {
                WD_DebugDump(hWD, &debugDump);
                printf("%s", debugDump.pcBuffer);
            } while(debugDump.pcBuffer[0]);
            Sleep(100);
        }
    }
    else if (stricmp(argv[1], "dbg_off")==0)
    {
        debug.dwCmd = KERNEL_DEBUGGER_OFF;
        WD_Debug(hWD, &debug);
        printf("Stop direct WinDriver's debug messages to kernel debugger.\n");
    }
    else
    {
        printf("Invalid option (%s)\n", argv[1]);
        Usage();
    }

    WD_Close(hWD);
    return EXIT_SUCCESS;
}


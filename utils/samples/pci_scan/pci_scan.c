/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - PCI_SCAN.C
//
// A utility for getting a list of the PCI cards installed 
// and the resources allocated for each one of them (memory 
// ranges, IO ranges and interrupts).
// 
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "windrvr.h"
#include "samples/shared/wdc_diag_lib.h"
#include "status_strings.h"

int main(int argc, char *argv[]) 
{

    DWORD dwStatus = WD_WINDRIVER_STATUS_ERROR;
    FILE *fp = stdout;

    if (argc !=1 && argc != 2)
    {
        printf("USAGE: %s [filename]\n", argv[0]);
        return -1;
    }
    if (argc == 2)
    {
        if (NULL == (fp = fopen(argv[1], "w")))
        {
            perror(argv[1]);
            return -1;
        }
    }
   
    fprintf(fp, "pci bus scan (using WD_PciConfigDump)\n");

    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_CHECK_VER, NULL);
    if (dwStatus)
        goto Exit;

    WDC_DIAG_PciDevicesInfoPrintAllFile(fp, FALSE);
  
Exit:
    WDC_DriverClose();
    if (dwStatus)
    {
        fprintf(fp, "%s failed, 0x%lx - %s\n", argv[0], dwStatus,
            Stat2Str(dwStatus));
    }
    if (fp)
        fclose(fp);
    return dwStatus == WD_STATUS_SUCCESS ? 0 : -1;
}


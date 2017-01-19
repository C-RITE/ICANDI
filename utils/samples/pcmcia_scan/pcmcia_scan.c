/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - PCMCIA_SCAN.C
//
// A utility for getting a list of the PCMCIA cards installed 
// and the resources allocated for each one of them (memory 
// ranges, IO ranges and interrupts).
// 
////////////////////////////////////////////////////////////////

#include "windrvr.h"
#include "samples/shared/print_struct.h"
#include "samples/shared/pcmcia_diag_lib.h"

int main (int argc, char *argv[]) 
{
    PCMCIA_Print_all_cards_info();

    return 0;
}

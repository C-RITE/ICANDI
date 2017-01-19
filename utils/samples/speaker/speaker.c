/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - speaker.c
//
// This application plays a tone on the speaker, accessing it 
// directly on the motherboard using WinDriver.
// 
////////////////////////////////////////////////////////////////

#include "speaker_lib.h"
#include <stdio.h>

int main()
{
    SPEAKER_HANDLE hSPEAKER = NULL;

    printf ("SPEAKER diagnostic utility.\n");
    printf ("Application accesses hardware using " WD_PROD_NAME ".\n");

    if (!SPEAKER_Open(&hSPEAKER))
    {
        printf("error while opening SPEAKER:\n");
        printf("%s", SPEAKER_ErrorString);
        return 0;
    }

    SPEAKER_Tone(hSPEAKER, 440, 200);
    SPEAKER_Tone(hSPEAKER, 554, 200);
    SPEAKER_Tone(hSPEAKER, 659, 200);
    SPEAKER_Tone(hSPEAKER, 880, 800);

    SPEAKER_Close(hSPEAKER);

    return 0;
}


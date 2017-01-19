/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - INT_IO.C
//
// This is a skeleton driver for an ISA card. The driver 
// implements IO port access and interrupt handler installation.
// 
////////////////////////////////////////////////////////////////

#include "windrvr.h"
#include "windrvr_int_thread.h"
#include "status_strings.h"
#include <stdio.h>

// put your IO range here (this example 0x378-0x37a)
enum {MY_IO_BASE = 0x378};
enum {MY_IO_SIZE = 0x3};
// put your IRQ number to install here (this example IRQ 5)
enum {MY_IRQ = 5};

// global WinDriver handle
static HANDLE hWD = INVALID_HANDLE_VALUE;

static char line[256];

// this is equivalent to the assembler "inp" command
BYTE IO_ReadByte(DWORD dwIOAddr)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = RP_BYTE; // R-Read P-Port BYTE
    trns.dwPort = dwIOAddr;
    WD_Transfer( hWD, &trns); // Perform read
    return trns.Data.Byte;
}

// this is equivalent to the assembler "outp" command
void IO_WriteByte(DWORD dwIOAddr, BYTE bData)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = WP_BYTE; // R-Write P-Port BYTE
    trns.dwPort = dwIOAddr;
    trns.Data.Byte = bData;
    WD_Transfer( hWD, &trns); // Perform write
}

void IO_ReadByteString(DWORD dwIOAddr, PBYTE pBuf, DWORD dwBytes)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = RP_SBYTE; // R-Read, P-Port, S-String BYTE
    trns.dwPort = dwIOAddr;
    trns.dwBytes = dwBytes;
    trns.fAutoinc = FALSE;
    trns.dwOptions = 0;
    trns.Data.pBuffer = pBuf;
    WD_Transfer( hWD, &trns); // Perform read
}

void IO_WriteByteString(DWORD dwIOAddr, PBYTE pBuf, DWORD dwBytes)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = WP_SBYTE; // W-Write, P-Port, S-String BYTE
    trns.dwPort = dwIOAddr;
    trns.dwBytes = dwBytes;
    trns.fAutoinc = FALSE;
    trns.dwOptions = 0;
    trns.Data.pBuffer = pBuf;
    WD_Transfer( hWD, &trns); // Perform write
}

VOID DLLCALLCONV interrupt_handler (PVOID pData)
{
    WD_INTERRUPT *pIntrp = (WD_INTERRUPT *)pData;
    // do your interrupt routine here 
    printf ("Got interrupt %ld\n", pIntrp->dwCounter);
}

int main()
{
    WD_CARD_REGISTER cardReg;
    WD_VERSION verBuf;
    DWORD dwStatus;

    hWD = WD_Open();
    if (hWD==INVALID_HANDLE_VALUE)
    {
        printf ("error opening WINDRVR\n");
        return 0;
    }

    BZERO(verBuf);
    WD_Version (hWD, &verBuf);
    printf (WD_PROD_NAME " version - %s\n", verBuf.cVer);
    if (verBuf.dwVer<WD_VER)
    {
        printf ("error incorrect WINDRVR version, needs ver %s\n", WD_VER_STR);
        WD_Close(hWD);
        return 0;
    }

    printf ("register IRQ %d and IO range %X - %X\n", MY_IRQ, MY_IO_BASE,
        MY_IO_BASE + MY_IO_SIZE);

    BZERO(cardReg);
    cardReg.Card.dwItems = 2;
    cardReg.Card.Item[0].item = ITEM_INTERRUPT;
    cardReg.Card.Item[0].fNotSharable = TRUE;
    cardReg.Card.Item[0].I.Int.dwInterrupt = MY_IRQ;
    cardReg.Card.Item[0].I.Int.dwOptions = 0;
    cardReg.Card.Item[1].item = ITEM_IO;
    cardReg.Card.Item[1].fNotSharable = TRUE;
    cardReg.Card.Item[1].I.IO.dwAddr = MY_IO_BASE;
    cardReg.Card.Item[1].I.IO.dwBytes = MY_IO_SIZE;
    cardReg.fCheckLockOnly = FALSE;
    dwStatus = WD_CardRegister(hWD, &cardReg);
    if (dwStatus)
    {
        printf("Failed locking device. Status 0x%lx - %s",
            dwStatus, Stat2Str(dwStatus));
    }
    else
    {
        HANDLE thread_handle;
        WD_INTERRUPT *pIntrp;
        pIntrp = (WD_INTERRUPT *)malloc(sizeof(WD_INTERRUPT));  
        BZERO(*pIntrp);
        pIntrp->hInterrupt = cardReg.Card.Item[0].I.Int.hInterrupt;
        printf ("starting interrupt thread\n");
        // this calls WD_IntEnable() and creates an interrupt handler thread
        dwStatus = InterruptEnable(&thread_handle, hWD, pIntrp, interrupt_handler, pIntrp);
        if (dwStatus)
        {
            printf("failed enabling interrupt. Status 0x%lx - %s\n",
                dwStatus, Stat2Str(dwStatus));
        }
        else
        {
            // call your driver code here 
            printf ("Press Enter to uninstall interrupt\n");
            fgets(line, sizeof(line), stdin);
            
            // this calls WD_IntDisable()
            InterruptDisable(thread_handle);
        }
        WD_CardUnregister(hWD, &cardReg);
        free(pIntrp);
    }

    WD_Close(hWD);
    return 0;
}


/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

/*
 * File: i8253.c
 * 
 * An example of using the Intel 8253/8254 programmable timer chip.
 */

#include <stdio.h>
#include "windrvr.h"

enum {
    I8253_BASE = 0x40, /* IO port 40h */
    I8253_SIZE = 0x4,  /* 4 bytes */
    I8253_C0 = 0,      /* counter 0 register */
    I8253_CTRL = 0     /* control register */
};

/* global WinDriver handle and card handle */
static HANDLE hWD;
static WD_CARD_REGISTER cardReg;

static BYTE IO_inpb(DWORD dwIOAddr)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = RP_BYTE; /* R-Read P-Port BYTE */
    trns.dwPort = dwIOAddr;
    WD_Transfer(hWD, &trns); /* Perform read */
    return trns.Data.Byte;
}

static void IO_outpb(DWORD dwIOAddr, BYTE bData)
{
    WD_TRANSFER trns;
    BZERO(trns);
    trns.cmdTrans = WP_BYTE; /* W-Write P-Port BYTE */
    trns.dwPort = dwIOAddr;
    trns.Data.Byte = bData;
    WD_Transfer(hWD, &trns); /* Perform write */
}

static WORD i8253_read_c0(void)
{
        BYTE lsb, msb;

        /* latch the counter */
        IO_outpb(I8253_BASE + I8253_CTRL, 0x36);

        /* read LSB and MSB */
        lsb = IO_inpb(I8253_BASE + I8253_C0);
        msb = IO_inpb(I8253_BASE + I8253_C0);
        return (((WORD)msb << 8) | lsb);
}

static BOOL i8253_init(void)
{
    WD_VERSION verBuf;
    DWORD dwStatus;

    hWD = WD_Open();
    if (hWD==INVALID_HANDLE_VALUE)
    {
        printf("Error: failed opening the driver\n");
        return FALSE;
    }

    BZERO(verBuf);
    WD_Version(hWD, &verBuf);
    printf("%s version - %s\n", WD_PROD_NAME, verBuf.cVer);
    if (verBuf.dwVer<WD_VER)
    {
        printf("Error: incorrect %s version. Needs version %s\n", WD_PROD_NAME,
            WD_VER_STR);
        WD_Close(hWD);
        return FALSE;
    }

    BZERO(cardReg);
    cardReg.Card.dwItems = 1;
    cardReg.Card.Item[0].item = ITEM_IO;
    cardReg.Card.Item[0].fNotSharable = TRUE;
    cardReg.Card.Item[0].I.IO.dwAddr = I8253_BASE;
    cardReg.Card.Item[0].I.IO.dwBytes = I8253_SIZE;
    cardReg.fCheckLockOnly = FALSE;
    dwStatus = WD_CardRegister(hWD, &cardReg);
    if (dwStatus)
    {
        printf("Failed locking device. Status 0x%lx\n", dwStatus);
        WD_Close(hWD);
        return FALSE;
    }

    return TRUE;
}

static void i8253_uninit(void)
{
    WD_CardUnregister(hWD, &cardReg);
    WD_Close(hWD);
}

int main(void)
{
    if (!i8253_init())
        return -1;

    printf("Polling i8253 chip, press CTRL-C to quit:\n");
    while (1)
        printf("%x\r", i8253_read_c0());

    /* unreachable code */
    i8253_uninit();
    return 0;
}


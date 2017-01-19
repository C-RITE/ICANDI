/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - speaker_gui.c
//
// This application plays a tone to the speaker, and is
// controlled via a graphical user interface.
// The speaker is accessed directly on the motherboard, using
// WinDriver functions.
//  
////////////////////////////////////////////////////////////////

#include <windows.h>
#include "resource.h"
#include "../speaker/speaker_lib.h"
#include <stdio.h>

BOOL PASCAL MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL PASCAL AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

SPEAKER_HANDLE hSpeaker = NULL;
HINSTANCE ghInstance;

// The main window.
// WinMain() opens a handle for speaker, and then creates the main window.
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    ghInstance = hInstance;

    if (!SPEAKER_Open(&hSpeaker))
    {
        char msg[256];
        sprintf (msg, "Error while opening speaker hardware:\n%s", SPEAKER_ErrorString);
        MessageBox(NULL, msg, "Speaker Sample", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // create the Speaker window
    DialogBoxParam(hInstance, MAKEINTRESOURCE(PLAYTONEDLGBOX), NULL, (DLGPROC)MainDlgProc, 0);

    SPEAKER_Close(hSpeaker);

    return 0;
} 

// This is the About dialog Window Proc.
BOOL PASCAL AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDD_OK)
        {
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// This is the Speaker Main dialog Window Proc.
BOOL PASCAL MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_FREQ, "440");
        SetDlgItemText(hDlg, IDC_DURATION, "1000");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_PLAY_TONE:
            {
                DWORD dwHertz = GetDlgItemInt(hDlg, IDC_FREQ, NULL, FALSE);
                DWORD dwMilli = GetDlgItemInt(hDlg, IDC_DURATION, NULL, FALSE);
                if (dwHertz && dwMilli) 
                    SPEAKER_Tone(hSpeaker, dwHertz, dwMilli);
                break;
            } 

        case IDD_CLOSE:
            EndDialog(hDlg, TRUE);
            return TRUE;
        
        case  IDC_PLAY_CHIME:
            SPEAKER_Tone(hSpeaker, 440, 400);
            SPEAKER_Tone(hSpeaker, 329, 200);
            SPEAKER_Tone(hSpeaker, 1, 10);
            SPEAKER_Tone(hSpeaker, 329, 200);
            SPEAKER_Tone(hSpeaker, 369, 400);
            SPEAKER_Tone(hSpeaker, 329, 800);
            SPEAKER_Tone(hSpeaker, 415, 400);
            SPEAKER_Tone(hSpeaker, 440, 600);
            break;

        case IDC_ABOUT:
            DialogBoxParam(ghInstance, MAKEINTRESOURCE(ABOUTDLGBOX), NULL, (DLGPROC)AboutDlgProc, 0);
            break;

        }
        break;
    }

    return FALSE;
}

// CeChat - A Windows CE communication demo
//
// Written for the book Programming Windows CE
// Copyright (C) 2003 Douglas Boling
// 2020 Jasper ter Weeme

#include "resource.h"
#include "mainwin.h"
#include <cstdio>
#include <windows.h>

HINSTANCE hInst;

BOOL fContinue = TRUE;
HANDLE hComPort = INVALID_HANDLE_VALUE;
HANDLE g_hSendEvent = INVALID_HANDLE_VALUE;


#define TEXTSIZE 256
#define dim(x) (sizeof(x) / sizeof(x[0]))

//======================================================================
// SendThread - Sends characters to the serial port
//
static DWORD WINAPI SendThread(PVOID pArg)
{
    int rc;
    DWORD cBytes;
    TCHAR szText[TEXTSIZE];

    HWND hWnd = (HWND)pArg;
    HWND hwndSText = GetDlgItem(hWnd, ID_SENDTEXT);

    while (1)
    {
        rc = WaitForSingleObject(g_hSendEvent, INFINITE);

        if (rc != WAIT_OBJECT_0)
            break;

        if (!fContinue)
            break;

        // Disable send button while sending.
        EnableWindow (GetDlgItem (hWnd, ID_SENDBTN), FALSE);
        GetWindowText (hwndSText, szText, dim(szText));
        lstrcat (szText, TEXT ("\r\n"));

        rc = WriteFile (hComPort, szText,
                        lstrlen (szText)*sizeof (TCHAR),&cBytes, 0);

        if (rc)
        {
            // Copy sent text to output window.
            SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL, 0,
                                (LPARAM)TEXT (" >"));
            SetWindowText (hwndSText, TEXT (""));  // Clear text box
        }
        else
        {
            // Else, print error message.
            wsprintf (szText, TEXT ("Send failed rc=%d\r\n"),
                      GetLastError());
            DWORD dwErr = 0;
            COMSTAT Stat;

            if (ClearCommError(hComPort, &dwErr, &Stat))
            {
                printf("fail\n");
            }
        }

        // Put text in receive text box.
        SendDlgItemMessage(hWnd, ID_RCVTEXT, EM_REPLACESEL, 0, LPARAM(szText));
        EnableWindow(GetDlgItem (hWnd, ID_SENDBTN), TRUE);

	}
	return 0;
}

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPXSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    hInst = hInstance;
    MainWindow mainWin(hInstance);
    try
    {
        mainWin.create();

        // Create write thread. Read thread created when port opened.
        HWND hWnd = mainWin.hwnd();
        HANDLE hThread = CreateThread(NULL, 0, SendThread, hWnd, 0, NULL);

        if (hThread == NULL)
            throw TEXT("Cannot create thread");

        CloseHandle(hThread);
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
        HACCEL hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(ID_ACCEL));

        MSG msg;
        while (GetMessage (&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(hWnd, hAccel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        HANDLE hPort = hComPort;
        fContinue = FALSE;
        hComPort = INVALID_HANDLE_VALUE;

        if (hPort != INVALID_HANDLE_VALUE)
            CloseHandle(hPort);

        if (g_hSendEvent != INVALID_HANDLE_VALUE)
        {
            PulseEvent(g_hSendEvent);
            Sleep(100);
            CloseHandle(g_hSendEvent);
        }
    }
    catch (...)
    {
        MessageBox(0, TEXT("Unknown error"), TEXT("Error"), 0);
        return 0;
    }

    return 0;
}


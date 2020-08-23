//cechat
//mainwin.cpp

#include "mainwin.h"
#include "menubar.h"
#include "resource.h"
#include "toolbox.h"
#include <commctrl.h>

extern HANDLE g_hSendEvent;
extern BOOL fContinue;
extern HANDLE hComPort;

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

HWND MainWindow::hwnd() const
{
    return _hwnd;
}

void MainWindow::show(int nCmdShow) const
{
    ::ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update() const
{
    ::UpdateWindow(_hwnd);
}

void MainWindow::create()
{
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = _hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("CeChat");

    if (RegisterClass(&wc) == 0)
        throw TEXT("Registering class failed");

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Create unnamed auto-reset event initially false.
    g_hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Create main window.
    CONSTEXPR DWORD style = WS_VISIBLE;
    CONSTEXPR INT x = CW_USEDEFAULT;
    CONSTEXPR INT y = CW_USEDEFAULT;
    CONSTEXPR INT w = CW_USEDEFAULT;
    CONSTEXPR INT h = CW_USEDEFAULT;

    _hwnd = CreateWindow(wc.lpszClassName, TEXT("CeChat"),
                         style, x, y, w, h, NULL, NULL, _hInstance, NULL);

    if (!IsWindow(_hwnd))
        throw TEXT("Cannot create window");
}

// About Dialog procedure
static INT_PTR CALLBACK AboutDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM)
{
    switch (wMsg)
    {
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
            case IDOK:
            case IDCANCEL:
                EndDialog (hWnd, 0);
                return TRUE;
        }
        break;
    }
    return FALSE;
}



// DoMainCommandSendText - Process the Send text button.
static LPARAM DoMainCommandSendText(HWND hWnd, WORD, HWND, WORD)
{
    // Set event so that sender thread will send the text.
    SetEvent(g_hSendEvent);
    SetFocus(GetDlgItem(hWnd, ID_SENDTEXT));
    return 0;
}

static int nSpeed = CBR_19200;
static int nLastDev = -1;
static HANDLE hReadThread = INVALID_HANDLE_VALUE;

#define TEXTSIZE 256

// ReadThread - Receives characters from the serial port
static DWORD WINAPI ReadThread(PVOID pArg)
{
    HWND hWnd;
    DWORD cBytes, i;
    BYTE szText[TEXTSIZE], *pPtr;
    TCHAR tch;
    hWnd = (HWND)pArg;

    while (fContinue)
    {
        tch = 0;
        pPtr = szText;

        for (i = 0; i < sizeof (szText)-sizeof (TCHAR); i++)
        {
            while (!ReadFile (hComPort, pPtr, 1, &cBytes, 0))
                if (hComPort == INVALID_HANDLE_VALUE)
                    return 0;

            // This syncs the proper byte order for Unicode.
            tch = (tch << 8) & 0xff00;
            tch |= *pPtr++;

            if (tch == TEXT ('\n'))
                break;
        }
        *pPtr++ = 0;  // Avoid alignment problems by addressing as bytes.
        *pPtr++ = 0;

        // If out of byte sync, move bytes down one.
        if (i % 2)
        {
            pPtr = szText;

            while (*pPtr || *(pPtr+1))
            {
                *pPtr = *(pPtr + 1);
                pPtr++;
            }
            *pPtr = 0;
        }
        SendDlgItemMessage(hWnd, ID_RCVTEXT, EM_REPLACESEL, 0, (LPARAM)szText);
    }
    return 0;
}

// InitCommunication - Open and initialize selected COM port.
static HANDLE InitCommunication(HWND hWnd, LPTSTR pszDevName)
{
    DCB dcb;
    TCHAR szDbg[128];
    COMMTIMEOUTS cto;
    HANDLE hLocal;
    DWORD dwTStat;
    hLocal = hComPort;
    hComPort = INVALID_HANDLE_VALUE;

    if (hLocal != INVALID_HANDLE_VALUE)
        CloseHandle(hLocal);  // This causes WaitCommEvent to return.

    hLocal = CreateFile(pszDevName, GENERIC_READ | GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING, 0, NULL);

    if (hLocal != INVALID_HANDLE_VALUE)
    {
        // Configure port.
        dcb.DCBlength = sizeof (dcb);
        GetCommState (hLocal, &dcb);
        dcb.BaudRate = nSpeed;
        dcb.fParity = FALSE;
        dcb.fNull = FALSE;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;
        dcb.ByteSize = 8;
        SetCommState (hLocal, &dcb);

        // Set the timeouts. Set infinite read timeout.
        cto.ReadIntervalTimeout = 0;
        cto.ReadTotalTimeoutMultiplier = 0;
        cto.ReadTotalTimeoutConstant = 0;
        cto.WriteTotalTimeoutMultiplier = 0;
        cto.WriteTotalTimeoutConstant = 0;
        SetCommTimeouts (hLocal, &cto);
        wsprintf(szDbg, TEXT("Port %s opened\r\n"), pszDevName);
        SendDlgItemMessage(hWnd, ID_RCVTEXT, EM_REPLACESEL, 0, (LPARAM)szDbg);

        // Start read thread if not already started.
        hComPort = hLocal;

        if (!GetExitCodeThread(hReadThread, &dwTStat) ||
            (dwTStat != STILL_ACTIVE))
        {
            hReadThread = CreateThread(NULL, 0, ReadThread, hWnd,
                                        0, &dwTStat);
            if (hReadThread)
                CloseHandle(hReadThread);
        }
    }
    else
    {
        wsprintf(szDbg, TEXT("Couldn\'t open port %s. rc=%d\r\n"),
                  pszDevName, GetLastError());

        SendDlgItemMessage(hWnd, ID_RCVTEXT, EM_REPLACESEL, 0, (LPARAM)szDbg);
    }
    return hComPort;
}

// DoMainCommandComPort - Process the COM port combo box commands.
void MainWindow::_comPortComboProc(HWND hWnd, WORD, HWND hwndCtl, WORD wNotifyCode)
{
    if (wNotifyCode != CBN_SELCHANGE)
        return;

    int i = int(SendMessage(hwndCtl, CB_GETCURSEL, 0, 0));

    if (i != nLastDev)
    {
        TCHAR szDev[32];
        SendMessage(hwndCtl, CB_GETLBTEXT, i, LPARAM(szDev));
        InitCommunication(hWnd, szDev);
        SetFocus(GetDlgItem(hWnd, ID_SENDTEXT));
    }
}

LRESULT MainWindow::_commandProc(HWND hWnd, UINT, WPARAM wParam, LPARAM lParam)
{
    WORD idItem = WORD(LOWORD(wParam));
    WORD wNotifyCode = WORD(HIWORD(wParam));
    HWND hwndCtl = HWND(lParam);

    switch (idItem)
    {
    case IDC_COMPORT:
        _comPortComboProc(hWnd, idItem, hwndCtl, wNotifyCode);
        return 0;
    case ID_SENDBTN:
        return DoMainCommandSendText(hWnd, idItem, hwndCtl, wNotifyCode);
    case IDM_EXIT:
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    case IDM_ABOUT:
        DialogBox(_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);
        return 0;
    }

    return 0;
}

// FillComComboBox - Fills the COM port combo box
static int FillComComboBox(HWND hWnd)
{
    int rc;
    WIN32_FIND_DATA fd;
    HANDLE hFind;

    hFind = FindFirstFileEx(TEXT("COM?:"), FindExInfoStandard, &fd,
                             FindExSearchLimitToDevices, NULL, 0);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            SendDlgItemMessage(GetDlgItem(hWnd, IDC_CMDBAR),
                                IDC_COMPORT, CB_INSERTSTRING,
                                -1, (LPARAM)fd.cFileName);

            rc = FindNextFile(hFind, &fd);
        }
        while (rc);

        rc = FindClose(hFind);
    }
    SendDlgItemMessage(GetDlgItem(hWnd, IDC_CMDBAR), IDC_COMPORT, CB_SETCURSEL, 0, 0);
    return 0;
}

void MainWindow::_createProc(HWND hWnd, UINT, WPARAM, LPARAM lParam)
{
    TCHAR szFirstDev[32];
    LPCREATESTRUCT lpcs = LPCREATESTRUCT(lParam);
    (void)szFirstDev;
    (void)lpcs;

    // Create a command bar.
    _cmdBar = new CommandBar(_hInstance);
    _cmdBar->create(hWnd);
    _cmdBar->insertMenuBar();
    _cmdBar->insertComboBox();
    FillComComboBox(hWnd);
    _cmdBar->addAdornments();

    // Create child windows. They will be positioned in WM_SIZE.
    // Create receive text window.
    CONSTEXPR DWORD style1 = WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_READONLY;
    _hC1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""), style1,
                          0, 0, 10, 10, hWnd, HMENU(ID_RCVTEXT), _hInstance, NULL);

    // Create send text window.
    CONSTEXPR DWORD style2 = WS_VISIBLE | WS_CHILD;
    _hC2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""), style2,
                          0, 0, 10, 10, hWnd, (HMENU)ID_SENDTEXT, _hInstance, NULL);

    // Create send text window.
    CONSTEXPR DWORD style3 = WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON;
    _hBtn = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("button"), TEXT("&Send"),
                 style3, 0, 0, 10, 10, hWnd, HMENU(ID_SENDBTN), _hInstance, NULL);

    // Destroy frame if window not created.
    if (!IsWindow(_hC1) || !IsWindow(_hC2) || !IsWindow(_hBtn))
    {
        DestroyWindow(hWnd);
        return;
    }

    // Open a COM port.
#if 0
    for (int i = 0; i < 10; i++)
    {
        if (SendDlgItemMessage(hwndCB, IDC_COMPORT, CB_GETLBTEXT, i, LPARAM(szFirstDev)) == CB_ERR)
            break;

        if (InitCommunication(hWnd, szFirstDev) != INVALID_HANDLE_VALUE)
        {
            SendDlgItemMessage(hwndCB, IDC_COMPORT, CB_SETCURSEL, i, LPARAM(szFirstDev));
            break;
        }
    }
#endif
    return;
}

void MainWindow::DoSizeMain(HWND hWnd)
{
    RECT rect;

    // Adjust the size of the client rect to take into account
    // the command bar height.
    GetClientRect (hWnd, &rect);
#ifdef WINCE
    rect.top += CommandBar_Height(GetDlgItem (hWnd, IDC_CMDBAR));
#endif
    SetWindowPos(GetDlgItem(hWnd, ID_RCVTEXT), NULL, rect.left,
                  rect.top, rect.right - rect.left,
                  rect.bottom - rect.top - 25, SWP_NOZORDER);
    SetWindowPos(GetDlgItem (hWnd, ID_SENDTEXT), NULL, rect.left,
                  rect.bottom - 25, (rect.right - rect.left) - 50,
                  25, SWP_NOZORDER);
    SetWindowPos(GetDlgItem (hWnd, ID_SENDBTN), NULL,
                  (rect.right - rect.left) - 50, rect.bottom - 25,
                  50, 25, SWP_NOZORDER);
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        _createProc(hwnd, msg, wParam, lParam);
        return 0;
    case WM_SIZE:
        DoSizeMain(hwnd);
        return 0;
    case WM_COMMAND:
        return _commandProc(hwnd, msg, wParam, lParam);
    case WM_SETTINGCHANGE:
        break;
    case WM_ACTIVATE:
        break;
    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hwnd, ID_SENDTEXT));
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}


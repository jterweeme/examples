#include "mainwin.h"
#include "mdi_unit.h"
#include "main.h"
#include "winclass.h"
#include "statusbar.h"
#include "toolbar.h"
#include <commctrl.h>

MainWindow::MainWindow(WinClass *wc) : Element(NULL), _wc(wc), _tb(NULL)
{
    _instance = this;
}

MainWindow::~MainWindow()
{

}

MainWindow *MainWindow::_instance;

int MainWindow::alles(int nCmdShow)
{
    _hwnd = CreateWindowEx(WS_EX_APPWINDOW, _wc->className(),
        TEXT("MDI File Editor"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, _wc->hInstance(), NULL);

    if (_hwnd == NULL)
        throw "No Window";

    ShowWindow(_hwnd, nCmdShow);
    UpdateWindow(_hwnd);
    return 0;
}

BOOL LoadFile(HWND hEdit, LPCWSTR pszFileName)
{
    BOOL bSuccess = FALSE;

    HANDLE hFile = ::CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return bSuccess;

    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize != 0xFFFFFFFF)
    {
        LPSTR pszFileText = LPSTR(GlobalAlloc(GPTR, dwFileSize + 1));

        if (pszFileText != NULL)
        {
            DWORD dwRead;
            if (ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL))
            {
                pszFileText[dwFileSize] = 0; // Null terminator

                if (SetWindowTextA(hEdit, pszFileText))
                    bSuccess = TRUE; // It worked!
            }
            GlobalFree(pszFileText);
        }
    }

    CloseHandle(hFile);
    return bSuccess;
}

BOOL SaveFile(HWND hEdit, LPCWSTR pszFileName)
{
    BOOL bSuccess = FALSE;

    HANDLE hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return bSuccess;

    DWORD dwTextLength = GetWindowTextLength(hEdit);

    // No need to bother if there's no text.
    if (dwTextLength > 0)
    {
        LPSTR pszText = LPSTR(GlobalAlloc(GPTR, dwTextLength + 1));

        if (pszText != NULL)
        {
            if (GetWindowTextA(hEdit, pszText, dwTextLength + 1))
            {
                DWORD dwWritten;
                if (WriteFile(hFile, pszText, dwTextLength, &dwWritten, NULL))
                    bSuccess = TRUE;
            }
            GlobalFree(pszText);
        }
    }

    CloseHandle(hFile);
    return bSuccess;
}

BOOL GetFileName(HWND hwnd, LPWSTR pszFileName, BOOL bSave)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    pszFileName[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";

    if (bSave)
    {
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (!GetSaveFileName(&ofn))
            return FALSE;
    }
    else
    {
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if  (!GetOpenFileName(&ofn))
            return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK
MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

void MainWindow::_sizeProc(HWND hwnd)
{
    RECT rectClient, rectStatus, rectTool;
    _tb->sendMsgW(TB_AUTOSIZE, 0, 0);
    _sb->sendMsgW(WM_SIZE, 0, 0);
    GetClientRect(hwnd, &rectClient);
    GetWindowRect(_sb->handle(), &rectStatus);
    GetWindowRect(_tb->handle(), &rectTool);
    UINT uToolHeight = rectTool.bottom - rectTool.top;
    UINT uStatusHeight = rectStatus.bottom - rectStatus.top;
    UINT uClientAlreaHeight = rectClient.bottom;
    MoveWindow(g_hMDIClient, 0, uToolHeight, rectClient.right, uClientAlreaHeight - uStatusHeight - uToolHeight, TRUE);
}

void MainWindow::_createProc(HWND hwnd)
{
    CLIENTCREATESTRUCT ccs;
    int iStatusWidths[] = {200, 300, -1};

    // Find window menu where children will be listed
    ccs.hWindowMenu  = GetSubMenu(GetMenu(hwnd), 2);
    ccs.idFirstChild = ID_MDI_FIRSTCHILD;

    g_hMDIClient = CreateWindowEx(WS_EX_CLIENTEDGE, L"mdiclient", NULL,
                    WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hwnd, (HMENU)ID_MDI_CLIENT, _wc->hInstance(), (LPVOID)&ccs);

    ShowWindow(g_hMDIClient, SW_SHOW);
    _sb = new StatusBar(hwnd);
    _sb->create(_wc->hInstance());
    _sb->setParts(3, iStatusWidths);
    _sb->setText(2, L"Toolbar & Statusbar Example");
    _tb = new Toolbar(hwnd);
    _tb->create(_wc->hInstance());
}

void MainWindow::_commandProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
       case CM_FILE_EXIT:
           PostMessage(hwnd, WM_CLOSE, 0, 0);
           break;
       case CM_FILE_NEW:
       {
           MDICREATESTRUCT mcs;
           mcs.szTitle = L"[Untitled]";
           mcs.szClass = g_szChild;
           mcs.hOwner  = _wc->hInstance();
           mcs.x = mcs.cx = CW_USEDEFAULT;
           mcs.y = mcs.cy = CW_USEDEFAULT;
           mcs.style = MDIS_ALLCHILDSTYLES;
           HWND hChild = HWND(SendMessage(g_hMDIClient, WM_MDICREATE, 0, LPARAM(&mcs)));

           if (!hChild)
           {
              throw "MDI Child creation failed";
           }
       }
           break;
       case CM_FILE_OPEN:
       {
            MDICREATESTRUCT mcs;
            HWND hChild;
            wchar_t szFileName[MAX_PATH];

            if (!GetFileName(hwnd, szFileName, FALSE))
                break;

            mcs.szTitle = szFileName;
            mcs.szClass = g_szChild;
            mcs.hOwner  = _wc->hInstance();
            mcs.x = mcs.cx = CW_USEDEFAULT;
            mcs.y = mcs.cy = CW_USEDEFAULT;
            mcs.style = MDIS_ALLCHILDSTYLES;
            hChild = HWND(SendMessage(g_hMDIClient, WM_MDICREATE, 0, LPARAM(&mcs)));

            if (!hChild)
                throw "MDI Child creation failed";
       }
       break;
       case CM_WINDOW_TILEHORZ:
           PostMessage(g_hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
           break;
       case CM_WINDOW_TILEVERT:
           PostMessage(g_hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
           break;
       case CM_WINDOW_CASCADE:
           PostMessage(g_hMDIClient, WM_MDICASCADE, 0, 0);
           break;
       case CM_WINDOW_ARRANGE:
           PostMessage(g_hMDIClient, WM_MDIICONARRANGE, 0, 0);
           break;
       default:
           if (LOWORD(wParam) >= ID_MDI_FIRSTCHILD)
           {
               DefFrameProc(hwnd, g_hMDIClient, msg, wParam, lParam);
           }
           else
           {
               HWND hChild = (HWND)SendMessage(g_hMDIClient, WM_MDIGETACTIVE,0,0);

               if (hChild)
               {
                   SendMessage(hChild, WM_COMMAND, wParam, lParam);
               }
           }
    }
}

LRESULT
MainWindow::_wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
            _createProc(hwnd);
            return 0;
        case WM_COMMAND:
            _commandProc(hwnd, Message, wParam, lParam);
            break;
        case WM_SIZE:
            _sizeProc(hwnd);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefFrameProc(hwnd, g_hMDIClient, Message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK
MainWindow::childProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    return _instance->_childProc(hwnd, Message, wParam, lParam);
}

LRESULT
MainWindow::_childProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
      case WM_CREATE:
      {
         wchar_t szFileName[MAX_PATH];
         HWND hEdit;

         hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, (HMENU)IDC_CHILD_EDIT, _wc->hInstance(), NULL);

         SendMessage(hEdit, WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)), MAKELPARAM(TRUE, 0));

         GetWindowText(hwnd, szFileName, MAX_PATH);
         if(*szFileName != '[')
         {
            if(!LoadFile(hEdit, szFileName))
            {
                MessageBox(hwnd, L"Couldn't Load File.", L"Error.", MB_OK | MB_ICONEXCLAMATION);
                return -1; //cancel window creation
            }
         }
      }
      break;
      case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
                MoveWindow(GetDlgItem(hwnd, IDC_CHILD_EDIT), 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
      case WM_MDIACTIVATE:
      {
         HMENU hMenu, hFileMenu;
         BOOL EnableFlag;
         wchar_t szFileName[MAX_PATH];

         hMenu = GetMenu(handle());
         EnableFlag = hwnd == HWND(lParam) ? TRUE : FALSE;
         EnableMenuItem(hMenu, 1, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));
         EnableMenuItem(hMenu, 2, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));

         hFileMenu = GetSubMenu(hMenu, 0);
         EnableMenuItem(hFileMenu, CM_FILE_SAVE, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
         EnableMenuItem(hFileMenu, CM_FILE_SAVEAS, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));

         DrawMenuBar(handle());
#if 0
         SendMessage(g_tb->handle(), TB_ENABLEBUTTON, CM_FILE_SAVE, MAKELONG(EnableFlag, 0));
         SendMessage(g_tb->handle(), TB_ENABLEBUTTON, CM_EDIT_UNDO, MAKELONG(EnableFlag, 0));
         SendMessage(g_tb->handle(), TB_ENABLEBUTTON, CM_EDIT_CUT, MAKELONG(EnableFlag, 0));
         SendMessage(g_tb->handle(), TB_ENABLEBUTTON, CM_EDIT_COPY, MAKELONG(EnableFlag, 0));
         SendMessage(g_tb->handle(), TB_ENABLEBUTTON, CM_EDIT_PASTE, MAKELONG(EnableFlag, 0));
#endif
         GetWindowText(hwnd, szFileName, MAX_PATH);
         //SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM)(EnableFlag ? szFileName : L""));
      }
      break;
      case WM_SETFOCUS:
         SetFocus(GetDlgItem(hwnd, IDC_CHILD_EDIT));
      break;
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case CM_FILE_SAVE:
            {
               wchar_t szFileName[MAX_PATH];

               GetWindowText(hwnd, szFileName, MAX_PATH);
               if(*szFileName != '[')
               {
                  if(!SaveFile(GetDlgItem(hwnd, IDC_CHILD_EDIT), szFileName))
                  {
                     MessageBox(hwnd, L"Couldn't Save File.", L"Error.",
                        MB_OK | MB_ICONEXCLAMATION);
                     return 0;
                  }
               }
               else
               {
                    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(CM_FILE_SAVEAS, 0), 0);
               }
            }
            return 0;
            case CM_FILE_SAVEAS:
            {
               wchar_t szFileName[MAX_PATH];

               if (GetFileName(hwnd, szFileName, TRUE))
               {
                  if(!SaveFile(GetDlgItem(hwnd, IDC_CHILD_EDIT), szFileName))
                  {
                      throw "Couldn't save file";
                  }
                  else
                  {
                     SetWindowText(hwnd, szFileName);
                  }
               }
            }
            return 0;
            case CM_EDIT_UNDO:
                SendDlgItemMessage(hwnd, IDC_CHILD_EDIT, EM_UNDO, 0, 0);
                break;
            case CM_EDIT_CUT:
                SendDlgItemMessage(hwnd, IDC_CHILD_EDIT, WM_CUT, 0, 0);
                break;
            case CM_EDIT_COPY:
                SendDlgItemMessage(hwnd, IDC_CHILD_EDIT, WM_COPY, 0, 0);
                break;
            case CM_EDIT_PASTE:
                SendDlgItemMessage(hwnd, IDC_CHILD_EDIT, WM_PASTE, 0, 0);
                break;
         }
      return 0;
   }
   return DefMDIChildProc(hwnd, Message, wParam, lParam);
}


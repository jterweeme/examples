#include "mainwin.h"
#include "winclass.h"
#include "main.h"

#define IDC_MAIN_TEXT   1001

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(WinClass *wc) : _wc(wc)
{
    _instance = this;
}

void MainWindow::show(int nCmdShow)
{
    ::ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    ::UpdateWindow(_hwnd);
}

int MainWindow::create()
{
    _hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _wc->className(),
                L"File Editor Example Program", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                NULL, NULL, _wc->hInstance(), NULL);

    if (_hwnd == NULL)
        throw "Window creation failed!";

    return 0;
}

BOOL MainWindow::LoadFile(HWND hEdit, LPWSTR pszFileName)
{
    BOOL bSuccess = FALSE;
    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwFileSize = GetFileSize(hFile, NULL);

        if (dwFileSize != 0xFFFFFFFF)
        {
            LPSTR pszFileText2 = LPSTR(GlobalAlloc(GPTR, dwFileSize + 1));

            if (pszFileText2 != NULL)
            {
                DWORD dwRead;

                if (ReadFile(hFile, pszFileText2, dwFileSize, &dwRead, NULL))
                {
                    pszFileText2[dwFileSize] = 0; // Null terminator

                    if (SetWindowTextA(hEdit, pszFileText2))
                        bSuccess = TRUE; // It worked!
                }
                GlobalFree(pszFileText2);
            }
        }
        CloseHandle(hFile);
    }
    return bSuccess;
}

BOOL MainWindow::SaveFile(HWND hEdit, LPWSTR pszFileName)
{
    BOOL bSuccess = FALSE;

    HANDLE hFile = ::CreateFile(pszFileName, GENERIC_WRITE, 0, 0,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return bSuccess;

    DWORD dwTextLength = GetWindowTextLength(hEdit);

    // No need to bother if there's no text.
    if (dwTextLength > 0)
    {
        LPWSTR pszText = LPWSTR(GlobalAlloc(GPTR, dwTextLength + 1));

        if (pszText != NULL)
        {
            if (GetWindowText(hEdit, pszText, dwTextLength + 1))
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

BOOL MainWindow::DoFileOpenSave(HWND hwnd, BOOL bSave)
{
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH];
    ZeroMemory(&ofn, sizeof(ofn));
    szFileName[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";

    if (bSave)
    {
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn))
        {
            if (!SaveFile(GetDlgItem(hwnd, IDC_MAIN_TEXT), szFileName))
                throw "Save file failed";
        }
    }
    else
    {
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileName(&ofn))
        {
            if (!LoadFile(GetDlgItem(hwnd, IDC_MAIN_TEXT), szFileName))
            {
                ::MessageBoxA(hwnd, "Load of file failed.", "Error", MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }
        }
    }
    return TRUE;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            ::CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                hwnd, HMENU(IDC_MAIN_TEXT), _wc->hInstance(), NULL);

            ::SendDlgItemMessage(hwnd, IDC_MAIN_TEXT, WM_SETFONT,
                WPARAM(GetStockObject(DEFAULT_GUI_FONT)), MAKELPARAM(TRUE, 0));

            break;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
                MoveWindow(GetDlgItem(hwnd, IDC_MAIN_TEXT), 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);

            break;
        case WM_SETFOCUS:
            SetFocus(GetDlgItem(hwnd, IDC_MAIN_TEXT));
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case CM_FILE_OPEN:
                    DoFileOpenSave(hwnd, FALSE);
                    break;
                case CM_FILE_SAVEAS:
                    DoFileOpenSave(hwnd, TRUE);
                    break;
                case CM_FILE_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                case CM_ABOUT:
                    MessageBoxA(NULL, "File Editor for Windows !\n Using the Win32 API" , "About...", 0);
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


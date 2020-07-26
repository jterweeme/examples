#include "mainwin.h"
#include "winclass.h"
#include "resource.h"
#include <windowsx.h>
#include <commctrl.h>

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(WinClass *wc) : Window(wc)
{
    _instance = this;
}

void MainWindow::create()
{
    Window::create(TEXT("Property Sheet Application"));
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

INT_PTR CALLBACK About(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM)
{
    switch (uMessage)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch(wParam)
        {
        case IDOK:
            EndDialog(hWnd, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

LRESULT CALLBACK
ButtonsDlgProc(HWND hdlg, UINT uMessage, WPARAM, LPARAM lParam)
{
    LPNMHDR lpnmhdr;

    switch (uMessage)
    {
    // on any command notification, tell the property sheet to enable the Apply button
    case WM_COMMAND:
        PropSheet_Changed(GetParent(hdlg), hdlg);
        break;
    case WM_NOTIFY:
        lpnmhdr = (NMHDR FAR *)lParam;

        switch (lpnmhdr->code)
        {
        case PSN_APPLY:   //sent when OK or Apply button pressed
            break;
        case PSN_RESET:   //sent when Cancel button pressed
            break;
        case PSN_SETACTIVE:
            //this will be ignored if the property sheet is not a wizard
            PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

LRESULT CALLBACK
ComboDlgProc(HWND hdlg, UINT uMessage, WPARAM, LPARAM lParam)
{
    LPNMHDR lpnmhdr;

    switch (uMessage)
    {
    // on any command notification, tell the property sheet to enable the Apply button
    case WM_COMMAND:
        PropSheet_Changed(GetParent(hdlg), hdlg);
        break;
    case WM_NOTIFY:
        lpnmhdr = (NMHDR FAR *)lParam;

        switch (lpnmhdr->code)
        {
        case PSN_APPLY:   //sent when OK or Apply button pressed
            break;
        case PSN_RESET:   //sent when Cancel button pressed
            break;
        case PSN_SETACTIVE:
            //this will be ignored if the property sheet is not a wizard
            PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_BACK | PSWIZB_FINISH);
            return FALSE;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

void CALLBACK PropSheetCallback(HWND, UINT uMsg, LPARAM lParam)
{
    switch(uMsg)
    {
    //called before the dialog is created, hwndPropSheet = NULL, lParam points to dialog resource
    case PSCB_PRECREATE:
    {
        LPDLGTEMPLATE lpTemplate = LPDLGTEMPLATE(lParam);

        if (!(lpTemplate->style & WS_SYSMENU))
        {
            lpTemplate->style |= WS_SYSMENU;
        }
    }
        break;
    case PSCB_INITIALIZED:   //called after the dialog is created
        break;
    }
}

INT_PTR DoPropSheet(HINSTANCE hInstance, HWND hwndOwner, LPCTSTR caption, ULONG flags)
{
    PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;

    //Fill out the PROPSHEETPAGE data structure for the Background Color sheet
    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags     = PSP_USETITLE;
    psp[0].hInstance   = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_BUTTONS);
    psp[0].pszIcon     = NULL;
    psp[0].pfnDlgProc  = ButtonsDlgProc;
    psp[0].pszTitle    = TEXT("Buttons");
    psp[0].lParam      = 0;
    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags     = PSP_USETITLE;
    psp[1].hInstance   = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_COMBOBOXES);
    psp[1].pszIcon     = NULL;
    psp[1].pfnDlgProc  = ComboDlgProc;
    psp[1].pszTitle    = TEXT("Combo Boxes");
    psp[1].lParam      = 0;
    psh.dwSize         = sizeof(PROPSHEETHEADER);
    psh.dwFlags        = flags;
    psh.hwndParent     = hwndOwner;
    psh.hInstance      = hInstance;
    psh.pszIcon        = MAKEINTRESOURCE(IDI_BACKCOLOR);
    psh.pszCaption     = caption;
    psh.nPages         = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp           = LPCPROPSHEETPAGE(&psp);
    psh.pfnCallback    = PFNPROPSHEETCALLBACK(PropSheetCallback);

    //And finally display the modal property sheet
    return ::PropertySheet(&psh);
}

INT_PTR DoModalPropSheet(HINSTANCE hInstance, HWND hwndOwner)
{
    return DoPropSheet(hInstance, hwndOwner, TEXT("Modal Property Sheet"),
                       PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK);
}

INT_PTR DoWizardPropSheet(HINSTANCE hInstance, HWND hwndOwner)
{
    return DoPropSheet(hInstance, hwndOwner, TEXT("Wizard Propery Sheet"),
                       PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_USEICONID | PSH_USECALLBACK);
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        break;
    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDM_MODAL:
            DoModalPropSheet(hInstance(), hwnd);
            break;
        case IDM_WIZARD:
            DoWizardPropSheet(hInstance(), hwnd);
            break;
        case IDM_EXIT:
            ::PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_ABOUT:
            DialogBox(hInstance(), MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hwnd, About);
            break;
        }
        return TRUE;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


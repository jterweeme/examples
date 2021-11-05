// DataBands.cpp : Defines the entry point for the application.
//

#include "DataBands.h"
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#define MAX_LOADSTRING 100
#define MIN_BAND_WIDTH  50

//this is an easy & flexible way to set a loop index 
//for iterating an array of structures
#define dim(x) (sizeof(x) / sizeof(x[0]))

struct CmdBarParms {                             
     UINT      uiID; 
     UINT      uiCaption;
     long      lStyles;
     int            iStringIndexBase;
     int            iNumStrings;
};


// Setup information for the controls that implement
//the fields of the forms
const struct CmdBarParms structCmdBarInit[] = 
{
      IDC_NAME, IDS_NAME, ES_AUTOHSCROLL,            NULL, 0,
      IDC_BADGE, IDS_BADGE, ES_AUTOHSCROLL,            NULL,            0,
      IDC_DESC, IDS_DESC, ES_MULTILINE | ES_AUTOVSCROLL, NULL, 0,
      IDC_CAT, IDS_CAT, CBS_DROPDOWNLIST, IDS_CAT1,    2,
      IDC_MODE_TRAVEL, IDS_MODE_TRAVEL,CBS_DROPDOWNLIST, IDS_MODE_TRAVEL1, 3,
      IDC_WHERE_SEEN, IDS_WHERE_SEEN, CBS_DROPDOWNLIST, IDS_WHERE1, 3,
      IDC_PHYS_EVIDENCE, IDS_PHYS_EVIDENCE, CBS_DROPDOWNLIST, IDS_PHYS_EVIDENCE1, 3,
      IDC_OFFICE, IDS_OFFICE, CBS_DROPDOWNLIST, IDS_OFFICE1, 3,
      IDC_ABDUCTIONS, IDS_ABDUCTIONS, CBS_DROPDOWNLIST, IDS_ABDUCTIONS1, 3,
      IDC_FRIENDLY, IDS_FRIENDLY, CBS_DROPDOWNLIST, IDS_FRIENDLY1,   3,
      IDC_TALKATIVE, IDS_TALKATIVE,      CBS_DROPDOWNLIST, IDS_TALKATIVE1, 3,
      IDC_APPEARANCE, IDS_APPEARANCE,   CBS_DROPDOWN,     IDS_APPEARANCE1, 5
};

// Global Variables:
static HINSTANCE hInst;                  // The current instance
static HWND hwndCB;            // The command bar handle
static int nBands;              // #bands in the CommandBands ctrl

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LPARAM  CreateDataBands (HWND, UINT, WPARAM, LPARAM); 
LRESULT CALLBACK About (HWND, UINT, WPARAM, LPARAM);

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc           = (WNDPROC) WndProc;
    wc.cbClsExtra               = 0;
    wc.cbWndExtra             = 0;
    wc.hInstance                  = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DATABANDS));
    wc.hCursor                      = 0;
    wc.hbrBackground          = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName         = 0;
    wc.lpszClassName          = szWindowClass;

    return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
      TCHAR      szTitle[MAX_LOADSTRING];                      // The title bar text
      TCHAR      szWindowClass[MAX_LOADSTRING];       // The window class name

// Store instance handle in our global variable
      hInst = hInstance;            

      // Initialize global strings
      LoadString(hInstance, IDC_DATABANDS, szWindowClass, MAX_LOADSTRING);
      MyRegisterClass(hInstance, szWindowClass);
      LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

      hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                               NULL, NULL, hInstance, NULL);

//if no window, fail and bail
      if (!hWnd)
		  throw TEXT("Cannot create window");

//it’s all good.  Show the window and command bar menu
      ShowWindow(hWnd, nCmdShow);
      UpdateWindow(hWnd);

      if (hwndCB)
            CommandBar_Show(hwndCB, TRUE);

      return TRUE;
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;
    INITCOMMONCONTROLSEX icex; //this struct is used to init the 
                                                                     //common controls dll

      //windows ce requires explicit init of the common controls
    icex.dwSize = sizeof (INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_COOL_CLASSES;
    InitCommonControlsEx (&icex);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) 
    {
        return FALSE;
    }

      hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_DATABANDS);

      // Main message loop:
      while (GetMessage(&msg, NULL, 0, 0)) 
      {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
            {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
            }
      }

      return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;

    switch (message) 
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
                  wmEvent = HIWORD(wParam); 
                  // process the menu selections:
                  switch (wmId)
                  {
                        case IDM_HELP_ABOUT:
                           DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                           break;

                        case IDM_FILE_EXIT:
                           DestroyWindow(hWnd);
                           break;

                        default:
                           return DefWindowProc(hWnd, message, wParam, lParam);
                  }// end switch ID
                  break;

            case WM_CREATE:
                  //build the bands that replace the Win32 dialog box form
                  CreateDataBands(hWnd, message, wParam, lParam);
                  break;

            case WM_DESTROY:
                  DestroyWindow (hwndCB);
                  PostQuitMessage(0);
                  return 0;

            default:
                  return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Message handler for the About box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
      RECT rt, rt1;
      int DlgWidth, DlgHeight;      // dialog width and height in pixel units
      int NewPosX, NewPosY;

      switch (message)
      {
            case WM_INITDIALOG:
                  // trying to center the About dialog
                  if (GetWindowRect(hDlg, &rt1)) {
                        GetClientRect(GetParent(hDlg), &rt);
                        DlgWidth      = rt1.right - rt1.left;
                        DlgHeight      = rt1.bottom - rt1.top ;
                        NewPosX            = (rt.right - rt.left - DlgWidth)/2;
                        NewPosY            = (rt.bottom - rt.top - DlgHeight)/2;
                        
                        // if the About box is larger than the physical screen 
                        if (NewPosX < 0) NewPosX = 0;
                        if (NewPosY < 0) NewPosY = 0;
                        SetWindowPos(hDlg, 0, NewPosX, NewPosY,
                              0, 0, SWP_NOZORDER | SWP_NOSIZE);
                  }
                  return TRUE;

            case WM_COMMAND:
                  if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
                  {
                        EndDialog(hDlg, LOWORD(wParam));
                        return TRUE;
                  }
                  break;
      }
    return FALSE;
}

LPARAM CreateDataBands(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBITMAP hBmp;
    LPREBARBANDINFO prbi;
    HWND hwndBand, hwndCombo;
    int i, j, rc = 0, nBandsPerRow;
    TCHAR tszCaptionBuff[34];
    TCHAR tszPlatType[256];



    //initialize nBands = # ctrls + the main menu band
    nBands = dim( structCmdBarInit ) + 1;
    ::hwndCB = CommandBands_Create(hInst, hWnd, IDC_BAND_BASE_ID, RBS_BANDBORDERS | RBS_AUTOSIZE | RBS_FIXEDORDER, NULL);

    // Load bitmap used as background for command bar.
    hBmp = LoadBitmap (hInst, TEXT ("CmdBarBack"));

      // Allocate space for the REBARBANDINFO array
      // Need one entry per ctl band ( nBands ) plus the menu
      prbi = (LPREBARBANDINFO)LocalAlloc(LPTR, (sizeof (REBARBANDINFO) * nBands) ); 

//always test returns if you attempt to allocate memory
    if (!prbi) {
            MessageBox( hWnd, TEXT("LocalAlloc Failed"), TEXT("Oops!"), MB_OK); 
        return 0;
    }

      // Initialize common REBARBANDINFO structure fields.
    for (i = 0; i < nBands; i++)
    {  
        prbi[i].cbSize = sizeof (REBARBANDINFO);
        prbi[i].fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_BACKGROUND |
                                  RBBIM_STYLE |
                                  RBBIM_IDEALSIZE ; 
        prbi[i].wID    = IDC_BAND_BASE_ID+i;
        prbi[i].hbmBack = hBmp;
    }

    // Initialize REBARBANDINFO structure for Menu band
    prbi[0].cx = GetSystemMetrics(SM_CXSCREEN ) / 4;
    prbi[0].fStyle = prbi[1].fStyle = RBBS_NOGRIPPER |
                                                        RBBS_BREAK;

      //Set bands per row based on platform type
      memset( &tszPlatType, 0x0, sizeof(tszPlatType));
      rc = SystemParametersInfo( SPI_GETPLATFORMTYPE, 
                                                      sizeof(tszPlatType),
                                                     &tszPlatType, 0);
      if( lstrcmp( tszPlatType, TEXT("Jupiter") ) == 0 )
      { nBandsPerRow = 4;}
      else if( lstrcmp( tszPlatType, TEXT("HPC") ) == 0 )
      { nBandsPerRow = 3;}
      else if( lstrcmp( tszPlatType, TEXT("Palm PC") ) == 0 )
      { nBandsPerRow = 1;}
 
      //Initialize data entry band attributes
      for (i = 1; i < nBands; i++) 
      {
            //  Common Combobox ctrl band attributes
            prbi[i].fMask |= RBBIM_IDEALSIZE;
            prbi[i].cx = (GetSystemMetrics(SM_CXSCREEN ) - 20 ) / nBandsPerRow ;
            prbi[i].cxIdeal = (GetSystemMetrics(SM_CXSCREEN ) - 20 ) / nBandsPerRow ;

            //Set style for line break at the end of a row
            if(((i - 1 ) % nBandsPerRow ) == 0 ) 
            {
                  prbi[i].fStyle |= RBBS_BREAK | RBBS_NOGRIPPER;
            }
            else
            {
                  prbi[i].fStyle |= RBBS_NOGRIPPER;
            }
                   
      }

    // Add the bands-- we've reserved real estate 
      //  for our dlg controls. 
    CommandBands_AddBands (hwndCB, hInst, nBands, prbi); 

      //Move past the menu bar and set the control behaviors
      for (i = 1; i < nBands; i++) 
      {
            hwndBand = CommandBands_GetCommandBar (hwndCB,i);
            CommandBar_InsertComboBox (hwndBand, hInst, 
                                                                   prbi[i].cx - 6, 
                                                                   structCmdBarInit[i].lStyles, 
                                                                   IDC_COMBO_BASE_ID + (i - 1), 0);

            //get a handle to the combo in this band
            hwndCombo = GetDlgItem(hwndBand,IDC_COMBO_BASE_ID + (i - 1));
            //get the caption string
            LoadString(hInst, structCmdBarInit[i - 1].uiCaption, 
                              (LPTSTR)&tszCaptionBuff,sizeof(tszCaptionBuff));

            //insert the label string
            SendMessage (hwndCombo, CB_INSERTSTRING, 0, 
                              (long)&tszCaptionBuff);
            //highlight the caption
            SendMessage (hwndCombo, CB_SETCURSEL, 0, 0);
            //does this combo have a string list?
            if(structCmdBarInit[i - 1].iNumStrings)
            {
                  //if so, insert them in the combo
                  for( j = 0; j < structCmdBarInit[i - 1].iNumStrings; j ++ )
                  {
                        LoadString(hInst, structCmdBarInit[i - 1].iStringIndexBase + j, 
                              (LPTSTR)&tszCaptionBuff,sizeof(tszCaptionBuff));
                        //add strings to the end of the list
                        SendMessage (hwndCombo, CB_INSERTSTRING, -1, 
                              (long)&tszCaptionBuff);
                  } //end for(iNumStrings)
            }//end if(iNumStrings)

      }
      
    // Add menu to band 0 --wahoo
    hwndBand = CommandBands_GetCommandBar (hwndCB, 0);
    CommandBar_InsertMenubar(hwndBand, hInst, IDM_MENU, 0);
    CommandBands_AddAdornments(hwndCB, hInst, 0, 0);
    LocalFree(prbi);
    return 0;
}


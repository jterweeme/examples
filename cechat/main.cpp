// CeChat - A Windows CE communication demo
//
// Written for the book Programming Windows CE
// Copyright (C) 2003 Douglas Boling
// 2020 Jasper ter Weeme

#include "resource.h"
#include <windows.h>				 // For all that Windows stuff
#include <commctrl.h>				 // Command bar includes

#define dim(x) (sizeof(x) / sizeof(x[0]))


struct decodeUINT {                             // Structure associates
    UINT Code;                                  // messages
                                                // with a function.
    LRESULT (*Fxn)(HWND, UINT, WPARAM, LPARAM);
};
struct decodeCMD {                              // Structure associates
    UINT Code;                                  // menu IDs with a
    LRESULT (*Fxn)(HWND, WORD, HWND, WORD);     // function.
};
//----------------------------------------------------------------------
// Generic defines used by application


#define TEXTSIZE 256
//----------------------------------------------------------------------
// Function prototypes
//
DWORD WINAPI ReadThread (PVOID pArg);
DWORD WINAPI SendThread (PVOID pArg);
HANDLE InitCommunication (HWND, LPTSTR);
int FillComComboBox (HWND);

HWND InitInstance (HINSTANCE, LPWSTR, int);
int TermInstance (HINSTANCE, int);

// Window procedures
LRESULT CALLBACK MainWndProc (HWND, UINT, WPARAM, LPARAM);

// Message handlers
LRESULT DoCreateMain (HWND, UINT, WPARAM, LPARAM);
LRESULT DoSizeMain (HWND, UINT, WPARAM, LPARAM);
LRESULT DoSetFocusMain (HWND, UINT, WPARAM, LPARAM);
LRESULT DoPocketPCShell (HWND, UINT, WPARAM, LPARAM);
LRESULT DoCommandMain (HWND, UINT, WPARAM, LPARAM);
LRESULT DoDestroyMain (HWND, UINT, WPARAM, LPARAM);
// Command functions
LPARAM DoMainCommandExit (HWND, WORD, HWND, WORD);
LPARAM DoMainCommandComPort (HWND, WORD, HWND, WORD);
LPARAM DoMainCommandSendText (HWND, WORD, HWND, WORD);
LPARAM DoMainCommandAbout (HWND, WORD, HWND, WORD);

// Dialog procedures
BOOL CALLBACK AboutDlgProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EditAlbumDlgProc (HWND, UINT, WPARAM, LPARAM);


#if defined(WIN32_PLATFORM_PSPC)
#include <aygshell.h>				 // Add Pocket PC includes.
#pragma comment( lib, "aygshell" )	 // Link Pocket PC lib for menu bar.
#endif
//----------------------------------------------------------------------
// Global data
//
const TCHAR szAppName[] = TEXT ("CeChat");
HINSTANCE hInst;					 // Program instance handle.

BOOL fContinue = TRUE;
HANDLE hComPort = INVALID_HANDLE_VALUE;
int nSpeed = CBR_19200;
int nLastDev = -1;

#if defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE >= 300)
SHACTIVATEINFO sai;
#endif

HANDLE g_hSendEvent = INVALID_HANDLE_VALUE;
HANDLE hReadThread = INVALID_HANDLE_VALUE;

// Message dispatch table for MainWindowProc
const struct decodeUINT MainMessages[] = {
	WM_CREATE, DoCreateMain,
	WM_SIZE, DoSizeMain,
	WM_COMMAND, DoCommandMain,
	WM_SETTINGCHANGE, DoPocketPCShell,
	WM_ACTIVATE, DoPocketPCShell,
	WM_SETFOCUS, DoSetFocusMain,
	WM_DESTROY, DoDestroyMain,
};
// Command Message dispatch for MainWindowProc
const struct decodeCMD MainCommandItems[] = {
	IDC_COMPORT, DoMainCommandComPort,
	ID_SENDBTN, DoMainCommandSendText,
	IDM_EXIT, DoMainCommandExit,
	IDM_ABOUT, DoMainCommandAbout,
};
//======================================================================
// Program entry point
//
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPWSTR lpCmdLine, int nCmdShow) {
	HWND hwndMain;
	HACCEL hAccel;
	MSG msg;
	int rc = 0;

	// Initialize this instance.
	hwndMain = InitInstance (hInstance, lpCmdLine, nCmdShow);
	if (hwndMain == 0)
		return 0x10;

	// Load accelerator table.
	hAccel = LoadAccelerators (hInst, MAKEINTRESOURCE (ID_ACCEL));

	// Application message loop
	while (GetMessage (&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator (hwndMain, hAccel, &msg)) {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}
	// Instance cleanup
	return TermInstance (hInstance, msg.wParam);
}

HWND InitInstance (HINSTANCE hInstance, LPWSTR, int nCmdShow)
{
	HWND hWnd;
	HANDLE hThread;
	WNDCLASS wc;
	INITCOMMONCONTROLSEX icex;

	// Save program instance handle in global variable.
	hInst = hInstance;

#if defined(WIN32_PLATFORM_PSPC)
	// If Pocket PC, allow only one instance of the application.
	HWND hWnd = FindWindow (szAppName, NULL);
	if (hWnd) {
		SetForegroundWindow ((HWND)(((DWORD)hWnd) | 0x01));    
		return 0;
	}
#endif
// Register application main window class.
	wc.style = 0;							  // Window style
	wc.lpfnWndProc = MainWndProc;			  // Callback function
	wc.cbClsExtra = 0;						  // Extra class data
	wc.cbWndExtra = 0;						  // Extra window data
	wc.hInstance = hInstance;				  // Owner handle
	wc.hIcon = NULL;						  // Application icon
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);// Default cursor
	wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wc.lpszMenuName =  NULL;				  // Menu name
	wc.lpszClassName = szAppName;			  // Window class name

	if (RegisterClass (&wc) == 0) return 0;

	// Load the command bar common control class.
	icex.dwSize = sizeof (INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx (&icex);

	// Create unnamed auto-reset event initially false.
	g_hSendEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

	// Create main window.
	hWnd = CreateWindow (szAppName, TEXT ("CeChat"),
						 WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
						 CW_USEDEFAULT, CW_USEDEFAULT, NULL,
						 NULL, hInstance, NULL);
	// Return fail code if window not created.
	if (!IsWindow (hWnd)) return 0;

	// Create write thread. Read thread created when port opened.
	hThread = CreateThread (NULL, 0, SendThread, hWnd, 0, NULL);
	if (hThread)
		CloseHandle (hThread);
	else {
		DestroyWindow (hWnd);
		return 0;
	}
	// Standard show and update calls
	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);
	return hWnd;
}
//----------------------------------------------------------------------
// TermInstance - Program cleanup
//
int TermInstance (HINSTANCE hInstance, int nDefRC) {
	HANDLE hPort = hComPort;

	fContinue = FALSE;

	hComPort = INVALID_HANDLE_VALUE;
	if (hPort != INVALID_HANDLE_VALUE)
		CloseHandle (hPort);

	if (g_hSendEvent != INVALID_HANDLE_VALUE) {
		PulseEvent (g_hSendEvent);
		Sleep(100);
		CloseHandle (g_hSendEvent);
	}
	return nDefRC;
}
//======================================================================
// Message handling procedures for MainWindow
//----------------------------------------------------------------------
// MainWndProc - Callback function for application window
//
LRESULT CALLBACK MainWndProc (HWND hWnd, UINT wMsg, WPARAM wParam,
							  LPARAM lParam) {
	int i;
	//
	// Search message list to see if we need to handle this
	// message.  If in list, call procedure.
	//
	for (i = 0; i < dim(MainMessages); i++) {
		if (wMsg == MainMessages[i].Code)
			 return (*MainMessages[i].Fxn)(hWnd, wMsg, wParam, lParam);
	}
	return DefWindowProc (hWnd, wMsg, wParam, lParam);
}
//----------------------------------------------------------------------
// DoCreateMain - Process WM_CREATE message for window.
//
LRESULT DoCreateMain (HWND hWnd, UINT wMsg, WPARAM wParam,
					  LPARAM lParam) {
	HWND hwndCB, hC1, hC2, hC3;
	int  i;
	TCHAR szFirstDev[32];
	LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;

#if defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE >= 300)
	memset (&sai, 0, sizeof (sai));
	sai.cbSize = sizeof (sai);
	{
	SHMENUBARINFO mbi;						// For Pocket PC, create
	memset(&mbi, 0, sizeof(SHMENUBARINFO)); // menu bar so that we
	mbi.cbSize = sizeof(SHMENUBARINFO); 	// have a sip button.
	mbi.hwndParent = hWnd;
	mbi.dwFlags = SHCMBF_EMPTYBAR;
	SHCreateMenuBar(&mbi);
	SetWindowPos (hWnd, 0, 0, 0, lpcs->cx, lpcs->cy-26, 
				  SWP_NOZORDER | SWP_NOMOVE);
	}
#endif	  
   
   // Create a command bar.
	hwndCB = CommandBar_Create (hInst, hWnd, IDC_CMDBAR);
	CommandBar_InsertMenubar (hwndCB, hInst, ID_MENU, 0);

	// Insert the COM port combo box.
	CommandBar_InsertComboBox (hwndCB, hInst, 140, CBS_DROPDOWNLIST,
							   IDC_COMPORT, 1);
	FillComComboBox (hWnd);

	// Add exit button to command bar.
	CommandBar_AddAdornments (hwndCB, 0, 0);

	// Create child windows. They will be positioned in WM_SIZE.
	// Create receive text window.
	hC1 = CreateWindowEx (WS_EX_CLIENTEDGE, TEXT ("edit"),
						  TEXT (""), WS_VISIBLE | WS_CHILD |
						  WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL |
						  ES_READONLY, 0, 0, 10, 10, hWnd,
						  (HMENU)ID_RCVTEXT, hInst, NULL);
	// Create send text window.
	hC2 = CreateWindowEx (WS_EX_CLIENTEDGE, TEXT ("edit"),
						  TEXT (""), WS_VISIBLE | WS_CHILD,
						  0, 0, 10, 10,  hWnd, (HMENU)ID_SENDTEXT, 
							 hInst, NULL);
	// Create send text window.
	hC3 = CreateWindowEx (WS_EX_CLIENTEDGE, TEXT ("button"),
						  TEXT ("&Send"), WS_VISIBLE | WS_CHILD |
						  BS_DEFPUSHBUTTON, 0, 0, 10, 10,
						  hWnd, (HMENU)ID_SENDBTN, hInst, NULL);
	// Destroy frame if window not created.
	if (!IsWindow (hC1) || !IsWindow (hC2) || !IsWindow (hC3)) {
		DestroyWindow (hWnd);
		return 0;
	}
	// Open a COM port.
	for (i = 0; i < 10; i++) {
		if (SendDlgItemMessage (hwndCB, IDC_COMPORT, CB_GETLBTEXT, i,
							(LPARAM)szFirstDev) == CB_ERR)
			break;
		if (InitCommunication (hWnd, szFirstDev) !=
			INVALID_HANDLE_VALUE) {
			SendDlgItemMessage (hwndCB, IDC_COMPORT, CB_SETCURSEL, i,
								(LPARAM)szFirstDev);
			break;
		}
	}
	return 0;
}
//----------------------------------------------------------------------
// DoSizeMain - Process WM_SIZE message for window.
//
LRESULT DoSizeMain (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam){
	RECT rect;

	// Adjust the size of the client rect to take into account
	// the command bar height.
	GetClientRect (hWnd, &rect);
	rect.top += CommandBar_Height (GetDlgItem (hWnd, IDC_CMDBAR));

	SetWindowPos (GetDlgItem (hWnd, ID_RCVTEXT), NULL, rect.left,
				  rect.top, (rect.right - rect.left),
				  rect.bottom - rect.top - 25, SWP_NOZORDER);
	SetWindowPos (GetDlgItem (hWnd, ID_SENDTEXT), NULL, rect.left,
				  rect.bottom - 25, (rect.right - rect.left) - 50,
				  25, SWP_NOZORDER);
	SetWindowPos (GetDlgItem (hWnd, ID_SENDBTN), NULL,
				  (rect.right - rect.left) - 50, rect.bottom - 25,
				  50, 25, SWP_NOZORDER);
	return 0;
}
//----------------------------------------------------------------------
// DoPocketPCShell - Process Pocket PC required messages.
//
LRESULT DoPocketPCShell (HWND hWnd, UINT wMsg, WPARAM wParam,
						 LPARAM lParam) {
#if defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE >= 300)
	if (wMsg == WM_SETTINGCHANGE) 
		return SHHandleWMSettingChange(hWnd, wParam, lParam, &sai);
	if (wMsg == WM_ACTIVATE) 
		return SHHandleWMActivate(hWnd, wParam, lParam, &sai, 0);
#endif 
	return 0;
}
//----------------------------------------------------------------------
// DoFocusMain - Process WM_SETFOCUS message for window.
//
LRESULT DoSetFocusMain (HWND hWnd, UINT wMsg, WPARAM wParam,
						LPARAM lParam) {
	SetFocus (GetDlgItem (hWnd, ID_SENDTEXT));
	return 0;
}
//----------------------------------------------------------------------
// DoCommandMain - Process WM_COMMAND message for window.
//
LRESULT DoCommandMain (HWND hWnd, UINT wMsg, WPARAM wParam,
					   LPARAM lParam) {
	WORD	idItem, wNotifyCode;
	HWND hwndCtl;
	int  i;

	// Parse the parameters.
	idItem = (WORD) LOWORD (wParam);
	wNotifyCode = (WORD) HIWORD (wParam);
	hwndCtl = (HWND) lParam;

	// Call routine to handle control message.
	for (i = 0; i < dim(MainCommandItems); i++) {
		if (idItem == MainCommandItems[i].Code)
			 return (*MainCommandItems[i].Fxn)(hWnd, idItem, hwndCtl,
											   wNotifyCode);
	}
	return 0;
}

LRESULT DoDestroyMain(HWND, UINT, WPARAM, LPARAM)
{
	PostQuitMessage (0);
	return 0;
}

LPARAM DoMainCommandExit (HWND hWnd, WORD idItem, HWND hwndCtl,
						  WORD wNotifyCode) {
	SendMessage (hWnd, WM_CLOSE, 0, 0);
	return 0;
}
//----------------------------------------------------------------------
// DoMainCommandComPort - Process the COM port combo box commands.
//
LPARAM DoMainCommandComPort (HWND hWnd, WORD idItem, HWND hwndCtl,
							 WORD wNotifyCode) {
	int i;
	TCHAR szDev[32];

	if (wNotifyCode == CBN_SELCHANGE) {
		i = SendMessage (hwndCtl, CB_GETCURSEL, 0, 0);
		if (i != nLastDev) {
			SendMessage (hwndCtl, CB_GETLBTEXT, i, (LPARAM)szDev);
			InitCommunication (hWnd, szDev);
			SetFocus (GetDlgItem (hWnd, ID_SENDTEXT));
		}
	}
	return 0;
}
//----------------------------------------------------------------------
// DoMainCommandSendText - Process the Send text button.
//
LPARAM DoMainCommandSendText (HWND hWnd, WORD idItem, HWND hwndCtl,
							  WORD wNotifyCode) {

	// Set event so that sender thread will send the text.
	SetEvent (g_hSendEvent);
	SetFocus (GetDlgItem (hWnd, ID_SENDTEXT));
	return 0;
}
//----------------------------------------------------------------------
// DoMainCommandAbout - Process the Help | About menu command.
//
LPARAM DoMainCommandAbout(HWND hWnd, WORD idItem, HWND hwndCtl,
						  WORD wNotifyCode) {
	// Use DialogBox to create modal dialog.
	DialogBox (hInst, TEXT ("aboutbox"), hWnd, AboutDlgProc);
	return 0;
}
//======================================================================
// About Dialog procedure
//
BOOL CALLBACK AboutDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam,
							LPARAM lParam) {
	switch (wMsg) {
		case WM_COMMAND:
			switch (LOWORD (wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog (hWnd, 0);
					return TRUE;
}
		break;
	}
	return FALSE;
}
//----------------------------------------------------------------------
// FillComComboBox - Fills the COM port combo box
//
int FillComComboBox (HWND hWnd) {
	int rc;
	WIN32_FIND_DATA fd;
	HANDLE hFind;

	hFind = FindFirstFileEx (TEXT ("COM?:"), FindExInfoStandard, &fd, 
							 FindExSearchLimitToDevices, NULL, 0);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			SendDlgItemMessage (GetDlgItem (hWnd, IDC_CMDBAR),
								IDC_COMPORT, CB_INSERTSTRING,
								-1, (LPARAM)fd.cFileName);
			rc = FindNextFile (hFind, &fd);
		} while (rc);

		rc = FindClose (hFind);
	}
	SendDlgItemMessage (GetDlgItem (hWnd, IDC_CMDBAR), IDC_COMPORT,
						CB_SETCURSEL, 0, 0);
	return 0;
}
//----------------------------------------------------------------------
// InitCommunication - Open and initialize selected COM port.
//
HANDLE InitCommunication (HWND hWnd, LPTSTR pszDevName) {
	DCB dcb;
	TCHAR szDbg[128];
	COMMTIMEOUTS cto;
	HANDLE hLocal;
	DWORD dwTStat;
	hLocal = hComPort;
	hComPort = INVALID_HANDLE_VALUE;

	if (hLocal != INVALID_HANDLE_VALUE)
		CloseHandle (hLocal);  // This causes WaitCommEvent to return.

	hLocal = CreateFile (pszDevName, GENERIC_READ | GENERIC_WRITE,
						 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hLocal != INVALID_HANDLE_VALUE) {
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

		wsprintf (szDbg, TEXT ("Port %s opened\r\n"), pszDevName);
		SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL, 0,
							(LPARAM)szDbg);

		// Start read thread if not already started.
		hComPort = hLocal;
		if (!GetExitCodeThread (hReadThread, &dwTStat) ||
			(dwTStat != STILL_ACTIVE)) {
			hReadThread = CreateThread (NULL, 0, ReadThread, hWnd,
										0, &dwTStat);
			if (hReadThread)
				CloseHandle (hReadThread);
		}
	} else {
		wsprintf (szDbg, TEXT ("Couldn\'t open port %s. rc=%d\r\n"),
				  pszDevName, GetLastError());
		SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL,
							0, (LPARAM)szDbg);
	}
	return hComPort;
}
//======================================================================
// SendThread - Sends characters to the serial port
//
DWORD WINAPI SendThread (PVOID pArg) {
	HWND hWnd, hwndSText;
	int rc;
	DWORD cBytes;
	TCHAR szText[TEXTSIZE];

	hWnd = (HWND)pArg;
	hwndSText = GetDlgItem (hWnd, ID_SENDTEXT);
	while (1) {
		rc = WaitForSingleObject (g_hSendEvent, INFINITE);
		if (rc == WAIT_OBJECT_0) {
			if (!fContinue)
				break;
			// Disable send button while sending.
			EnableWindow (GetDlgItem (hWnd, ID_SENDBTN), FALSE);
			GetWindowText (hwndSText, szText, dim(szText));
			lstrcat (szText, TEXT ("\r\n"));
			rc = WriteFile (hComPort, szText, 
							lstrlen (szText)*sizeof (TCHAR),&cBytes, 0);
			if (rc) {
				// Copy sent text to output window. 
				SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL, 0,
									(LPARAM)TEXT (" >"));
				SetWindowText (hwndSText, TEXT (""));  // Clear text box
			} else {
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
			SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL, 0,
								(LPARAM)szText);
			EnableWindow (GetDlgItem (hWnd, ID_SENDBTN), TRUE);
		} else
			break;
	}
	return 0;
}
//======================================================================
// ReadThread - Receives characters from the serial port
//
DWORD WINAPI ReadThread (PVOID pArg) {
	HWND hWnd;
	DWORD cBytes, i;
	BYTE szText[TEXTSIZE], *pPtr;
	TCHAR tch;

	hWnd = (HWND)pArg;
	while (fContinue) {
		tch = 0;
		pPtr = szText;
		for (i = 0; i < sizeof (szText)-sizeof (TCHAR); i++) {

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
		if (i % 2) {
			pPtr = szText;
			while (*pPtr || *(pPtr+1)) {
				*pPtr = *(pPtr+1);
				pPtr++;
			}
			*pPtr = 0;
		}
		SendDlgItemMessage (hWnd, ID_RCVTEXT, EM_REPLACESEL, 0,
							(LPARAM)szText);
	}
	return 0;
}


#ifndef SAVEOPEN_H
#define SAVEOPEN_H
#include <windows.h>

int DoFileOpenDlg(HINSTANCE hInst, HWND hWnd, LPCSTR szFileSpecIn,
            LPCSTR szDefExtIn, WORD wFileAttrIn, char *szFileNameOut,
                       POFSTRUCT pof);

int DoWildFileOpenDlg (HINSTANCE hInst, HWND hWnd, LPCSTR szFileSpecIn,
                       LPCSTR szDefExtIn, WORD wFileAttrIn, char *szFileNameOut,
                       POFSTRUCT pof);
int DoFileSaveDlg (HINSTANCE hInst, HWND hWnd, LPCSTR szFileSpecIn,
                       LPCSTR szDefExtIn, int *pwStatusOut, char *szFileNameOut,
                       POFSTRUCT pof);

#define FILESAVE 270
#define FILEOPEN 271
#define WILDFILEOPEN 272
#endif


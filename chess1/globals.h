#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>

extern DWORD clrBackGround;
extern DWORD clrBlackSquare;
extern DWORD clrWhiteSquare;
extern DWORD clrBlackPiece;
extern DWORD clrWhitePiece;
extern DWORD clrText;
extern HINSTANCE hInst;
extern HACCEL hAccel;
extern char mvstr[4][6];
extern long evrate;
extern HWND hComputerColor;
extern HWND hComputerMove;
extern HWND hWhosTurn;
extern HWND hClockComputer;
extern HWND hClockHuman;
extern HWND hMsgComputer;
extern HWND hMsgHuman;

#endif


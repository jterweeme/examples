/* MemDcUsage.h ***************************************************************
Author     Paul Watt
Date:      7/3/2011 11:54:11 AM
Purpose:   Demonstration code for the usage or Win32 Memory DCs.
Copyright 2011 Paul Watt
******************************************************************************/
#ifndef MEMDCUSAGE_H_INCLUDED
#define MEMDCUSAGE_H_INCLUDED
#include <windows.h>

int  AdjustAnimation(HWND hWnd, int offset);
void EnableBackBuffer(bool isEnable);
void FlushBackBuffer();
UINT GetFrameRateDelay();
bool IsBackBufferEnabled();
void Init(const UINT width, const UINT height);
void PaintAnimation(HWND hWnd, HDC hDc);
void StepAnimation(HWND hWnd);
void Term();
#endif

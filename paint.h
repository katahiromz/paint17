#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cstdio>
#include <string>
#include <strsafe.h>
#include "resource.h"

LPTSTR LoadStringDx(INT nID);

LRESULT CALLBACK
CanvasWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HBITMAP DoCreate24BppBitmap(INT cx, INT cy);
HBITMAP DoGetSubImage(HBITMAP hbm, const RECT *prc);
void DoPutSubImage(HBITMAP hbm, const RECT *prc, HBITMAP hbmSubImage);
HBITMAP LoadBitmapFromFile(LPCTSTR bmp_file);
BOOL SaveBitmapToFile(LPCTSTR bmp_file, HBITMAP hbm);
HGLOBAL DIBFromBitmap(HBITMAP hbm);

extern HBITMAP g_hbm;

enum MODE
{
    MODE_SELECT,
    MODE_PENCIL,
    MODE_RECT,
    MODE_ELLIPSE,
    MODE_LINE
};
extern MODE g_nMode;

struct IMode
{
    IMode()
    {
    }
    virtual ~IMode()
    {
    }
    virtual void DoLButtonDown(HWND hwnd, POINT pt) = 0;
    virtual void DoMouseMove(HWND hwnd, POINT pt) = 0;
    virtual void DoLButtonUp(HWND hwnd, POINT pt) = 0;
    virtual void DoPostPaint(HWND hwnd, HDC hDC) = 0;
};

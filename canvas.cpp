#include "paint.h"

static HWND s_hCanvasWnd = NULL;
HBITMAP g_hbm = NULL;
static BOOL s_bDragging = FALSE;
static POINT s_ptOld;
MODE g_nMode = MODE_PENCIL;
static POINT s_pt;
static HBITMAP s_hbmFloating = NULL;
static RECT s_rcFloating;
static COLORREF s_rgbLine = RGB(255, 255, 255);
static COLORREF s_rgbFill = RGB(0, 0, 0);
static HPEN s_hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
static HBRUSH s_hBrush = CreateSolidBrush(RGB(0, 0, 0));

void DoUpdateCanvas(HWND hwnd, HBITMAP hbm, BOOL bResetPos = FALSE)
{
    if (g_hbm != hbm)
    {
        DeleteObject(g_hbm);
        g_hbm = hbm;
    }

    BITMAP bm;
    GetObject(hbm, sizeof(bm), &bm);

    RECT rc;
    GetClientRect(hwnd, &rc);

    {
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = bm.bmWidth;
        si.nPage = rc.right - rc.left;
        if (bResetPos)
        {
            si.fMask |= SIF_POS;
            si.nPos = 0;
        }
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    {
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = bm.bmHeight;
        si.nPage = rc.bottom - rc.top;
        if (bResetPos)
        {
            si.fMask |= SIF_POS;
            si.nPos = 0;
        }
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void DoChooseColor(HWND hwnd, BOOL bFill)
{
    static COLORREF s_custom_colors[16];

    CHOOSECOLOR cc = { sizeof(cc) };
    cc.hwndOwner = hwnd;

    if (bFill)
        cc.rgbResult = s_rgbFill;
    else
        cc.rgbResult = s_rgbLine;

    cc.lpCustColors = s_custom_colors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc))
    {
        if (bFill)
        {
            s_rgbFill = cc.rgbResult;
            DeleteObject(s_hBrush);
            s_hBrush = CreateSolidBrush(s_rgbFill);
        }
        else
        {
            s_rgbLine = cc.rgbResult;
            DeleteObject(s_hPen);
            s_hPen = CreatePen(PS_SOLID, 1, s_rgbLine);
        }
    }
}

void DoNormalizeRect(RECT *prc)
{
    if (prc->left > prc->right)
        std::swap(prc->left, prc->right);
    if (prc->top > prc->bottom)
        std::swap(prc->top, prc->bottom);
}

void DoTakeOff(void)
{
    if (!s_hbmFloating)
    {
        s_hbmFloating = DoGetSubImage(g_hbm, &s_rcFloating);
        DoPutSubImage(g_hbm, &s_rcFloating, NULL);
    }
}

void DoLanding(void)
{
    if (s_hbmFloating)
    {
        DoPutSubImage(g_hbm, &s_rcFloating, s_hbmFloating);
        DeleteBitmap(s_hbmFloating);
        s_hbmFloating = NULL;
    }
    SetRectEmpty(&s_rcFloating);
}

struct ModeSelect : IMode
{
    virtual void DoLButtonDown(HWND hwnd, POINT pt)
    {
        s_pt = s_ptOld = pt;

        if (PtInRect(&s_rcFloating, pt))
        {
            DoTakeOff();
        }
        else
        {
            DoLanding();
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoMouseMove(HWND hwnd, POINT pt)
    {
        if (s_hbmFloating)
        {
            OffsetRect(&s_rcFloating, pt.x - s_ptOld.x, pt.y - s_ptOld.y);
            s_pt = s_ptOld = pt;
        }
        else
        {
            s_pt = pt;
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoLButtonUp(HWND hwnd, POINT pt)
    {
        if (s_hbmFloating)
        {
            OffsetRect(&s_rcFloating, pt.x - s_ptOld.x, pt.y - s_ptOld.y);
            s_pt = s_ptOld = pt;
        }
        else
        {
            s_pt = pt;
            SetRect(&s_rcFloating, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
            DoNormalizeRect(&s_rcFloating);
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoPostPaint(HWND hwnd, HDC hDC)
    {
        RECT rc;
        INT x = GetScrollPos(hwnd, SB_HORZ);
        INT y = GetScrollPos(hwnd, SB_VERT);

        if (s_hbmFloating)
        {
            rc = s_rcFloating;
        }
        else
        {
            SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
        }
        DoNormalizeRect(&rc);

        if (!IsRectEmpty(&s_rcFloating) && s_hbmFloating)
        {
            if (HDC hMemDC = CreateCompatibleDC(NULL))
            {
                HBITMAP hbmOld = SelectBitmap(hMemDC, s_hbmFloating);
                {
                    BitBlt(hDC, s_rcFloating.left - x, s_rcFloating.top - y,
                        s_rcFloating.right - s_rcFloating.left,
                        s_rcFloating.bottom - s_rcFloating.top,
                        hMemDC, 0, 0, SRCCOPY);
                }
                SelectBitmap(hMemDC, hbmOld);
                DeleteDC(hMemDC);
            }
        }

        OffsetRect(&rc, -x, -y);
        DrawFocusRect(hDC, &rc);
    }
};

struct ModePencil : IMode
{
    virtual void DoLButtonDown(HWND hwnd, POINT pt)
    {
        s_pt = s_ptOld = pt;
    }

    virtual void DoMouseMove(HWND hwnd, POINT pt)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HPEN hPenOld = SelectPen(hMemDC, s_hPen);
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            MoveToEx(hMemDC, s_ptOld.x, s_ptOld.y, NULL);
            LineTo(hMemDC, pt.x, pt.y);
            SelectBitmap(hMemDC, hbmOld);
            SelectPen(hMemDC, hPenOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }

        s_ptOld = pt;
    }

    virtual void DoLButtonUp(HWND hwnd, POINT pt)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HPEN hPenOld = SelectPen(hMemDC, s_hPen);
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            MoveToEx(hMemDC, s_ptOld.x, s_ptOld.y, NULL);
            LineTo(hMemDC, pt.x, pt.y);
            SelectBitmap(hMemDC, hbmOld);
            SelectPen(hMemDC, hPenOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }

        s_pt = s_ptOld = pt;
    }

    virtual void DoPostPaint(HWND hwnd, HDC hDC)
    {
    }
};

struct ModeRect : IMode
{
    virtual void DoLButtonDown(HWND hwnd, POINT pt)
    {
        s_pt = s_ptOld = pt;
    }

    virtual void DoMouseMove(HWND hwnd, POINT pt)
    {
        s_pt = pt;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoLButtonUp(HWND hwnd, POINT pt)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            HPEN hPenOld = SelectPen(hMemDC, s_hPen);
            HBRUSH hbrOld = SelectBrush(hMemDC, s_hBrush);
            RECT rc;
            SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
            DoNormalizeRect(&rc);
            Rectangle(hMemDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectBrush(hMemDC, hbrOld);
            SelectPen(hMemDC, hPenOld);
            SelectBitmap(hMemDC, hbmOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }
        s_pt = s_ptOld = pt;
    }

    virtual void DoPostPaint(HWND hwnd, HDC hDC)
    {
        if (memcmp(&s_pt, &s_ptOld, sizeof(POINT)) != 0)
        {
            HPEN hPenOld = SelectPen(hDC, s_hPen);
            HBRUSH hbrOld = SelectBrush(hDC, s_hBrush);
            RECT rc;
            SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
            INT x = GetScrollPos(hwnd, SB_HORZ);
            INT y = GetScrollPos(hwnd, SB_VERT);
            OffsetRect(&rc, -x, -y);
            DoNormalizeRect(&rc);
            Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectBrush(hDC, hbrOld);
            SelectPen(hDC, hPenOld);
        }
    }
};

struct ModeEllipse : IMode
{
    virtual void DoLButtonDown(HWND hwnd, POINT pt)
    {
        s_pt = s_ptOld = pt;
    }

    virtual void DoMouseMove(HWND hwnd, POINT pt)
    {
        s_pt = pt;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoLButtonUp(HWND hwnd, POINT pt)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            HPEN hPenOld = SelectPen(hMemDC, s_hPen);
            HBRUSH hbrOld = SelectBrush(hMemDC, s_hBrush);
            RECT rc;
            SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
            DoNormalizeRect(&rc);
            Ellipse(hMemDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectBrush(hMemDC, hbrOld);
            SelectPen(hMemDC, hPenOld);
            SelectBitmap(hMemDC, hbmOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }
        s_pt = s_ptOld = pt;
    }

    virtual void DoPostPaint(HWND hwnd, HDC hDC)
    {
        if (memcmp(&s_pt, &s_ptOld, sizeof(POINT)) != 0)
        {
            HPEN hPenOld = SelectPen(hDC, s_hPen);
            HBRUSH hbrOld = SelectBrush(hDC, s_hBrush);
            RECT rc;
            SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
            DoNormalizeRect(&rc);
            INT x = GetScrollPos(hwnd, SB_HORZ);
            INT y = GetScrollPos(hwnd, SB_VERT);
            OffsetRect(&rc, -x, -y);
            Ellipse(hDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectBrush(hDC, hbrOld);
            SelectPen(hDC, hPenOld);
        }
    }
};

struct ModeLine : IMode
{
    virtual void DoLButtonDown(HWND hwnd, POINT pt)
    {
        s_pt = s_ptOld = pt;
    }

    virtual void DoMouseMove(HWND hwnd, POINT pt)
    {
        s_pt = pt;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    virtual void DoLButtonUp(HWND hwnd, POINT pt)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            HPEN hPenOld = SelectPen(hMemDC, s_hPen);
            MoveToEx(hMemDC, s_ptOld.x, s_ptOld.y, NULL);
            LineTo(hMemDC, s_pt.x, s_pt.y);
            SelectPen(hMemDC, hPenOld);
            SelectBitmap(hMemDC, hbmOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }
        s_pt = s_ptOld = pt;
    }

    virtual void DoPostPaint(HWND hwnd, HDC hDC)
    {
        if (memcmp(&s_pt, &s_ptOld, sizeof(POINT)) != 0)
        {
            HPEN hPenOld = SelectPen(hDC, s_hPen);
            RECT rc;
            POINT pt1 = { s_ptOld.x, s_ptOld.y};
            POINT pt2 = { s_pt.x, s_pt.y };
            INT x = GetScrollPos(hwnd, SB_HORZ);
            INT y = GetScrollPos(hwnd, SB_VERT);
            pt1.x += -x;
            pt2.x += -x;
            pt1.y += -y;
            pt2.y += -y;
            MoveToEx(hDC, pt1.x, pt1.y, NULL);
            LineTo(hDC, pt2.x, pt2.y);
            SelectPen(hDC, hPenOld);
        }
    }
};

static IMode *s_pMode = new ModePencil();

void DoSetMode(MODE nMode)
{
    delete s_pMode;
    switch (nMode)
    {
    case MODE_SELECT:
        s_pMode = new ModeSelect();
        break;
    case MODE_PENCIL:
        s_pMode = new ModePencil();
        break;
    case MODE_RECT:
        s_pMode = new ModeRect();
        break;
    case MODE_ELLIPSE:
        s_pMode = new ModeEllipse();
        break;
    case MODE_LINE:
        s_pMode = new ModeLine();
        break;
    }
    g_nMode = nMode;
}

static void OnDelete(HWND hwnd)
{
    if (g_nMode != MODE_SELECT)
        return;

    RECT rc;
    SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
    DoNormalizeRect(&rc);

    DoPutSubImage(g_hbm, &rc, NULL);
    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnCopy(HWND hwnd)
{
    RECT rc;
    if (g_nMode == MODE_SELECT)
    {
        SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
        DoNormalizeRect(&rc);
    }
    else
    {
        BITMAP bm;
        GetObject(g_hbm, sizeof(bm), &bm);
        SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
    }

    HBITMAP hbmNew = DoGetSubImage(g_hbm, &rc);
    if (!hbmNew)
        return;

    HGLOBAL hDIB = DIBFromBitmap(hbmNew);

    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();
        SetClipboardData(CF_DIB, hDIB);
        CloseClipboard();
    }

    DeleteBitmap(hbmNew);
}

static void OnSelectAll(HWND hwnd)
{
    DoSetMode(MODE_SELECT);

    DoLanding();

    BITMAP bm;
    GetObject(g_hbm, sizeof(bm), &bm);
    SetRect(&s_rcFloating, 0, 0, bm.bmWidth, bm.bmHeight);
    s_ptOld.x = s_rcFloating.left;
    s_ptOld.y = s_rcFloating.top;
    s_pt.x = s_rcFloating.right;
    s_pt.y = s_rcFloating.bottom;

    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnPaste(HWND hwnd)
{
    if (!IsClipboardFormatAvailable(CF_BITMAP))
        return;

    if (OpenClipboard(hwnd))
    {
        HBITMAP hbm = (HBITMAP)GetClipboardData(CF_BITMAP);
        BITMAP bm;
        if (GetObject(hbm, sizeof(bm), &bm))
        {
            DoLanding();

            DoSetMode(MODE_SELECT);

            SetRect(&s_rcFloating, 0, 0, bm.bmWidth, bm.bmHeight);
            if (s_hbmFloating)
                DeleteObject(s_hbmFloating);
            s_hbmFloating = hbm;

            InvalidateRect(hwnd, NULL, TRUE);
        }

        CloseClipboard();
    }
}

BOOL Size_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    BITMAP bm;
    GetObject(g_hbm, sizeof(bm), &bm);

    SetDlgItemInt(hwnd, edt1, bm.bmWidth, FALSE);
    SetDlgItemInt(hwnd, edt2, bm.bmHeight, FALSE);
    return TRUE;
}

void Size_OnOK(HWND hwnd)
{
    BOOL bTrans;

    INT cx = GetDlgItemInt(hwnd, edt1, &bTrans, FALSE);
    if (!bTrans)
    {
        MessageBox(hwnd, TEXT("ERROR"), NULL, MB_ICONERROR);
        return;
    }

    INT cy = GetDlgItemInt(hwnd, edt1, &bTrans, FALSE);
    if (!bTrans)
    {
        MessageBox(hwnd, TEXT("ERROR"), NULL, MB_ICONERROR);
        return;
    }

    HBITMAP hbm = DoCreate24BppBitmap(cx, cy);
    RECT rc;
    SetRect(&rc, 0, 0, cx, cy);
    DoPutSubImage(hbm, &rc, g_hbm);
    DoUpdateCanvas(s_hCanvasWnd, hbm, TRUE);

    EndDialog(hwnd, IDOK);
}

void Size_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        Size_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    }
}

INT_PTR CALLBACK
SizeDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Size_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Size_OnCommand);
    }
    return 0;
}

static void OnCanvasSize(HWND hwnd)
{
    if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SIZE),
                  hwnd, SizeDialogProc) == IDOK)
    {
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_CUT:
        OnCopy(hwnd);
        OnDelete(hwnd);
        break;
    case ID_COPY:
        OnCopy(hwnd);
        break;
    case ID_PASTE:
        OnPaste(hwnd);
        break;
    case ID_DELETE:
        OnDelete(hwnd);
        break;
    case ID_SELECT_ALL:
        OnSelectAll(hwnd);
        break;
    case ID_SELECT:
        DoSetMode(MODE_SELECT);
        break;
    case ID_PENCIL:
        DoSetMode(MODE_PENCIL);
        break;
    case ID_RECT:
        DoSetMode(MODE_RECT);
        break;
    case ID_ELLIPSE:
        DoSetMode(MODE_ELLIPSE);
        break;
    case ID_LINE:
        DoSetMode(MODE_LINE);
        break;
    case ID_LINE_COLOR:
        DoChooseColor(hwnd, FALSE);
        break;
    case ID_FILL_COLOR:
        DoChooseColor(hwnd, TRUE);
        break;
    case ID_CANVAS_SIZE:
        OnCanvasSize(hwnd);
        break;
    }
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    s_hCanvasWnd = hwnd;
    DoUpdateCanvas(hwnd, DoCreate24BppBitmap(500, 500), TRUE);
    return TRUE;
}

static void OnDestroy(HWND hwnd)
{
    DeleteObject(g_hbm);
    g_hbm = NULL;

    DeleteObject(s_hPen);
    s_hPen = NULL;

    DeleteObject(s_hBrush);
    s_hBrush = NULL;
}

static void OnPaint(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    INT cx = rc.right - rc.left, cy = rc.bottom - rc.top;

    BITMAP bm;
    GetObject(g_hbm, sizeof(bm), &bm);

    PAINTSTRUCT ps;
    if (HDC hDC = BeginPaint(hwnd, &ps))
    {
        if (HBITMAP hbmBuffer = CreateCompatibleBitmap(hDC, cx, cy))
        {
            if (HDC hMemDC1 = CreateCompatibleDC(NULL))
            {
                HBITMAP hbm1Old = SelectBitmap(hMemDC1, hbmBuffer);
                FillRect(hMemDC1, &rc, GetStockBrush(GRAY_BRUSH));
                if (HDC hMemDC2 = CreateCompatibleDC(NULL))
                {
                    HBITMAP hbm2Old = SelectBitmap(hMemDC2, g_hbm);
                    INT x = -GetScrollPos(hwnd, SB_HORZ);
                    INT y = -GetScrollPos(hwnd, SB_VERT);
                    BitBlt(hMemDC1, x, y, bm.bmWidth, bm.bmHeight,
                           hMemDC2, 0, 0, SRCCOPY);
                    s_pMode->DoPostPaint(hwnd, hMemDC1);
                    SelectBitmap(hMemDC2, hbm2Old);
                    DeleteDC(hMemDC2);
                }
                BitBlt(hDC, 0, 0, cx, cy, hMemDC1, 0, 0, SRCCOPY);
                SelectBitmap(hMemDC1, hbm1Old);
                DeleteDC(hMemDC1);
            }
            DeleteObject(hbmBuffer);
        }
        EndPaint(hwnd, &ps);
    }
}

static void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick || s_bDragging)
        return;

    s_bDragging = TRUE;
    SetCapture(hwnd);

    POINT pt = { x, y };
    pt.x += GetScrollPos(hwnd, SB_HORZ);
    pt.y += GetScrollPos(hwnd, SB_VERT);
    s_pMode->DoLButtonDown(hwnd, pt);
}

static void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (!s_bDragging)
        return;

    POINT pt = { x, y };
    pt.x += GetScrollPos(hwnd, SB_HORZ);
    pt.y += GetScrollPos(hwnd, SB_VERT);
    s_pMode->DoMouseMove(hwnd, pt);
}

static void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (!s_bDragging)
        return;

    POINT pt = { x, y };
    pt.x += GetScrollPos(hwnd, SB_HORZ);
    pt.y += GetScrollPos(hwnd, SB_VERT);
    s_pMode->DoLButtonUp(hwnd, pt);

    ReleaseCapture();
    s_bDragging = FALSE;
}

static void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, SB_HORZ, &si);

    INT nOldPos = si.nPos;
    INT nNewPos = nOldPos;
    switch (code)
    {
    case SB_LINELEFT:
        --nNewPos;
        break;
    case SB_LINERIGHT:
        ++nNewPos;
        break;
    case SB_PAGELEFT:
        nNewPos -= si.nPage;
        break;
    case SB_PAGERIGHT:
        nNewPos += si.nPage;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nNewPos = pos;
        break;
    }

    if (nNewPos < si.nMin)
        nNewPos = si.nMin;
    if (nNewPos > si.nMax - si.nPage)
        nNewPos = si.nMax - si.nPage;

    si.fMask = SIF_POS;
    si.nPos = nNewPos;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, SB_VERT, &si);

    INT nOldPos = si.nPos;
    INT nNewPos = nOldPos;
    switch (code)
    {
    case SB_LINEUP:
        --nNewPos;
        break;
    case SB_LINEDOWN:
        ++nNewPos;
        break;
    case SB_PAGEUP:
        nNewPos -= si.nPage;
        break;
    case SB_PAGEDOWN:
        nNewPos += si.nPage;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nNewPos = pos;
        break;
    }

    if (nNewPos < si.nMin)
        nNewPos = si.nMin;
    if (nNewPos > si.nMax - si.nPage)
        nNewPos = si.nMax - si.nPage;

    si.fMask = SIF_POS;
    si.nPos = nNewPos;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    INT x = GetScrollPos(hwnd, SB_HORZ);
    INT y = GetScrollPos(hwnd, SB_VERT);
    if (fwKeys & MK_SHIFT)
        x += -zDelta;
    else
        y += -zDelta;
    SetScrollPos(hwnd, SB_HORZ, x, TRUE);
    SetScrollPos(hwnd, SB_VERT, y, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    DoUpdateCanvas(hwnd, g_hbm, FALSE);
}

LRESULT CALLBACK
CanvasWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
        HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
        HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        case WM_CAPTURECHANGED:
            s_bDragging = FALSE;
            break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

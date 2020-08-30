#include "paint.h"

static const TCHAR s_szName[] = TEXT("My Paint");
static const TCHAR s_szCanvasName[] = TEXT("My Paint Canvas");

static HINSTANCE s_hInst = NULL;
static HWND s_hMainWnd = NULL;
static HWND s_hCanvasWnd = NULL;

LPTSTR LoadStringDx(INT nID)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static TCHAR s_sz[4][cchBuffMax];

    TCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
    ::LoadString(NULL, nID, pszBuff, cchBuffMax);
    return pszBuff;
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    DWORD style = WS_HSCROLL | WS_VSCROLL | WS_CHILD | WS_VISIBLE;
    DWORD exstyle = WS_EX_CLIENTEDGE;
    HWND hCanvas = CreateWindowEx(exstyle, s_szCanvasName, s_szCanvasName, style,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hwnd, (HMENU)(INT_PTR)ctl1, s_hInst, NULL);
    if (hCanvas == NULL)
        return FALSE;

    s_hCanvasWnd = hCanvas;
    DragAcceptFiles(hwnd, TRUE);

    return TRUE;
}

void OnDestroy(HWND hwnd)
{
    PostQuitMessage(0);
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    MoveWindow(s_hCanvasWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

BOOL DoLoad(HWND hwnd, LPCTSTR pszFile)
{
    if (HBITMAP hbm = LoadBitmapFromFile(pszFile))
    {
        DeleteBitmap(g_hbm);
        g_hbm = hbm;
        InvalidateRect(s_hCanvasWnd, NULL, TRUE);
        return TRUE;
    }
    MessageBox(hwnd, LoadStringDx(101), NULL, MB_ICONERROR);
    return FALSE;
}

static void OnOpen(HWND hwnd)
{
    TCHAR szFile[MAX_PATH] = TEXT("");
    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_ENABLESIZING;
    ofn.lpstrDefExt = TEXT("bmp");
    if (GetOpenFileName(&ofn))
    {
        DoLoad(hwnd, szFile);
    }
}

BOOL DoSave(HWND hwnd, LPCTSTR pszFile)
{
    if (SaveBitmapToFile(pszFile, g_hbm))
        return TRUE;
    MessageBox(hwnd, LoadStringDx(100), NULL, MB_ICONERROR);
    return FALSE;
}

static void OnSave(HWND hwnd)
{
    TCHAR szFile[MAX_PATH] = TEXT("");
    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_ENABLESIZING;
    ofn.lpstrDefExt = TEXT("bmp");
    if (GetSaveFileName(&ofn))
    {
        DoSave(hwnd, szFile);
    }
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_OPEN:
        OnOpen(hwnd);
        break;
    case ID_SAVE:
        OnSave(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_CUT:
    case ID_COPY:
    case ID_PASTE:
    case ID_DELETE:
    case ID_SELECT_ALL:
    case ID_SELECT:
    case ID_PENCIL:
    case ID_RECT:
    case ID_ELLIPSE:
    case ID_LINE:
    case ID_LINE_COLOR:
    case ID_FILL_COLOR:
    case ID_CANVAS_SIZE:
        SendMessage(s_hCanvasWnd, WM_COMMAND, id, 0);
        break;
    }
}

static void OnDropFiles(HWND hwnd, HDROP hdrop)
{
    TCHAR szPath[MAX_PATH];

    DragQueryFile(hdrop, 0, szPath, MAX_PATH);
    DragFinish(hdrop);

    DoLoad(hwnd, szPath);
}

static void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    switch (g_nMode)
    {
    case MODE_SELECT:
        CheckMenuRadioItem(hMenu, ID_SELECT, ID_LINE, ID_SELECT, MF_BYCOMMAND);
        break;
    case MODE_PENCIL:
        CheckMenuRadioItem(hMenu, ID_SELECT, ID_LINE, ID_PENCIL, MF_BYCOMMAND);
        break;
    case MODE_RECT:
        CheckMenuRadioItem(hMenu, ID_SELECT, ID_LINE, ID_RECT, MF_BYCOMMAND);
        break;
    case MODE_ELLIPSE:
        CheckMenuRadioItem(hMenu, ID_SELECT, ID_LINE, ID_ELLIPSE, MF_BYCOMMAND);
        break;
    case MODE_LINE:
        CheckMenuRadioItem(hMenu, ID_SELECT, ID_LINE, ID_LINE, MF_BYCOMMAND);
        break;
    }
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    s_hInst = hInstance;
    InitCommonControls();

    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(1);
    wc.lpszClassName = s_szName;
    if (!RegisterClass(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return -1;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = CanvasWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = s_szCanvasName;
    if (!RegisterClass(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return -2;
    }

    s_hMainWnd = CreateWindow(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
    if (!s_hMainWnd)
    {
        MessageBoxA(NULL, "CreateWindow failed", NULL, MB_ICONERROR);
        return -3;
    }

    ShowWindow(s_hMainWnd, nCmdShow);
    UpdateWindow(s_hMainWnd);

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(1));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(s_hMainWnd, hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyAcceleratorTable(hAccel);

    return 0;
}

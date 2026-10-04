// MouseTracker.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "MouseTracker.h"
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

#define MAX_LOADSTRING 100
#define IDM_BACKGROUND 200
#define IDM_CURSOR     201

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HDC      g_hdc;                                 // the window's own DC (CS_OWNDC)
COLORREF g_bg = GetSysColor(COLOR_WINDOW);      // background color
COLORREF g_fg = GetSysColor(COLOR_WINDOWTEXT);  // cursor color
LONGLONG g_freq;                                // timer ticks per second
LONGLONG g_last;                                // time of the previous report
LONGLONG g_curTicks, g_curCount;                // report intervals since the last readout
LONGLONG g_sumTicks, g_sumCount;                // report intervals for the whole session
WCHAR    g_text[64] = L"Current: 0 Hz    Average: 0 Hz";

const POINT g_arrow[] = { {0,0}, {0,16}, {4,12}, {7,18}, {9,17}, {6,11}, {11,11} };

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MOUSETRACKER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSETRACKER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSETRACKER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MOUSETRACKER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// Applies the two chosen colors to the window's DC and repaints.
void ApplyColors(HWND hWnd)
{
    SetBkColor(g_hdc, g_bg);        // background fill and text background
    SetDCPenColor(g_hdc, g_bg);     // cursor outline, keeps overlapping cursors distinct
    SetDCBrushColor(g_hdc, g_fg);   // cursor fill
    SetTextColor(g_hdc, g_fg);
    InvalidateRect(hWnd, nullptr, TRUE);
}

// Shows the standard Windows color dialog.
void PickColor(HWND hWnd, COLORREF& color)
{
    static COLORREF custom[16];
    CHOOSECOLORW cc = { sizeof(cc) };
    cc.hwndOwner = hWnd;
    cc.rgbResult = color;
    cc.lpCustColors = custom;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&cc))
    {
        color = cc.rgbResult;
        ApplyColors(hWnd);
    }
}

// Fills the window with the background color.
void Clear(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    ExtTextOutW(g_hdc, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);
}

// Called once per mouse report: times it and draws a cursor where the pointer is.
void OnReport(HWND hWnd, HRAWINPUT hRaw)
{
    RAWINPUT ri;
    UINT size = sizeof(ri);
    if (GetRawInputData(hRaw, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1
        || ri.header.dwType != RIM_TYPEMOUSE
        || !(ri.data.mouse.lLastX | ri.data.mouse.lLastY))
        return;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LONGLONG dt = now.QuadPart - g_last;
    g_last = now.QuadPart;

    if (dt < g_freq / 20)           // under 50 ms: the mouse is still moving
    {
        g_curTicks += dt; g_curCount++;
        g_sumTicks += dt; g_sumCount++;
    }
    else if (dt > g_freq / 2)       // new movement after a pause: start a fresh trail
        Clear(hWnd);

    POINT p, a[7];
    GetCursorPos(&p);
    ScreenToClient(hWnd, &p);
    for (int i = 0; i < 7; i++)
        a[i] = { p.x + g_arrow[i].x, p.y + g_arrow[i].y };
    Polygon(g_hdc, a, 7);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_CREATE     - set up the DC, the Color menu, raw mouse input and the readout timer
//  WM_INPUT      - one mouse report
//  WM_TIMER      - refresh the polling rate readout
//  WM_COMMAND    - process the application menu
//  WM_DESTROY    - post a quit message and return
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            g_hdc = GetDC(hWnd);
            SelectObject(g_hdc, GetStockObject(DC_BRUSH));
            SelectObject(g_hdc, GetStockObject(DC_PEN));
            SelectObject(g_hdc, GetStockObject(DEFAULT_GUI_FONT));
            ApplyColors(hWnd);

            HMENU colors = CreatePopupMenu();
            AppendMenuW(colors, MF_STRING, IDM_BACKGROUND, L"&Background...");
            AppendMenuW(colors, MF_STRING, IDM_CURSOR, L"&Cursor...");
            InsertMenuW(GetMenu(hWnd), 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)colors, L"&Color");

            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            g_freq = f.QuadPart;

            RAWINPUTDEVICE rid = { 1, 2, 0, hWnd };     // generic desktop, mouse
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
            SetTimer(hWnd, 1, 250, nullptr);
        }
        break;
    case WM_INPUT:
        OnReport(hWnd, (HRAWINPUT)lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_TIMER:
        if (g_curCount)
        {
            wsprintfW(g_text, L"Current: %d Hz    Average: %d Hz        ",
                (int)(g_curCount * g_freq / g_curTicks), (int)(g_sumCount * g_freq / g_sumTicks));
            g_curTicks = g_curCount = 0;
        }
        TextOutW(g_hdc, 8, 8, g_text, lstrlenW(g_text));
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_BACKGROUND:
                PickColor(hWnd, g_bg);
                break;
            case IDM_CURSOR:
                PickColor(hWnd, g_fg);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_ERASEBKGND:
        Clear(hWnd);
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
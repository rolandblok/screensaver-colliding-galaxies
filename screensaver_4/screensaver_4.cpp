#include <windows.h>
#include <iostream>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int g_iMoveX = 2;
int g_iMoveY = 2;
int g_iTextX = 100;
int g_iTextY = 100;
const wchar_t* g_szText = L"Hello, scr!";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND hWndParent = NULL;
    if (lpCmdLine[0] == '/' && (lpCmdLine[1] == 'p' || lpCmdLine[1] == 'P'))
    {
        // Preview mode
        hWndParent = (HWND)atoi(lpCmdLine + 3);
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("ScreensaverClass");

    RegisterClass(&wc);

    HWND hWnd;
    if (hWndParent)
    {
        OutputDebugString( L"Screensaver preview started\n" );
        // Create a child window for preview mode
        hWnd = CreateWindowEx(
            0,                          // No extended styles
            wc.lpszClassName,           // Class name
            NULL,                       // No window name
            WS_CHILD | WS_VISIBLE,      // Child window style
            0, 0, 0, 0,                 // Position and size (will be adjusted)
            hWndParent,                 // Parent window
            NULL,                       // No menu
            hInstance,                  // Instance handle
            NULL                        // No additional application data
        );

        // Adjust the window size to fit the preview window
        RECT rect;
        GetClientRect(hWndParent, &rect);
        SetWindowPos(hWnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else
    {
        OutputDebugString(L"Screensaver started\n");

        // Full-screen mode
        hWnd = CreateWindowEx(
            WS_EX_TOPMOST,              // Extended window style
            wc.lpszClassName,           // Class name
            TEXT("Screensaver"),        // Window name
            WS_POPUP,                   // Window style
            0, 0,                       // Position
            GetSystemMetrics(SM_CXSCREEN), // Width
            GetSystemMetrics(SM_CYSCREEN), // Height
            NULL,                       // No parent window
            NULL,                       // No menu
            hInstance,                  // Instance handle
            NULL                        // No additional application data
        );

        if (!hWnd)
        {
            MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error"), MB_ICONEXCLAMATION | MB_OK);
            return 0;
        }

        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    }

    
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hdcMem = NULL;
    static HBITMAP hbmMem = NULL;
    static HBITMAP hbmOld = NULL;


    switch (message)
    {

    case WM_CREATE:
    {
        SetTimer(hWnd, 1, 16, NULL);    // Approximately 60 FPS
        // Initialize double buffering
        HDC hdc = GetDC(hWnd);
        static RECT rect;
        GetClientRect(hWnd, &rect);
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        ReleaseDC(hWnd, hdc);
        break;
    }

    case WM_TIMER:
    {
        // detect if key is pressed
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            PostQuitMessage(0);
        }

        RECT rect;
        GetClientRect(hWnd, &rect);

        // Move the text
        g_iTextX += g_iMoveX;
        g_iTextY += g_iMoveY;

        // Bounce off the edges
        if (g_iTextX < 0 || g_iTextX > rect.right - 200)
            g_iMoveX = -g_iMoveX;
        if (g_iTextY < 0 || g_iTextY > rect.bottom - 50)
            g_iMoveY = -g_iMoveY;

        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Clear the off-screen buffer
        RECT rect;
        GetClientRect(hWnd, &rect);
        FillRect(hdcMem, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        // Set text color and background mode
        SetTextColor(hdcMem, RGB(0, 0, 0));
        SetBkMode(hdcMem, TRANSPARENT);

        // Draw the text in the off-screen buffer
        TextOut(hdcMem, g_iTextX, g_iTextY, g_szText, lstrlen(g_szText));

        // Copy the off-screen buffer to the screen
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_SIZE:
    {
        // Handle window resizing
        RECT rect;
        GetClientRect(hWnd, &rect);
        if (hdcMem)
        {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            hbmMem = CreateCompatibleBitmap(GetDC(hWnd), rect.right, rect.bottom);
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        }
        break;
    }
    case WM_DESTROY:
        // Clean up double buffering resources
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
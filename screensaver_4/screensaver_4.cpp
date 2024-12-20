#include <windows.h>
#include <iostream>
#include <string>

#include "Star.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


// Timer IDs
#define IDT_TIMER_DRAW 1
#define IDT_TIMER_UPDATE 2


// Timer intervals
#define TIMER_DRAW_INTERVAL 1  
#define TIMER_UPDATE_INTERVAL 1  

// galaxy
Universe* universe;
constexpr int no_galaxies = 3;
constexpr int no_stars = 1000;
constexpr int restart_time_s = 15;


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND hWndParent = NULL;
    if (lpCmdLine[0] == '/' && (lpCmdLine[1] == 'p' || lpCmdLine[1] == 'P'))
    {
        // Preview mode
        hWndParent = (HWND)strtoull(lpCmdLine + 3, NULL, 10);
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
            //1200,1200,
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

        // do not show mouse
        ShowCursor(FALSE);

        //ShowWindow(hWnd, nCmdShow);
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
    static HBITMAP hbmMem = NULL; // off-screen buffer
    static HBITMAP hbmOld = NULL; // off-screen buffer
    static bool mouseDown = false;
    static int fps = 0;
    static int fps_count = 0;
    static ULONGLONG last_fps_ms = 0;
    static ULONGLONG last_restart_ms = 0;
    // Create a pixel buffer
    static DWORD* pixels = NULL;


    switch (message)
    {

    case WM_CREATE:
    {
        SetTimer(hWnd, IDT_TIMER_DRAW, TIMER_DRAW_INTERVAL, NULL);
        SetTimer(hWnd, IDT_TIMER_UPDATE, TIMER_UPDATE_INTERVAL, NULL);

        // Initialize double buffering
        HDC hdc = GetDC(hWnd);
        static RECT rect;
        GetClientRect(hWnd, &rect);
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        ReleaseDC(hWnd, hdc);

        pixels = new DWORD[rect.right * rect.bottom];

        fps = 0;
        fps_count = 0;
        last_fps_ms = GetTickCount64();

        // Create the universe
        universe = new Universe(no_galaxies, no_stars, rect.right, rect.bottom);

        break;
    }

    case WM_TIMER:
    {
        // detect if any key is pressed
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            PostQuitMessage(0);
        }

        // if mouse click-release, create new universe
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
            if (!mouseDown)
            {
                mouseDown = true;
                OutputDebugStringA("Mouse click, create new universe\n");
                RECT rect;
                GetClientRect(hWnd, &rect);
                delete universe;
                universe = new Universe(no_galaxies, no_stars, rect.right, rect.bottom);
            }
        }
        else {
            mouseDown = false;
        }

        if (wParam == IDT_TIMER_DRAW)
        {
			// Redraw the window
            InvalidateRect(hWnd, NULL, TRUE);

        } else if (wParam == IDT_TIMER_UPDATE) {
            // if duration_s is greater than restart_time_s, create new universe
            ULONGLONG current_time_ms = GetTickCount64();
            int duration_ms = (int)(current_time_ms - last_restart_ms);
            if (duration_ms > restart_time_s * 1000)
			{
				OutputDebugStringA("Time to create new universe\n");
				RECT rect;
				GetClientRect(hWnd, &rect);
				delete universe;
				universe = new Universe(no_galaxies, no_stars, rect.right, rect.bottom);
                last_restart_ms = current_time_ms;
			}
            
            // Update the universe
			universe->update();
        }
        break; // WM_TIMER
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Clear the off-screen buffer
        RECT rect;
        GetClientRect(hWnd, &rect);

        int width = rect.right;
        int height = rect.bottom;

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32; // 32-bit color
        bmi.bmiHeader.biCompression = BI_RGB;


        if (pixels != NULL)
        {
            // Clear the pixel buffer
            memset(pixels, 0, width * height * sizeof(DWORD));

            universe->draw(pixels, width, height);

            // Draw the bitmap
            SetDIBitsToDevice(hdcMem, 0, 0, width, height, 0, 0, 0, height, pixels, &bmi, DIB_RGB_COLORS);
        }

        // Calculate FPS
        fps_count++;
        ULONGLONG current_time_ms = GetTickCount64();
        if (current_time_ms - last_fps_ms > 1000)
		{
            fps = fps_count;
			last_fps_ms = current_time_ms;
			fps_count = 0;
            OutputDebugStringA(("FPS: " + std::to_string(fps) + "\n").c_str());
		}
        
        std::string title = "FPS: " + std::to_string(fps);
        //draw the fps on the screen
        //set the text color to white
        SetTextColor(hdcMem, RGB(255, 255, 255));
        SetBkMode(hdcMem, TRANSPARENT);
        // change text size
        HFONT hFont, hOldFont;
        hFont = CreateFont(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        hOldFont = (HFONT)SelectObject(hdcMem, hFont);

        TextOutA(hdcMem, 10, 10, title.c_str(), title.size());
        

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

        delete[] pixels;
		pixels = new DWORD[rect.right * rect.bottom];

        OutputDebugStringA(("Window resized to " + std::to_string(rect.right) + "x" + std::to_string(rect.bottom) + "\n").c_str());

        break;
    }
    case WM_DESTROY:
        // Clean up double buffering resources
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        PostQuitMessage(0);
        delete universe;
        delete[] pixels;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
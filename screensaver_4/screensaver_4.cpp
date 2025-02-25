#include <windows.h>
#include <iostream>
#include <string>

#include "Star.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


// Timer IDs
#define IDT_TIMER_DRAW 1
#define IDT_TIMER_UPDATE 2

CRITICAL_SECTION universe_lock;  // Global critical section object

// Timer intervals
#define TIMER_DRAW_INTERVAL 10  
#define TIMER_UPDATE_INTERVAL 2

// galaxy
Universe* universe;
constexpr int no_galaxies = 1;
constexpr int no_stars = 1000;
constexpr int restart_time_s = 15;

// window size
RECT window_rect;

DWORD WINAPI universe_update(PVOID lpParam)
{
    LARGE_INTEGER last_restart;
    LARGE_INTEGER current_time;
    LARGE_INTEGER frequency; 
    if (lpParam == NULL)
    {
        printf("TimerRoutine lpParam is NULL\n");
    }
    else
    {
        // get the frequency of the performance counter
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&last_restart);
        while (true) {
            HWND hWnd = (HWND)lpParam;

            // lpParam points to the argument; in this case it is an int
            // if duration_s is greater than restart_time_s, create new universe
            LARGE_INTEGER current_time;
            QueryPerformanceCounter(&current_time);
            double elapsed_time_s = 1.0 * (current_time.QuadPart - last_restart.QuadPart) / frequency.QuadPart;

            if (elapsed_time_s > restart_time_s)
            {
                OutputDebugStringA("Time to create new universe\n");
                
                // output the rect.right and rect.bottom
                OutputDebugStringA(("Window resized to " + std::to_string(window_rect.right) + "x" + std::to_string(window_rect.bottom) + "\n").c_str());
                EnterCriticalSection(&universe_lock);
                delete universe;
                universe = new Universe(no_galaxies, no_stars, window_rect.right, window_rect.bottom);
                LeaveCriticalSection(&universe_lock);
                QueryPerformanceCounter(&last_restart);
            }

            // Update the universe
            EnterCriticalSection(&universe_lock);
            universe->update();
            LeaveCriticalSection(&universe_lock);

			// sleep for nothing, release thread for moment to other threads
			Sleep(0);
            
            //get the current time
            

            //sleep for TIMER_UPDATE_INTERVAL
            LARGE_INTEGER current_sleep_time;
            do {
				QueryPerformanceCounter(&current_sleep_time);
			} while (1000 * (current_sleep_time.QuadPart - current_time.QuadPart) / frequency.QuadPart < TIMER_UPDATE_INTERVAL);
            
        }
        
    }

    return (DWORD)0;
}


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
    InitializeCriticalSection(&universe_lock);


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
        GetClientRect(hWndParent, &window_rect);
        SetWindowPos(hWnd, NULL, 0, 0, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
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

        // make event thread stuff
        DWORD dwThreadID;
        HANDLE   thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size  
            universe_update,       // thread function name
            &hWnd,          // argument to thread function 
            0,                      // use default creation flags 
            &dwThreadID);   // returns the thread identifier 


        // Check the return value for success.
        // If CreateThread fails, terminate execution. 
        // This will automatically clean up threads and memory. 
		if (thread == NULL)
		{
			// print error
            OutputDebugStringA("CreateThread failed\n");
			ExitProcess(3);
		}
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
        
        GetClientRect(hWnd, &window_rect);
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, window_rect.right, window_rect.bottom);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        ReleaseDC(hWnd, hdc);

        pixels = new DWORD[window_rect.right * window_rect.bottom];

        fps = 0;
        fps_count = 0;
        last_fps_ms = GetTickCount64();

        // Create the universe
        EnterCriticalSection(&universe_lock);
        universe = new Universe(no_galaxies, no_stars, window_rect.right, window_rect.bottom);
        LeaveCriticalSection(&universe_lock);
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
               
                GetClientRect(hWnd, &window_rect);
                EnterCriticalSection(&universe_lock);
                delete universe;
                universe = new Universe(no_galaxies, no_stars, window_rect.right, window_rect.bottom);
                LeaveCriticalSection(&universe_lock);
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
            //replaced by TimerRoutine
        }
        break; // WM_TIMER
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Clear the off-screen buffer
        GetClientRect(hWnd, &window_rect);

        int width = window_rect.right;
        int height = window_rect.bottom;

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
        BitBlt(hdc, 0, 0, window_rect.right, window_rect.bottom, hdcMem, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);

        break;
    }
    case WM_SIZE:
    {
        // Handle window resizing
        GetClientRect(hWnd, &window_rect);
        if (hdcMem)
        {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            hbmMem = CreateCompatibleBitmap(GetDC(hWnd), window_rect.right, window_rect.bottom);
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        }

        delete[] pixels;
		pixels = new DWORD[window_rect.right * window_rect.bottom];

        OutputDebugStringA(("Window resized to " + std::to_string(window_rect.right) + "x" + std::to_string(window_rect.bottom) + "\n").c_str());

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
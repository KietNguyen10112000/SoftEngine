#include "Platform/Platform.h"

#include "Core/Input/Input.h"
#include "Core/Input/KEYBOARD.h"

#include <Windows.h>

NAMESPACE_PLATFORM_BEGIN

struct WindowsWindow
{
    HWND hwnd;
    const wchar_t* title;
    Input* input;
    bool close = false;
};

class PlatformInput : public Input
{
    friend LRESULT WndHandle(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (!ptr)
        {
            goto Return;
        }

        auto w = (WindowsWindow*)ptr;
        PlatformInput* input = (PlatformInput*)w->input;

        switch (uMsg)
        {
        case WM_KEYDOWN:
            //Input::lastKeyDown = wParam;
            input->DownKey(wParam);
            break;

        case WM_KEYUP:
            input->UpKey(wParam);
            break;

        case WM_LBUTTONDOWN:
            input->DownKey(KEYBOARD::MOUSE_LEFT);
            break;

        case WM_LBUTTONUP:
            input->UpKey(KEYBOARD::MOUSE_LEFT);
            break;

        //case WM_LBUTTONDBLCLK:
        //    //Input::doubleClick = MOUSE_BUTTON::LEFT;
        //    break;

        case WM_RBUTTONDOWN:
            input->DownKey(KEYBOARD::MOUSE_RIGHT);
            break;

        case WM_RBUTTONUP:
            input->UpKey(KEYBOARD::MOUSE_MID);
            break;

        /*case WM_RBUTTONDBLCLK:
            Input::doubleClick = MOUSE_BUTTON::RIGHT;
            break;*/

        case WM_MBUTTONDOWN:
            input->DownKey(KEYBOARD::MOUSE_MID);
            break;

        case WM_MBUTTONUP:
            input->UpKey(KEYBOARD::MOUSE_MID);
            break;

        /*case WM_MBUTTONDBLCLK:
            Input::doubleClick = MOUSE_BUTTON::MID;
            break;*/

        /*case WM_MOUSEWHEEL:
            Input::deltaMouseWheel = GET_WHEEL_DELTA_WPARAM(wParam);
            break;*/

        /*case WM_SIZE:

            switch (wParam)
            {
            case SIZE_MAXIMIZED:
                break;
            case SIZE_MINIMIZED:
                break;
            case SIZE_RESTORED:
                break;
            }

            break;*/

        case WM_CLOSE:
        case WM_DESTROY:
            //PostQuitMessage(0);
            //SendMessage(hWnd, WM_CLOSE, wParam, lParam);
            w->close = true;
            break;
        }

    Return:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
};

#undef CreateWindow
WindowNative* CreateWindow(::soft::Input* input, int x, int y, int width, int height, const char* title)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WindowsWindow* ret = rheap::New<WindowsWindow>();

    int wtitleLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = rheap::NewArray<wchar_t>(wtitleLen);
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, wtitleLen);

    auto hr = CoInitialize(nullptr);

    HINSTANCE hInstance = NULL;

    if (hInstance == NULL)
        hInstance = (HINSTANCE)GetModuleHandle(NULL);

    WNDCLASS wndClass;
    wndClass.style = CS_DBLCLKS;
    wndClass.lpfnWndProc = WndHandle;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = NULL;
    wndClass.hCursor = LoadCursor(NULL, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = wtitle;

    if (!RegisterClass(&wndClass))
    {
        DWORD dwError = GetLastError();
        throw L"RegisterClassW() failed.";
    }

    //RECT desktop;
    //// Get a handle to the desktop window
    //const HWND hDesktop = GetDesktopWindow();
    //// Get the size of screen to the variable desktop
    //GetWindowRect(hDesktop, &desktop);

    //int x = (desktop.right - m_width) / 2;
    //int y = (desktop.bottom - m_height) / 2;

    int maxW = GetSystemMetrics(SM_CXSCREEN);
    int maxH = GetSystemMetrics(SM_CYSCREEN);

    width = width == -1 ? maxW : width;
    height = height == -1 ? maxH : height;

    if (height == maxH)
    {
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
        height = rect.bottom - rect.top;
    }

    auto style = WS_SYSMENU | WS_BORDER | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

    RECT rc;

    SetRect(&rc, 0, 0, width, height);
    AdjustWindowRect(
        &rc,
        style,
        false
    );

    // Create the window for our viewport.
    auto hwnd = CreateWindowW(
        wtitle,
        wtitle,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (rc.right - rc.left), (rc.bottom - rc.top),
        0,
        NULL,
        hInstance,
        0
    );

    ret->hwnd = hwnd;
    ret->title = wtitle;
    ret->input = input;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ret);
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);

    return ret;
}

bool ProcessPlatformMsg(WindowNative* window)
{
    WindowsWindow* w = (WindowsWindow*)window;

    MSG  msg = { 0 };
    while (PeekMessage(&msg, w->hwnd, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return w->close == true;
}

void DeleteWindow(WindowNative* window)
{
    WindowsWindow* w = (WindowsWindow*)window;

    CoUninitialize();
    UnregisterClass(w->title, (HINSTANCE)GetModuleHandle(NULL));
    DestroyWindow(w->hwnd);

    rheap::Delete(w->title);
    rheap::Delete(w);
}

void* GetWindowNativeHandle(WindowNative* window)
{
    WindowsWindow* w = (WindowsWindow*)window;
    return w->hwnd;
}

NAMESPACE_PLATFORM_END
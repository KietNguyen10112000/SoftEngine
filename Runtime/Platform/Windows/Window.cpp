#include "Platform/Platform.h"

#include "Input/Input.h"
#include "Input/KEYBOARD.h"

#include <Windows.h>
#include <windowsx.h>
#include <hidusage.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
public:
    friend void PlatformInput_Init(WindowsWindow* w)
    {
        PlatformInput* input = (PlatformInput*)w->input;

        POINT point;
        GetCursorPos(&point);

        auto& cursor = input->m_curCursors[0];
        auto& prev = input->m_prevCursors[0];

        cursor.isLocked = false;
        cursor.isActive = true;
        cursor.offset = { 0,0 };
        cursor.position = { point.x, point.y };

        prev = cursor;

        RECT rect;
        ::GetClientRect(w->hwnd, &rect);
        input->m_clientWidth = rect.right - rect.left;
        input->m_clientHeight = rect.bottom - rect.top;
        input->m_clientPosition = { rect.top, rect.left };

        ::GetWindowRect(w->hwnd, &rect);
        input->m_windowWidth = rect.right - rect.left;
        input->m_windowHeight = rect.bottom - rect.top;
        input->m_windowPosition = { rect.top, rect.left };

        /*RAWINPUTDEVICE Rid[1];
        Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        Rid[0].dwFlags = RIDEV_INPUTSINK;
        Rid[0].hwndTarget = w->hwnd;
        RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));*/
    }

    friend void PlatformInput_ProcessCursorPos(WindowsWindow* w, int deltaX, int deltaY)
    {
        PlatformInput* input = (PlatformInput*)w->input;

        POINT point;
        GetCursorPos(&point);

        int x = point.x;
        int y = point.y;

        //int x = 0;
        //int y = 0;

        auto& cursor = input->m_curCursors[0];
        auto& prev = input->m_prevCursors[0];

        if (cursor.position.x == x && cursor.position.y == y)
        {
            return;
        }

        input->SetCursor(0, x, y, true);

        //cursor.offset = { deltaX, deltaY };

        if (cursor.isLocked)
        {
            SetCursorPos(prev.position.x, prev.position.y);
            cursor.position = prev.position;
        }
    }

    friend LRESULT WndHandle(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        auto ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (!ptr)
        {
            goto Return;
        }

        auto w = (WindowsWindow*)ptr;
        PlatformInput* input = (PlatformInput*)w->input;

        switch (uMsg)
        {
        case WM_MOVE:
        case WM_SIZE:
        {
            RECT rect;
            ::GetClientRect(w->hwnd, &rect);
            input->m_clientWidth = rect.right - rect.left;
            input->m_clientHeight = rect.bottom - rect.top;
            input->m_clientPosition = { rect.top, rect.left };

            ::GetWindowRect(w->hwnd, &rect);
            input->m_windowWidth = rect.right - rect.left;
            input->m_windowHeight = rect.bottom - rect.top;
            input->m_windowPosition = { rect.top, rect.left };

            break;
        }
        //case WM_INPUT:
        //{
        //    UINT dwSize = sizeof(RAWINPUT);
        //    static BYTE lpb[sizeof(RAWINPUT)];

        //    GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
        //        lpb, &dwSize, sizeof(RAWINPUTHEADER));

        //    RAWINPUT* raw = (RAWINPUT*)lpb;

        //    if (raw->header.dwType == RIM_TYPEMOUSE)
        //    {
        //        int xPosRelative = raw->data.mouse.lLastX;
        //        int yPosRelative = raw->data.mouse.lLastY;
        //        PlatformInput_ProcessCursorPos(w, xPosRelative, yPosRelative);

        //        //std::cout << xPosRelative << ", " << yPosRelative << "\n";
        //    }
        //    break;
        //}
        /*case WM_MOUSEMOVE:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            PlatformInput_ProcessCursorPos(w, xPos + input->m_windowPosition.x, yPos + input->m_windowPosition.y);
            break;
        }*/
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
    ShowWindow(hwnd, SW_SHOW);

    PlatformInput_Init(ret);

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

    PlatformInput_ProcessCursorPos(w, 0, 0);

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

String GetExecutablePath()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH] = { 0 };
    auto len = GetModuleFileNameW(NULL, path, MAX_PATH);

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)len, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)len, &strTo[0], size_needed, NULL, NULL);

    return strTo.c_str();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#endif
}

NAMESPACE_PLATFORM_END
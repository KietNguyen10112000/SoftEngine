#include "Window.h"

LRESULT WndHandle(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        //return true;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        //Input::lastKeyDown = wParam;
        Input::key[wParam] = true;
        break;

    case WM_KEYUP:
        Input::lastKeyUp = wParam;
        Input::key[wParam] = false;
        break;

    case WM_LBUTTONDOWN:
        Input::mouse[0] = true;
        Input::deltaClickTimer = 0;
        Input::click = LEFT;
        break;

    case WM_LBUTTONUP:
        Input::mouse[0] = false;
        Input::click = LEFT;
        break;

    case WM_LBUTTONDBLCLK:
        Input::doubleClick = MOUSE_BUTTON::LEFT;
        break;

    case WM_RBUTTONDOWN:
        Input::mouse[1] = true;
        Input::deltaClickTimer = 0;
        Input::click = RIGHT;
        break;

    case WM_RBUTTONUP:
        Input::mouse[1] = false;
        Input::click = RIGHT;
        break;

    case WM_RBUTTONDBLCLK:
        Input::doubleClick = MOUSE_BUTTON::RIGHT;
        break;

    case WM_MBUTTONDOWN:
        Input::mouse[2] = true;
        break;

    case WM_MBUTTONUP:
        Input::mouse[2] = false;
        Input::click = MID;
        break;

    case WM_MBUTTONDBLCLK:
        Input::doubleClick = MOUSE_BUTTON::MID;
        break;

    case WM_MOUSEWHEEL:
        Input::deltaMouseWheel = GET_WHEEL_DELTA_WPARAM(wParam);
        break;

    case WM_SIZE:
        
        switch (wParam)
        {
        case SIZE_MAXIMIZED:
            break;
        case SIZE_MINIMIZED:
            break;
        case SIZE_RESTORED:
            break;
        }

        break;

    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Window::Window(const wchar_t* title, int& width, int& height, bool fullScreen)
{
	auto hr = CoInitialize(nullptr);
	m_width = width;
	m_height = height;
    m_title = title;

    HINSTANCE hInstance = NULL;

    if (hInstance == NULL)
        hInstance = (HINSTANCE)GetModuleHandle(NULL);

    WNDCLASSW wndClass;
    wndClass.style = CS_DBLCLKS;
    wndClass.lpfnWndProc = WndHandle;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = NULL;
    wndClass.hCursor = LoadCursor(NULL, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = m_title.c_str();

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
    bool forceMaximize = false;

    if (m_width == 0 || m_height == 0)
    {
        m_width = maxW;
        m_height = maxH;

        width = m_width;
        height = m_height;

        forceMaximize = true;
    }
    else
    {
        if (maxW < m_width)
        {
            m_height = maxW / (m_width / (float)m_height);
            m_width = maxW;
        }

        if (maxH < m_height)
        {
            m_width = maxH / (m_height / (float)m_width);
            m_height = maxH;
        }
    }


    RECT rc;

    SetRect(&rc, 0, 0, m_width, m_height);
    AdjustWindowRect(
        &rc,
        WS_OVERLAPPEDWINDOW,
        false
    );

    // Create the window for our viewport.
    m_hwnd = CreateWindow(
        m_title.c_str(),
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (rc.right - rc.left), (rc.bottom - rc.top),
        0,
        NULL,
        hInstance,
        0
    );

    //if (forceMaximize)
    //{
        //ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
        //SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    //}
    //else
    //{
    ShowWindow(m_hwnd, SW_SHOW);
    //}

    if (forceMaximize)
    {
        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }

    m_input = new Input();
}

Window::~Window()
{
	CoUninitialize();
    UnregisterClassW(m_title.c_str(), (HINSTANCE)GetModuleHandleW(NULL));
    DestroyWindow(m_hwnd);

    delete m_input;
}

void Window::Loop()
{
    MSG  msg = {0};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            m_input->FirstUpdate();
            Update();
            m_input->LastUpdate();
        }
    }
}

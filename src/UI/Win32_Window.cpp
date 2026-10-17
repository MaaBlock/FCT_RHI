#include "../FCTAPI.h"
#include "./Win32_Window.h"

namespace FCT
{
    namespace
    {
        constexpr int kMouseButtonLeft = 0;
        constexpr int kMouseButtonRight = 1;
        constexpr int kActionRelease = 0;
        constexpr int kActionPress = 1;
    }

    std::wstring Win32_Window::utf8ToWide(const std::string& str)
    {
        if (str.empty())
        {
            return std::wstring();
        }
        int needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        if (needed <= 0)
        {
            return std::wstring();
        }
        std::wstring result(static_cast<size_t>(needed), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), needed);
        return result;
    }

    std::string Win32_Window::wideToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
        {
            return std::string();
        }
        int needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
        {
            return std::string();
        }
        std::string result(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), needed, nullptr, nullptr);
        return result;
    }

    Win32_Window::Win32_Window(Win32_UICommon* common, Runtime* rt)
        : m_hwnd(nullptr)
        , m_hInstance(GetModuleHandleW(nullptr))
        , m_windowStyle(WS_OVERLAPPEDWINDOW)
        , m_rt(rt)
        , m_common(common)
        , m_running(false)
#ifdef FCT_USE_VULKAN
        , m_vkSurface(nullptr)
#endif
        , m_swapchainNativeHandle(nullptr)
    {
        m_swapchain = nullptr;
        m_x = CW_USEDEFAULT;
        m_y = CW_USEDEFAULT;
        m_width = 800;
        m_height = 600;
        m_title = "FCT";
        m_ctx = nullptr;
    }

    Win32_Window::~Win32_Window()
    {
        FCT_SAFE_RELEASE(m_swapchain);

#ifdef FCT_USE_VULKAN
        if (m_vkSurface && m_ctx)
        {
            if (auto* vkCtx = dynamic_cast<VK_Context*>(m_ctx))
            {
                vkCtx->getVkInstance().destroySurfaceKHR(m_vkSurface);
                m_vkSurface = nullptr;
            }
        }
#endif

        if (m_hwnd && m_common)
        {
            HWND hwndToDestroy = m_hwnd;
            m_hwnd = nullptr;
            m_running = false;
            m_common->sendUiTask([hwndToDestroy](void*)
            {
                if (IsWindow(hwndToDestroy))
                {
                    SetWindowLongPtrW(hwndToDestroy, GWLP_USERDATA, 0);
                    DestroyWindow(hwndToDestroy);
                }
            });
        }
    }

    SwapchainTargetWrapper* Win32_Window::getSwapchainTarget(Context* srcCtx)
    {
        SwapchainTargetWrapper* ret = nullptr;
        m_ctx = srcCtx;

        if (false)
        {
        }
#ifdef FCT_USE_D3D12
        else if (dynamic_cast<D3D12_Context*>(srcCtx))
        {
            m_swapchainNativeHandle = static_cast<void*>(m_hwnd);
            ret = new Win32_SwapchainTargetWrapper(m_swapchainNativeHandle);
        }
#endif
#ifdef FCT_USE_VULKAN
        else if (auto* vkCtx = dynamic_cast<VK_Context*>(srcCtx))
        {
            vk::Win32SurfaceCreateInfoKHR createInfo{};
            createInfo.setHinstance(m_hInstance);
            createInfo.setHwnd(m_hwnd);
            try
            {
                m_vkSurface = static_cast<VkSurfaceKHR>(vkCtx->getVkInstance().createWin32SurfaceKHR(createInfo));
            }
            catch (const std::exception& e)
            {
                ferr << "Failed to create Win32 Vulkan surface: " << e.what() << std::endl;
            }
            m_swapchainNativeHandle = &m_vkSurface;
            ret = new Win32_SwapchainTargetWrapper(m_swapchainNativeHandle);
        }
#endif
        else
        {
            ferr << "没有受支持的context" << std::endl;
        }
        return ret;
    }

    void Win32_Window::createPlatform()
    {
        m_common->postUiTask([this](void*)
        {
            int clientWidth = (m_width > 0 && m_width != CW_USEDEFAULT) ? m_width : 800;
            int clientHeight = (m_height > 0 && m_height != CW_USEDEFAULT) ? m_height : 600;
            m_width = clientWidth;
            m_height = clientHeight;

            RECT rc{ 0, 0, clientWidth, clientHeight };
            AdjustWindowRect(&rc, m_windowStyle, FALSE);
            int windowWidth = rc.right - rc.left;
            int windowHeight = rc.bottom - rc.top;

            std::wstring wTitle = utf8ToWide(m_title);

            m_hwnd = CreateWindowExW(
                WS_EX_ACCEPTFILES,
                Win32_UICommon::getClassName(),
                wTitle.c_str(),
                m_windowStyle,
                m_x,
                m_y,
                windowWidth,
                windowHeight,
                nullptr,
                nullptr,
                m_hInstance,
                this);

            if (!m_hwnd)
            {
                ferr << "Failed to create Win32 window, error = " << GetLastError() << std::endl;
                m_running = false;
                return;
            }

            DragAcceptFiles(m_hwnd, TRUE);
            ShowWindow(m_hwnd, SW_SHOW);
            UpdateWindow(m_hwnd);
            m_running = true;
        });

        if (m_behavior)
        {
            delete m_behavior;
            m_behavior = nullptr;
        }
        m_behavior = new Win32_WindowBehavior(this);
    }

    uint32_t Win32_Window::addWndProcCallback(Win32WndProcCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_wndProcCallbackMutex);
        uint32_t id = m_nextWndProcCallbackId++;
        m_wndProcCallbacks[id] = std::move(callback);
        return id;
    }

    void Win32_Window::removeWndProcCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_wndProcCallbackMutex);
        m_wndProcCallbacks.erase(id);
    }

    LRESULT CALLBACK Win32_Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Win32_Window* pWnd = reinterpret_cast<Win32_Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pWnd = static_cast<Win32_Window*>(cs->lpCreateParams);
            if (pWnd)
            {
                pWnd->m_hwnd = hWnd;
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
            }
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }

        if (pWnd)
        {
            return pWnd->handleMessage(hWnd, message, wParam, lParam);
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    LRESULT Win32_Window::handleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        {
            std::lock_guard<std::mutex> lock(m_wndProcCallbackMutex);
            for (const auto& pair : m_wndProcCallbacks)
            {
                if (pair.second)
                {
                    LRESULT res = pair.second(hWnd, message, wParam, lParam);
                    if (res)
                    {
                        return res;
                    }
                }
            }
        }

        switch (message)
        {
        case WM_MOVE:
            m_x = GET_X_LPARAM(lParam);
            m_y = GET_Y_LPARAM(lParam);
            return 0;

        case WM_SIZE:
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            if (wParam != SIZE_MINIMIZED && newWidth > 0 && newHeight > 0)
            {
                if (m_width != newWidth || m_height != newHeight)
                {
                    m_width = newWidth;
                    m_height = newHeight;
                    invokeResizeCallbacks(newWidth, newHeight);
                }
            }
            return 0;
        }

        case WM_MOUSEMOVE:
            invokeMouseMoveCallbacks(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            invokeMouseCallbacks(kMouseButtonLeft, kActionPress, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:
            ReleaseCapture();
            invokeMouseCallbacks(kMouseButtonLeft, kActionRelease, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_RBUTTONDOWN:
            SetCapture(hWnd);
            invokeMouseCallbacks(kMouseButtonRight, kActionPress, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_RBUTTONUP:
            ReleaseCapture();
            invokeMouseCallbacks(kMouseButtonRight, kActionRelease, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            invokeScrollCallbacks(0, delta);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            invokeKeyCallbacks(static_cast<int>(wParam), kActionPress);
            return 0;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            invokeKeyCallbacks(static_cast<int>(wParam), kActionRelease);
            return 0;

        case WM_DROPFILES:
            handleDropFiles(reinterpret_cast<HDROP>(wParam));
            return 0;

        case WM_CLOSE:
            m_running = false;
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            m_running = false;
            m_hwnd = nullptr;
            PostQuitMessage(0);
            return 0;

        default:
            break;
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    void Win32_Window::handleDropFiles(HDROP hDrop)
    {
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        std::vector<std::string> filePaths;
        filePaths.reserve(fileCount);

        for (UINT i = 0; i < fileCount; ++i)
        {
            UINT charCount = DragQueryFileW(hDrop, i, nullptr, 0);
            if (charCount > 0)
            {
                std::wstring wPath(static_cast<size_t>(charCount) + 1, L'\0');
                DragQueryFileW(hDrop, i, wPath.data(), charCount + 1);
                wPath.resize(charCount);
                filePaths.push_back(wideToUtf8(wPath));
            }
        }
        DragFinish(hDrop);
        invokeFileDropCallbacks(filePaths);
    }

    void Win32_Window::invokeResizeCallbacks(int width, int height)
    {
        for (auto* cb : m_handlers)
        {
            cb->onResize(this, width, height);
        }
    }

    void Win32_Window::invokeMouseMoveCallbacks(int xpos, int ypos)
    {
        for (auto* cb : m_handlers)
        {
            cb->onMouseMove(this, xpos, ypos);
        }
    }

    void Win32_Window::invokeMouseCallbacks(int button, int action, int xpos, int ypos)
    {
        for (auto* cb : m_handlers)
        {
            if (action == kActionPress)
            {
                if (button == kMouseButtonLeft)
                {
                    cb->onLButtonDown(this, xpos, ypos);
                }
                else if (button == kMouseButtonRight)
                {
                    cb->onRButtonDown(this, xpos, ypos);
                }
            }
            else if (action == kActionRelease)
            {
                if (button == kMouseButtonLeft)
                {
                    cb->onLButtonUp(this, xpos, ypos);
                }
                else if (button == kMouseButtonRight)
                {
                    cb->onRButtonUp(this, xpos, ypos);
                }
            }
        }
    }

    void Win32_Window::invokeKeyCallbacks(int key, int action)
    {
        for (auto* cb : m_handlers)
        {
            if (action == kActionPress)
            {
                cb->onKeyDown(this, key);
            }
            else if (action == kActionRelease)
            {
                cb->onKeyUp(this, key);
            }
        }
    }

    void Win32_Window::invokeScrollCallbacks(int xoffset, int yoffset)
    {
        (void)xoffset;
        for (auto* cb : m_handlers)
        {
            cb->onMouseWheel(this, yoffset);
        }
    }

    void Win32_Window::invokeFileDropCallbacks(const std::vector<std::string>& filePaths)
    {
        for (auto* cb : m_handlers)
        {
            cb->onFileDrop(this, filePaths);
        }
    }

    bool Win32_Window::isRunning() const
    {
        return m_running.load();
    }

    void Win32_Window::swapBuffers()
    {
        if (isRunning())
        {
            if (m_swapchain)
            {
                m_swapchain->present();
            }
        }
    }

    int Win32_Window::getWidth()
    {
        if (m_hwnd)
        {
            RECT rc{};
            if (GetClientRect(m_hwnd, &rc))
            {
                int w = rc.right - rc.left;
                if (w > 0)
                {
                    m_width = w;
                }
            }
        }
        return m_width;
    }

    int Win32_Window::getHeight()
    {
        if (m_hwnd)
        {
            RECT rc{};
            if (GetClientRect(m_hwnd, &rc))
            {
                int h = rc.bottom - rc.top;
                if (h > 0)
                {
                    m_height = h;
                }
            }
        }
        return m_height;
    }

    void Win32_Window::setCursorPos(int x, int y)
    {
        if (m_hwnd)
        {
            POINT pt{ x, y };
            ClientToScreen(m_hwnd, &pt);
            SetCursorPos(pt.x, pt.y);
        }
    }

    Vec2 Win32_Window::getCursorPos() const
    {
        POINT pt{ 0, 0 };
        if (GetCursorPos(&pt) && m_hwnd)
        {
            ScreenToClient(m_hwnd, &pt);
        }
        return Vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }

    Image* Win32_Window::getImage() const
    {
        return nullptr;
    }

    void Win32_Window::recreateSwapchain(int width, int height)
    {
        if (!m_swapchain)
        {
            m_swapchain = m_ctx->createResource<RHI::Swapchain>();
        }
        m_swapchain->nativeHandle(m_swapchainNativeHandle);
        m_swapchain->size(width, height);
        m_swapchain->create();
    }
}

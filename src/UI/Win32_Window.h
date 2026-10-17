#ifndef FCT_WIN32_WINDOW_H
#define FCT_WIN32_WINDOW_H

#include "../ThirdParty.h"
#include "./Window.h"
#include "./Win32_UICommon.h"
#include "../Runtime/runtime.h"

namespace FCT
{
    class Win32_SwapchainTargetWrapper : public SwapchainTargetWrapper
    {
    public:
        explicit Win32_SwapchainTargetWrapper(void* surface) : m_surface(surface) {}
        void* getNativeHandler() override
        {
            return m_surface;
        }
    private:
        void* m_surface;
    };

    using Win32WndProcCallback = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

    class Win32_WindowBehavior;

    class Win32_Window : public Window
    {
    private:
        SwapchainTargetWrapper* getSwapchainTarget(Context* ctx) override;

    public:
        friend class Win32_WindowBehavior;

        Win32_Window(Win32_UICommon* common, Runtime* rt);
        ~Win32_Window() override;

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        static std::wstring utf8ToWide(const std::string& str);
        static std::string wideToUtf8(const std::wstring& wstr);

        uint32_t addWndProcCallback(Win32WndProcCallback callback);
        void removeWndProcCallback(uint32_t id);

        void invokeResizeCallbacks(int width, int height);
        void invokeMouseMoveCallbacks(int xpos, int ypos);
        void invokeMouseCallbacks(int button, int action, int xpos, int ypos);
        void invokeKeyCallbacks(int key, int action);
        void invokeScrollCallbacks(int xoffset, int yoffset);
        void invokeFileDropCallbacks(const std::vector<std::string>& filePaths);

        void createPlatform() override;
        bool isRunning() const override;
        void swapBuffers() override;
        int getWidth() override;
        int getHeight() override;
        void setCursorPos(int x, int y) override;
        Vec2 getCursorPos() const override;
        Image* getImage() const override;

        HWND getHwnd() const { return m_hwnd; }
        HINSTANCE getHInstance() const { return m_hInstance; }

        void postUiTask(UITaskFunction task, void* param = nullptr, bool waited = true)
        {
            m_common->postUiTask(task, param, waited);
        }

        uint32_t postTicker(UiTicker ticker)
        {
            return m_common->postTicker(ticker);
        }

        void sendUiTask(UITaskFunction task, void* param = nullptr, bool waited = true)
        {
            m_common->sendUiTask(task, param, waited);
        }

    private:
        LRESULT handleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void handleDropFiles(HDROP hDrop);
        void recreateSwapchain(int width, int height);

    private:
        HWND m_hwnd;
        HINSTANCE m_hInstance;
        DWORD m_windowStyle;
        Runtime* m_rt;
        Win32_UICommon* m_common;
        std::atomic<bool> m_running{false};
        std::mutex m_wndProcCallbackMutex;
        std::unordered_map<uint32_t, Win32WndProcCallback> m_wndProcCallbacks;
        uint32_t m_nextWndProcCallbackId{1};
#ifdef FCT_USE_VULKAN
        VkSurfaceKHR m_vkSurface;
#endif
        void* m_swapchainNativeHandle;
    };

    class Win32_WindowBehavior : public WindowBehavior
    {
    public:
        explicit Win32_WindowBehavior(Win32_Window* wnd) : m_wnd(wnd) {}

        void pos(int x, int y) override
        {
            m_wnd->m_x = x;
            m_wnd->m_y = y;
            if (m_wnd->m_hwnd)
            {
                m_wnd->addRef();
                m_wnd->m_common->postUiTask([this, x, y](void*)
                {
                    if (m_wnd->m_hwnd)
                    {
                        SetWindowPos(m_wnd->m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    m_wnd->release();
                }, nullptr, false);
            }
        }

        void size(int w, int h) override
        {
            m_wnd->m_width = w;
            m_wnd->m_height = h;
            if (m_wnd->m_hwnd)
            {
                m_wnd->addRef();
                m_wnd->m_common->postUiTask([this, w, h](void*)
                {
                    if (m_wnd->m_hwnd)
                    {
                        RECT rc{ 0, 0, w, h };
                        AdjustWindowRect(&rc, m_wnd->m_windowStyle, FALSE);
                        SetWindowPos(m_wnd->m_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
                    }
                    m_wnd->release();
                }, nullptr, false);
            }
        }

        void title(const std::string& title) override
        {
            m_wnd->m_title = title;
            if (m_wnd->m_hwnd)
            {
                m_wnd->addRef();
                m_wnd->m_common->postUiTask([this, title](void*)
                {
                    if (m_wnd->m_hwnd)
                    {
                        std::wstring wTitle = Win32_Window::utf8ToWide(title);
                        SetWindowTextW(m_wnd->m_hwnd, wTitle.c_str());
                    }
                    m_wnd->release();
                }, nullptr, false);
            }
        }

    private:
        Win32_Window* m_wnd;
    };
}

#endif // FCT_WIN32_WINDOW_H

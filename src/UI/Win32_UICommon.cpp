#include "../FCTAPI.h"
#include "./Win32_UICommon.h"
#include "./Win32_Window.h"

namespace FCT
{
    Win32_UICommon::Win32_UICommon()
    {
        m_running = true;
        std::atomic<bool> inited{false};
        m_uiThread = std::thread([this, &inited]()
        {
            tracy::SetThreadName("Win32 UI Thread");
            m_uiThreadId = std::this_thread::get_id();
            registerWindowClass();
            inited = true;

            while (m_running)
            {
                processTaskBatch();
                runTickers();
                pumpMessages();
            }

            unregisterWindowClass();
        });

        while (!inited)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    Win32_UICommon::~Win32_UICommon()
    {
        m_running = false;
        if (m_uiThread.joinable())
        {
            m_uiThread.join();
        }
    }

    void Win32_UICommon::registerWindowClass()
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = Win32_Window::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = GetModuleHandleW(nullptr);
        wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = getClassName();
        wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        RegisterClassExW(&wcex);
    }

    void Win32_UICommon::unregisterWindowClass()
    {
        UnregisterClassW(getClassName(), GetModuleHandleW(nullptr));
    }

    void Win32_UICommon::processTaskBatch()
    {
        uint8_t count = 0;
        while (!m_taskQueue.empty() && m_running && count < 10)
        {
            ZoneScopedN("Win32 UITask");
            UITaskTrans trans;
            if (m_taskQueue.pop(trans) && trans.data)
            {
                UiTaskData* task = trans.data;
                task->task(task->param);
                *task->waiting = false;
                delete task;
                ++count;
            }
        }
    }

    void Win32_UICommon::runTickers()
    {
        for (auto& [id, ticker] : m_ticker)
        {
            if (ticker)
            {
                ticker();
            }
        }
    }

    void Win32_UICommon::pumpMessages()
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void Win32_UICommon::postUiTask(UITaskFunction task, void* param, bool waited)
    {
        UiTaskData* data = new UiTaskData();
        std::shared_ptr<bool> waiting = std::make_shared<bool>(waited);
        data->task = std::move(task);
        data->param = param;
        data->waiting = waiting;
        m_taskQueue.push(UITaskTrans(data));
        while (*waiting)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }
}

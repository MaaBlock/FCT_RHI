#ifndef FCT_WIN32_UICOMMON_H
#define FCT_WIN32_UICOMMON_H

#include "../ThirdParty.h"

namespace FCT
{
#ifndef FCT_UI_TASK_TYPES_H
#define FCT_UI_TASK_TYPES_H
    using UITaskFunction = std::function<void(void *)>;
    struct UiTaskData {
        UITaskFunction task;
        void *param;
        std::shared_ptr<bool> waiting;
    };
    struct UITaskTrans
    {
        UITaskTrans() = default;
        UITaskTrans(UiTaskData* data) : data(data) {}
        UiTaskData* data;
    };
    using UiTicker = std::function<void(void)>;
#endif

    class Win32_UICommon
    {
    public:
        Win32_UICommon();
        ~Win32_UICommon();

        void postUiTask(UITaskFunction task, void* param = nullptr, bool waited = true);

        uint32_t postTicker(UiTicker ticker)
        {
            uint32_t id = ++m_tickerId;
            postUiTask([this, ticker, id](void*)
            {
                m_ticker[id] = ticker;
            });
            return id;
        }

        void sendUiTask(UITaskFunction task, void* param = nullptr, bool waited = true)
        {
            if (m_uiThreadId == std::this_thread::get_id())
            {
                task(param);
            }
            else
            {
                postUiTask(task, param, waited);
            }
        }

        static constexpr const wchar_t* getClassName()
        {
            return L"FCT_Win32_WindowClass";
        }

    private:
        void registerWindowClass();
        void unregisterWindowClass();
        void processTaskBatch();
        void runTickers();
        void pumpMessages();

    private:
        std::thread m_uiThread;
        std::atomic<bool> m_running{false};
        boost::lockfree::queue<UITaskTrans, boost::lockfree::capacity<1024>> m_taskQueue;
        std::unordered_map<uint32_t, UiTicker> m_ticker;
        uint32_t m_tickerId = 0;
        std::thread::id m_uiThreadId;
    };
}

#endif // FCT_WIN32_UICOMMON_H

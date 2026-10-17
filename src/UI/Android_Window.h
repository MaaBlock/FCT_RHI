#include "../ThirdParty.h"
#include "Window.h"
#include "../Runtime/ActivityWrapper.h"

#ifndef ANDROID_WINDOW_H
#define ANDROID_WINDOW_H
namespace FCT {
    class Android_SwapchainTargetWrapper : public SwapchainTargetWrapper
    {
    public:
        Android_SwapchainTargetWrapper(void* surface) : m_surface(surface) {}
        void* getNativeHandler() override
        {
            return m_surface;
        }
    private:
        void* m_surface;
    };

    class Android_WindowBehavior;

    class Android_Window : public Window {
    private:
        SwapchainTargetWrapper* getSwapchainTarget(Context* ctx) override;
    public:
        friend class Android_WindowBehavior;
        Android_Window();
        Android_Window(JavaVM* vm);
        ~Android_Window() override;

        void createPlatform() override;
        bool isRunning() const override;
        void swapBuffers() override;
        int getWidth() override;
        int getHeight() override;
        uint32_t getDeviceWidth();
        uint32_t getDeviceHeight();
        void onDestory();

        void setCursorPos(int x, int y) override;
        Vec2 getCursorPos() const override;
        Image* getImage() const override;

        void invokeResizeCallbacks(int width, int height);
        void invokeMouseMoveCallbacks(int xpos, int ypos);
        void invokeMouseCallbacks(int button, int action, int mods);
        void invokeKeyCallbacks(int key, int scancode, int action, int mods);
        void invokeScrollCallbacks(int xoffset, int yoffset);

        void destroy();
        bool isCreated() const;
        bool hasSurface() const;

        void destroySurface();
        void viewport(Vec2 lt, Vec2 rb);
        void sizeParam(int w, int h);
        void resize(int w, int h);
        void onSurfaceCreate(jobject surface);
        void onSurfaceChanged(int width, int height);
        void onSurfaceDestroy();
    private:
        jni::GlobalObject<FCTWindowJavaClass>* m_windowObject = nullptr;
        JNIEnv* getEnv() {
            JNIEnv* env = nullptr;
            if (m_vm) {
                m_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
            }
            return env;
        }
        JavaVM* m_vm = nullptr;
        std::atomic<bool> m_hasSurface{false};
        bool m_running = false;
        bool m_created = false;
        Vec2 m_lt;
        Vec2 m_rb;
        Vec2 m_cursorPos{0.0f, 0.0f};
        ANativeWindow* m_nativeWindow = nullptr;
        bool m_needRecreateSwapChain = false;
        void* m_swapchainNativeHandle = nullptr;
#ifdef FCT_USE_VULKAN
        void createVulkanSurface();
        void destroyVulkanSurface();
        vk::SurfaceKHR m_vulkanSurface;
#endif
        std::mutex m_nativeSurfaceMutex;
    };

    class Android_WindowBehavior : public WindowBehavior {
    public:
        Android_WindowBehavior(Android_Window* wnd) : m_wnd(wnd) {}
        void pos(int x, int y) override
        {
            (void)x;
            (void)y;
        }
        void size(int w, int h) override
        {
            (void)w;
            (void)h;
        }
        void title(const std::string& title) override
        {
            (void)title;
        }
    private:
        Android_Window* m_wnd;
    };
}

#endif //ANDROID_WINDOW_H

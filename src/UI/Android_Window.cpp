#include "../FCTAPI.h"
#include "Android_Window.h"

namespace FCT {

    SwapchainTargetWrapper* Android_Window::getSwapchainTarget(Context* srcCtx) {
        SwapchainTargetWrapper* ret = nullptr;
        m_ctx = srcCtx;
#ifdef FCT_USE_VULKAN
        if (dynamic_cast<VK_Context*>(srcCtx)) {
            while (!hasSurface()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            std::lock_guard<std::mutex> lock(m_nativeSurfaceMutex);
            createVulkanSurface();
            m_swapchainNativeHandle = &m_vulkanSurface;
            ret = new Android_SwapchainTargetWrapper(m_swapchainNativeHandle);
        } else
#endif
        {
            ferr << "没有受支持的context" << std::endl;
        }
        return ret;
    }

    bool Android_Window::isRunning() const {
        return m_running;
    }

    void Android_Window::swapBuffers() {
        if (isRunning() && hasSurface()) {
            if (m_swapchain) {
                m_swapchain->present();
            }
        }
    }

    int Android_Window::getWidth() {
        if (m_nativeWindow) {
            int w = ANativeWindow_getWidth(m_nativeWindow);
            if (w > 0) {
                m_width = w;
            }
        }
        return m_width;
    }

    int Android_Window::getHeight() {
        if (m_nativeWindow) {
            int h = ANativeWindow_getHeight(m_nativeWindow);
            if (h > 0) {
                m_height = h;
            }
        }
        return m_height;
    }

    void Android_Window::setCursorPos(int x, int y) {
        m_cursorPos = Vec2(static_cast<float>(x), static_cast<float>(y));
    }

    Vec2 Android_Window::getCursorPos() const {
        return m_cursorPos;
    }

    Image* Android_Window::getImage() const {
        return nullptr;
    }

    void Android_Window::invokeResizeCallbacks(int width, int height) {
        for (auto cb : m_handlers) {
            cb->onResize(this, width, height);
        }
    }

    void Android_Window::invokeMouseMoveCallbacks(int xpos, int ypos) {
        m_cursorPos = Vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        for (auto cb : m_handlers) {
            cb->onMouseMove(this, xpos, ypos);
        }
    }

    void Android_Window::invokeMouseCallbacks(int button, int action, int mods) {
        (void)button;
        (void)action;
        (void)mods;
    }

    void Android_Window::invokeKeyCallbacks(int key, int scancode, int action, int mods) {
        (void)key;
        (void)scancode;
        (void)action;
        (void)mods;
    }

    void Android_Window::invokeScrollCallbacks(int xoffset, int yoffset) {
        (void)xoffset;
        for (auto cb : m_handlers) {
            cb->onMouseWheel(this, yoffset);
        }
    }

    void Android_Window::createPlatform() {
        g_androidRuntime->getUICommon()->postUiTask([this](void*) {
            jni::GlobalObject<FCTMainActivityJavaClass>* activity = g_androidRuntime->getActivity();
            m_windowObject = new jni::GlobalObject<FCTWindowJavaClass>(
                activity->Call<"createWindow">(jlong(this), m_x, m_y, m_width, m_height));
            aout << "创建窗口" << std::endl;
        });
        while (!hasSurface()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (m_behavior) {
            delete m_behavior;
            m_behavior = nullptr;
        }
        m_behavior = new Android_WindowBehavior(this);
        m_created = true;
        m_running = true;
    }

    void Android_Window::destroy() {
        m_running = false;
    }

    bool Android_Window::isCreated() const {
        return m_created;
    }

    void Android_Window::destroySurface() {
        m_hasSurface = false;
        std::lock_guard<std::mutex> lock(m_nativeSurfaceMutex);
        if (m_nativeWindow) {
            ANativeWindow_release(m_nativeWindow);
            m_nativeWindow = nullptr;
        }
    }

    void Android_Window::viewport(Vec2 lt, Vec2 rb) {
        m_lt = lt;
        m_rb = rb;
    }

    uint32_t Android_Window::getDeviceWidth() {
        return g_androidRuntime ? static_cast<uint32_t>(g_androidRuntime->getDeviceWidth()) : 0;
    }

    uint32_t Android_Window::getDeviceHeight() {
        return g_androidRuntime ? static_cast<uint32_t>(g_androidRuntime->getDeviceHeight()) : 0;
    }

    Android_Window::Android_Window(JavaVM* vm) {
        m_x = 0;
        m_y = 0;
        m_width = 800;
        m_height = 600;
        m_created = false;
        m_running = false;
        m_hasSurface = false;
        m_nativeWindow = nullptr;
        m_vm = vm;
        m_needRecreateSwapChain = true;
    }

    bool Android_Window::hasSurface() const {
        return m_hasSurface.load();
    }

    Android_Window::Android_Window() {
        m_x = 0;
        m_y = 0;
        m_width = 800;
        m_height = 600;
        m_created = false;
        m_running = false;
        m_hasSurface = false;
        m_nativeWindow = nullptr;
        m_needRecreateSwapChain = false;
    }

    Android_Window::~Android_Window() {
        FCT_SAFE_RELEASE(m_swapchain);
#ifdef FCT_USE_VULKAN
        destroyVulkanSurface();
#endif
        destroySurface();
    }

    void Android_Window::sizeParam(int w, int h) {
        m_width = w;
        m_height = h;
    }

    void Android_Window::resize(int w, int h) {
        m_width = w;
        m_height = h;
        invokeResizeCallbacks(w, h);
    }

    void Android_Window::onDestory() {
        m_running = false;
    }

    void Android_Window::onSurfaceCreate(jobject surface) {
        std::lock_guard<std::mutex> lock(m_nativeSurfaceMutex);
        if (m_nativeWindow) {
            ANativeWindow_release(m_nativeWindow);
            m_nativeWindow = nullptr;
        }
        m_nativeWindow = ANativeWindow_fromSurface(g_androidRuntime->getEnv(), surface);
        ANativeWindow_acquire(m_nativeWindow);
        fout << "onSurfaceCreate" << std::endl;

        int w = ANativeWindow_getWidth(m_nativeWindow);
        int h = ANativeWindow_getHeight(m_nativeWindow);
        if (w > 0 && h > 0) {
            m_width = w;
            m_height = h;
        }

        fout << "NativeWindow width: " << w << std::endl;
        fout << "NativeWindow height: " << h << std::endl;

        m_needRecreateSwapChain = true;
        m_hasSurface = true;
    }

    void Android_Window::onSurfaceChanged(int width, int height) {
        fout << "onSurfaceChanged: " << width << "x" << height << std::endl;
        if (width > 0 && height > 0) {
            m_width = width;
            m_height = height;
            invokeResizeCallbacks(width, height);
        }
    }

    void Android_Window::onSurfaceDestroy() {
        std::lock_guard<std::mutex> lock(m_nativeSurfaceMutex);
        if (m_nativeWindow) {
            ANativeWindow_release(m_nativeWindow);
            m_nativeWindow = nullptr;
        }
        fout << "onSurfaceDestroy" << std::endl;
        m_hasSurface = false;
    }

#ifdef FCT_USE_VULKAN
    void Android_Window::createVulkanSurface() {
        if (m_vulkanSurface) {
            destroyVulkanSurface();
        }
        vk::AndroidSurfaceCreateInfoKHR createInfo;
        createInfo.setWindow(m_nativeWindow);

        auto vkInstance = (dynamic_cast<VK_Context*>(m_ctx))->getVkInstance();
        m_vulkanSurface = vkInstance.createAndroidSurfaceKHR(createInfo);
    }

    void Android_Window::destroyVulkanSurface() {
        if (m_vulkanSurface && m_ctx) {
            auto vkInstance = (dynamic_cast<VK_Context*>(m_ctx))->getVkInstance();
            vkInstance.destroySurfaceKHR(m_vulkanSurface);
            m_vulkanSurface = nullptr;
        }
    }
#endif
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTWindow_onDismiss(JNIEnv *env, jobject thiz) {
    (void)env;
    jni::LocalObject<FCTWindowJavaClass> window(thiz);
    FCT::Android_Window* pThis = (FCT::Android_Window*)window.Call<"getNativeWindowPtr">();
    FCT::fout << "窗口关闭" << std::endl;
    pThis->onDestory();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTSurfaceView_onSUrfaceDestroyed(JNIEnv *env, jobject thiz,
                                                                   jlong native_window_ptr) {
    (void)env;
    (void)thiz;
    FCT::Android_Window* wnd = (FCT::Android_Window*)native_window_ptr;
    wnd->onSurfaceDestroy();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTSurfaceView_onSurfaceCreated(JNIEnv *env, jobject thiz,
                                                                 jobject surface,
                                                                 jlong native_window_ptr) {
    (void)env;
    (void)thiz;
    FCT::Android_Window* wnd = (FCT::Android_Window*)native_window_ptr;
    wnd->onSurfaceCreate(surface);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTSurfaceView_onSurfaceChanged(JNIEnv *env, jobject thiz,
                                                                 jlong native_window_ptr,
                                                                 jint width, jint height) {
    (void)env;
    (void)thiz;
    FCT::Android_Window* wnd = (FCT::Android_Window*)native_window_ptr;
    wnd->onSurfaceChanged(width, height);
}
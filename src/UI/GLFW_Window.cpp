//
// Created by Administrator on 2025/3/22.
//
#include "../FCTAPI.h"
namespace FCT
{
#ifdef FCT_USE_VULKAN
/*    void GLFW_Window::createVulkanSwapChain() {
        if (!m_vkSwapchain)
            m_vkSwapchain = new VK_Swapchain((VK_Context*)m_ctx);
        m_vkSwapchain->create(m_vkSurface,m_width,m_height);
    }

    void GLFW_Window::presentVulkan()
    {
        m_vkSwapchain->present();
    }*/
#endif
}

FCT::SwapchainTargetWrapper* FCT::GLFW_Window::getSwapchainTarget(Context* srcCtx)
{
    FCT::SwapchainTargetWrapper* ret = nullptr;
    m_ctx = srcCtx;
    if (false) {

    }
#ifdef FCT_USE_VULKAN
    else if (dynamic_cast<VK_Context*>(srcCtx))
    {
        auto ctx = dynamic_cast<VK_Context*>(srcCtx);
        auto res = glfwCreateWindowSurface(ctx->getVkInstance(), m_window, nullptr, &m_vkSurface);
        if (res!= VK_SUCCESS)
        {
            ferr << "Failed to create Vulkan surface. Error code: " << res << std::endl;
        }
        m_swapchainNativeHandle = &m_vkSurface;
        ret = new GLFW_SwapchainTargetWrapper(m_swapchainNativeHandle);
    }
#endif
#ifdef FCT_USE_D3D12
    else if (dynamic_cast<D3D12_Context*>(srcCtx))
    {
        m_swapchainNativeHandle = static_cast<void*>(glfwGetWin32Window(m_window));
        ret = new GLFW_SwapchainTargetWrapper(m_swapchainNativeHandle);
    }
#endif
    else {
        ferr << "没有受支持的context" << std::endl;
    }
    return ret;
}

FCT::GLFW_Window::GLFW_Window(GLFW_UICommon* common, Runtime* rt)
{
    m_common = common;
    m_rt = rt;
    m_swapchain = nullptr;
#ifdef FCT_USE_VULKAN
    m_vkSurface = nullptr;
#endif
    m_window = nullptr;
    m_width = 0;
    m_height = 0;
    m_title = "FCT";
    m_ctx = nullptr;
    m_swapchainNativeHandle = nullptr;
}

FCT::GLFW_Window::~GLFW_Window()
{
    FCT_SAFE_RELEASE(m_swapchain);
    /*
    if (m_vkSwapchain)
    {
        m_vkSwapchain->destroy();
        delete m_vkSwapchain;
    }
    */
}

void FCT::GLFW_Window::invokeResizeCallbacks(int width, int height)
{
	for (auto cb : m_handlers) {
		cb->onResize(this, width, height);
	}
}

void FCT::GLFW_Window::invokeMouseMoveCallbacks(int xpos, int ypos)
{
    for (auto cb : m_handlers) {
        cb->onMouseMove(this, static_cast<int>(xpos), static_cast<int>(ypos));
    }
}

void FCT::GLFW_Window::invokeMouseCallbacks(int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);

    for (auto cb : m_handlers) {
        if (action == GLFW_PRESS) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                cb->onLButtonDown(this, static_cast<int>(xpos), static_cast<int>(ypos));
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                cb->onRButtonDown(this, static_cast<int>(xpos), static_cast<int>(ypos));
            }
        } else if (action == GLFW_RELEASE) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                cb->onLButtonUp(this, static_cast<int>(xpos), static_cast<int>(ypos));
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                cb->onRButtonUp(this, static_cast<int>(xpos), static_cast<int>(ypos));
            }
        }
    }
}

void FCT::GLFW_Window::invokeKeyCallbacks(int key, int scancode, int action, int mods)
{
	for (auto cb : m_handlers) {
        if (action == GLFW_PRESS) {
            cb->onKeyDown(this, key);
        } else if (action == GLFW_RELEASE) {
            cb->onKeyUp(this, key);
        }
    }
}

void FCT::GLFW_Window::invokeScrollCallbacks(int xoffset, int yoffset)
{
	for (auto cb : m_handlers) {
        cb->onMouseWheel(this, yoffset);
    }
}

void FCT::GLFW_Window::invokeFileDropCallbacks(int count, const char** paths)
{
    std::vector<std::string> filePaths;
    for (int i = 0; i < count; ++i)
        filePaths.push_back(paths[i]);
    for (auto cb : m_handlers) {
        cb->onFileDrop(this, filePaths);
    }
}

void FCT::GLFW_Window::createPlatform()
{
    m_common->postUiTask([this](void*)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window,this);
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
            auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));\
            wnd->invokeResizeCallbacks(width, height);
            });
        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
            auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));\
            wnd->invokeMouseCallbacks(button, action, mods);
            });
        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
            auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));\
            wnd->invokeMouseMoveCallbacks(static_cast<int>(xpos), static_cast<int>(ypos));
            });
        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));
            wnd->invokeKeyCallbacks(key, scancode, action, mods);
        });
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
           auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));
           wnd->invokeScrollCallbacks(xoffset, yoffset);
       });
        glfwSetDropCallback(m_window, [](GLFWwindow* window, int count, const char** paths) {
         auto* wnd = static_cast<FCT::GLFW_Window*>(glfwGetWindowUserPointer(window));
         wnd->invokeFileDropCallbacks(count, paths);
        });
    });
    if (m_behavior)
    {
        delete m_behavior;
        m_behavior = nullptr;
    }
    m_behavior = new GLFW_WindowBehavior(this);
}


bool FCT::GLFW_Window::isRunning() const
{
	return !glfwWindowShouldClose(m_window);
}

void FCT::GLFW_Window::swapBuffers()
{
    if (isRunning())
    {
        if (m_swapchain)
        {
            m_swapchain->present();
        }
    }
	//glfwSwapBuffers(m_window);
}

int FCT::GLFW_Window::getWidth()
{
	int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return width;
}

int FCT::GLFW_Window::getHeight()
{
	int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return height;

}

void FCT::GLFW_Window::setCursorPos(int x, int y)
{
    glfwSetCursorPos(m_window, x, y);
}

FCT::Image* FCT::GLFW_Window::getImage() const
{
    return nullptr;
}

FCT::Vec2 FCT::GLFW_Window::getCursorPos() const
{
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    return FCT::Vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

/*
std::vector<FCT::Image*> FCT::GLFW_Window::getTargetImages()
{
    return std::vector<FCT::Image*>();
}
*/




void FCT::GLFW_Window::recreateSwapchain(int width, int height)
{
    if (!m_swapchain)
    {
        m_swapchain = m_ctx->createResource<RHI::Swapchain>();
    }
    m_swapchain->nativeHandle(m_swapchainNativeHandle);
    m_swapchain->size(width, height);
    m_swapchain->create();
}


#include "./WGL2_Swapchain.h"
#include "./WGL2_Image.h"
#include "./WGL2_Semaphore.h"
#include "../Context/WGL2_Context.h"
#include "../Context/SingleBufferImage.h"
#include "../Context/MutilBufferImage.h"
#include "../Context/ImageRenderTarget.h"
#include <GLFW/glfw3.h>

namespace FCT::RHI {

WGL2_Swapchain::WGL2_Swapchain(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_Swapchain::~WGL2_Swapchain()
{
    cleanupOldResources();
    if (m_presentFinishSemaphore) {
        m_presentFinishSemaphore->release();
        m_presentFinishSemaphore = nullptr;
    }
    if (m_imageAvailableSemaphore) {
        m_imageAvailableSemaphore->release();
        m_imageAvailableSemaphore = nullptr;
    }
}

void WGL2_Swapchain::cleanupOldResources()
{
    if (m_fctImage) {
        m_fctImage->release();
        m_fctImage = nullptr;
    }
    if (m_depthStencilImage) {
        m_depthStencilImage->release();
        m_depthStencilImage = nullptr;
    }
    if (m_target) {
        m_target->release();
        m_target = nullptr;
    }
    if (m_backbufferImage) {
        m_backbufferImage->release();
        m_backbufferImage = nullptr;
    }
    m_fctImages.clear();
}

void WGL2_Swapchain::enableDepthBuffer(Format format)
{
    // WebGL 2.0 默认 Framebuffer 包含深度/模板缓冲
}

void WGL2_Swapchain::create()
{
    cleanupOldResources();

    m_width = (m_pendingWidth > 0) ? m_pendingWidth : 800;
    m_height = (m_pendingHeight > 0) ? m_pendingHeight : 600;

    m_backbufferImage = FCT_NEW(WGL2_Image, m_ctx);
    m_backbufferImage->format(m_format);
    m_backbufferImage->width(m_width);
    m_backbufferImage->height(m_height);
    m_backbufferImage->createFromExisting(0, GL_TEXTURE_2D, false);

    m_fctImages.push_back(m_backbufferImage);
    bindSwapchainImages(m_ctx);

    if (!m_imageAvailableSemaphore) {
        m_imageAvailableSemaphore = FCT_NEW(WGL2_Semaphore, m_ctx);
        m_imageAvailableSemaphore->create();
    }
}

void WGL2_Swapchain::present()
{
#if defined(__EMSCRIPTEN__)
    glFlush();
#else
    const void* CanvasMagicId = reinterpret_cast<void*>(uintptr_t(0x43414E56));
    if (m_isCanvas || m_nativeHandle == CanvasMagicId) {
        return;
    }
    if (m_nativeHandle != nullptr) {
        GLFWwindow* window = static_cast<GLFWwindow*>(m_nativeHandle);
        glfwSwapBuffers(window);
    }
#endif
}

bool WGL2_Swapchain::acquireFirstImage()
{
    m_currentImageIndex = 0;
    return true;
}

RHI::Semaphore* WGL2_Swapchain::getImageAvailableSemaphore()
{
    return m_imageAvailableSemaphore;
}

void WGL2_Swapchain::setPresentFinshSemaphore(RHI::Semaphore* semaphore)
{
    if (m_presentFinishSemaphore) {
        m_presentFinishSemaphore->release();
    }
    m_presentFinishSemaphore = dynamic_cast<WGL2_Semaphore*>(semaphore);
    if (m_presentFinishSemaphore) {
        m_presentFinishSemaphore->addRef();
    }
}

bool WGL2_Swapchain::doRecreate()
{
    create();
    return true;
}

} // namespace FCT::RHI

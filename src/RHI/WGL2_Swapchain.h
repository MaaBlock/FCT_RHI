#pragma once

#include "./Swapcain.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;
class SingleBufferImage;

namespace RHI {

class WGL2_Image;
class WGL2_Semaphore;

class WGL2_Swapchain : public Swapchain {
public:
    explicit WGL2_Swapchain(WGL2_Context* ctx);
    ~WGL2_Swapchain() override;

    uint32_t getImageCount() const override { return 1; }
    void enableDepthBuffer(Format format) override;
    void create() override;
    void present() override;
    Format getFormat() const override { return m_format; }
    bool acquireFirstImage() override;
    RHI::Semaphore* getImageAvailableSemaphore() override;
    void setPresentFinshSemaphore(RHI::Semaphore* semaphore) override;

    void setCanvasTarget(bool isCanvas) noexcept { m_isCanvas = isCanvas; }
    bool isCanvasTarget() const noexcept { return m_isCanvas; }

protected:
    bool doRecreate() override;

private:
    void cleanupOldResources();

    WGL2_Context* m_ctx{nullptr};
    Format m_format{Format::R8G8B8A8_UNORM};
    WGL2_Semaphore* m_presentFinishSemaphore{nullptr};
    WGL2_Semaphore* m_imageAvailableSemaphore{nullptr};
    WGL2_Image* m_backbufferImage{nullptr};
    bool m_isCanvas{false};
};

} // namespace RHI
} // namespace FCT

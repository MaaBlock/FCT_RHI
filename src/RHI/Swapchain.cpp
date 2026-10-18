#include "../FCTAPI.h"
#include "./Swapcain.h"
#include "../Context/Context.h"
#include "../Context/Context.hpp"
#include "../Context/MutilBufferImage.h"
#include "../Context/ImageRenderTarget.h"

namespace FCT
{
    namespace RHI
    {
        Swapchain::Swapchain()
            : m_fctImage(nullptr)
            , m_depthStencilImage(nullptr)
            , m_target(nullptr)
            , m_currentImageIndex(0)
            , m_recreatedFlag(false)
            , m_pendingWidth(0)
            , m_pendingHeight(0)
            , m_nativeHandle(nullptr)
            , m_width(0)
            , m_height(0)
            , m_dstWidth(0)
            , m_dstHeight(0)
            , m_hasRecreatedInThisFrame(false)
        {
        }

        FCT::Image* Swapchain::image() const
        {
            return m_fctImage;
        }

        void Swapchain::needRecreate(int width, int height)
        {
            m_lastRecreateRequestTime = std::chrono::high_resolution_clock::now();
            m_recreatedFlag = true;
            m_pendingWidth = static_cast<uint32_t>(width > 0 ? width : 0);
            m_pendingHeight = static_cast<uint32_t>(height > 0 ? height : 0);
        }

        void Swapchain::sync()
        {
            ZoneScoped();
            m_hasRecreatedInThisFrame = false;
            while (m_recreatedFlag)
            {
                using namespace std::chrono;
                auto now = high_resolution_clock::now();
                auto timeSinceRequest = duration_cast<milliseconds>(
                    now - m_lastRecreateRequestTime
                ).count();
                while (timeSinceRequest < RECREATE_DEBOUNCE_MS || !m_pendingWidth || !m_pendingHeight)
                {
                    now = high_resolution_clock::now();
                    timeSinceRequest = duration_cast<milliseconds>(
                        now - m_lastRecreateRequestTime
                    ).count();
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                if (doRecreate())
                {
                    now = high_resolution_clock::now();
                    timeSinceRequest = duration_cast<milliseconds>(
                        now - m_lastRecreateRequestTime
                    ).count();
                    if (timeSinceRequest > RECREATE_DEBOUNCE_MS &&
                        m_pendingWidth == m_dstWidth &&
                        m_pendingHeight == m_dstHeight)
                    {
                        m_recreatedFlag = false;
                    }
                }
                m_hasRecreatedInThisFrame = true;
            }
            if (m_hasRecreatedInThisFrame)
            {
                trigger(SwapchainEvent::Recreate{
                    m_width, m_height,
                    this
                });
            }
        }

        void Swapchain::bindSwapchainImages(FCT::Context* ctx)
        {
            if (m_fctImage)
            {
                m_fctImage->create(m_fctImages);
            }
            else
            {
                m_fctImage = FCT_NEW(MutilBufferImage, ctx);
                m_fctImage->renderTargetType(RenderTargetType::WindowTarget);
                m_fctImage->as(ImageUsage::RenderTarget);
                m_fctImage->create(m_fctImages);
            }

            if (!m_target)
            {
                m_target = FCT_NEW(ImageRenderTarget, ctx);
                m_target->renderTargetType(RenderTargetType::WindowTarget);
                m_target->bindTarget(m_fctImage);
            }

            if (m_depthStencilImage)
            {
                m_depthStencilImage->resize(m_width, m_height);
            }
        }

        void Swapchain::createDepthBuffer(FCT::Context* ctx, Format format, uint32_t imageCount)
        {
            m_depthStencilImage = ctx->createResource<MutilBufferImage>();
            m_depthStencilImage->width(m_width);
            m_depthStencilImage->height(m_height);
            m_depthStencilImage->format(format);
            m_depthStencilImage->samples(Samples::sample_1);
            m_depthStencilImage->as(ImageUsage::DepthStencil);
            m_depthStencilImage->imageCount(imageCount);
            m_depthStencilImage->create();
            if (m_target)
            {
                m_target->setDepthStencilBuffer(m_depthStencilImage);
            }
        }

        void Swapchain::updateCurrentImageIndex(uint32_t index)
        {
            m_currentImageIndex = index;
            if (m_fctImage)
            {
                m_fctImage->changeCurrentIndex(m_currentImageIndex);
            }
            if (m_depthStencilImage)
            {
                m_depthStencilImage->changeCurrentIndex(m_currentImageIndex);
            }
        }

        void Swapchain::waitAndResetRenderFinishFences()
        {
            for (auto* fence : m_renderFinshFences)
            {
                if (fence)
                {
                    fence->waitFor();
                    fence->reset();
                }
            }
        }

        void Swapchain::recreateRenderFinishSemaphores()
        {
            for (auto* semaphore : m_renderFinshSemaphores)
            {
                if (semaphore)
                {
                    semaphore->create();
                }
            }
        }
    }
}

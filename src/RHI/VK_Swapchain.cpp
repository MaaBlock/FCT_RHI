#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
#include "./VK_Swapchain.h"

#include "./VK_Fence.h"
#include "./Fence.h"
#include "./Semaphore.h"
#include "./Image.h"
#include "./Pass.h"
#include "../Context/MutilBufferImage.h"

namespace FCT {
    namespace RHI
    {
        VK_Swapchain::VK_Swapchain(VK_Context *ctx) {
            m_ctx = ctx;
            m_swapchain = nullptr;
            m_fctImage = nullptr;
            m_target = nullptr;
            m_prensentFinshSemphore = nullptr;
            m_needRecreated = false;
            m_depthStencilImage = nullptr;
            m_recreatedFlag = false;
        }

        VK_Swapchain::~VK_Swapchain() {

        }
        void VK_Swapchain::cleanupOldResources() {
            if (m_fctImages.size()) {
                for (auto image : m_fctImages) {
                    image->release();
                }
                m_fctImages.clear();
            }
        }

        void VK_Swapchain::setupQueueFamilies() {
            auto phyDc = m_ctx->getPhysicalDevice();
            auto dc = m_ctx->getDevice();
            auto graphicsQueueFamily = m_ctx->getGraphicsQueueFamily();

            m_presentQueueFamilyIndex = UINT32_MAX;

            auto queueFamilyCount = phyDc.getQueueFamilyProperties().size();
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (phyDc.getSurfaceSupportKHR(i, m_surface)) {
                    m_presentQueueFamilyIndex = i;
                    break;
                }
            }

            if (m_presentQueueFamilyIndex == UINT32_MAX) {
                m_presentQueueFamilyIndex = graphicsQueueFamily;
            }

            m_presentQueues.clear();
            m_presentQueues.push_back(dc.getQueue(m_presentQueueFamilyIndex, 0));
            m_presentQueue = dc.getQueue(m_presentQueueFamilyIndex, 0);
        }
        vk::SwapchainCreateInfoKHR VK_Swapchain::buildSwapchainCreateInfo(
            const vk::SurfaceFormatKHR& format,
            vk::PresentModeKHR presentMode,
            const vk::Extent2D& extent,
            uint32_t imageCount,
            const vk::SurfaceCapabilitiesKHR& capabilities,
            uint32_t graphicsQueueFamily) {

            vk::SwapchainCreateInfoKHR createInfo{};
            createInfo.setSurface(m_surface)
                      .setMinImageCount(imageCount)
                      .setImageFormat(format.format)
                      .setImageColorSpace(format.colorSpace)
                      .setImageExtent(extent)
                      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                      .setPreTransform(capabilities.currentTransform);
            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;
            if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
                usage |= vk::ImageUsageFlagBits::eTransferDst;
            }
            createInfo.setPresentMode(presentMode)
                      .setImageUsage(usage)
                      .setImageArrayLayers(1)
                      .setClipped(true);

            uint32_t queueFamilyIndices[] = {graphicsQueueFamily, m_presentQueueFamilyIndex};
            if (graphicsQueueFamily != m_presentQueueFamilyIndex) {
                createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                          .setQueueFamilyIndexCount(2)
                          .setPQueueFamilyIndices(queueFamilyIndices);
            } else {
                createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
            }

            return createInfo;
        }
        void VK_Swapchain::createSwapchain(uint32_t width, uint32_t height) {
            auto phyDc = m_ctx->getPhysicalDevice();
            auto dc = m_ctx->getDevice();
            auto graphicsQueueFamily = m_ctx->getGraphicsQueueFamily();

            auto capabilities = phyDc.getSurfaceCapabilitiesKHR(m_surface);
            auto formats = phyDc.getSurfaceFormatsKHR(m_surface);
            auto presentModes = phyDc.getSurfacePresentModesKHR(m_surface);

            auto format = chooseSwapSurfaceFormat(formats);
            auto presentMode = chooseSwapPresentMode(presentModes);
            m_imageFormat = format.format;

            vk::Extent2D extent(
                   std::clamp(m_dstWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                   std::clamp(m_dstHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
               );

            m_width = extent.width;
            m_height = extent.height;


            auto imageCount = std::clamp(3u, capabilities.minImageCount, capabilities.maxImageCount);
            vk::SwapchainCreateInfoKHR createInfo = buildSwapchainCreateInfo(
                format, presentMode, extent, imageCount, capabilities, graphicsQueueFamily
            );

            vk::SwapchainKHR oldSwapchain = m_swapchain;
            if (oldSwapchain) {
                createInfo.setOldSwapchain(oldSwapchain);
            }

            m_swapchain = dc.createSwapchainKHR(createInfo);
            m_images = dc.getSwapchainImagesKHR(m_swapchain);

            if (oldSwapchain) {
                dc.destroySwapchainKHR(oldSwapchain);
            }
        }
        void VK_Swapchain::createImagesAndTargets() {
            for (auto image : m_images) {
                auto ret = FCT_NEW(VK_Image,m_ctx);
                ret->samples(getSamples());
                ret->format(getFormat());
                ret->width(m_width);
                ret->height(m_height);
                ret->usage(ImageUsage::RenderTarget);
                ret->create(image);

                m_fctImages.push_back(ret);
            }
            bindSwapchainImages(m_ctx);
        }

        void VK_Swapchain::create(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
            ZoneScoped();
            m_dstWidth = width;
            m_dstHeight = height;
            cleanupOldResources();
            m_surface = surface;
            setupQueueFamilies();
            createSwapchain(m_dstWidth, m_dstHeight);
            createImagesAndTargets();
        }

        void VK_Swapchain::create()
        {
            create(*static_cast<vk::SurfaceKHR*>(m_nativeHandle),m_pendingWidth,m_pendingHeight);
        }

        vk::SurfaceFormatKHR VK_Swapchain::chooseSwapSurfaceFormat(
                const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
            for (const auto &availableFormat : availableFormats) {
                if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
                    availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                    return availableFormat;
                    }
            }
            return availableFormats[0];
        }

        vk::PresentModeKHR VK_Swapchain::chooseSwapPresentMode(
                const std::vector<vk::PresentModeKHR> &availablePresentModes) {
            for (const auto &availablePresentMode : availablePresentModes) {
                if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                    return availablePresentMode;
                }
            }
            return vk::PresentModeKHR::eFifo;
        }

        void VK_Swapchain::destroy() {

        }


        bool VK_Swapchain::doRecreate()
        {
            ZoneScoped();
            auto dc = m_ctx->getDevice();
            dc.waitIdle();
            recreateRenderFinishSemaphores();
            create();
            return acquireFirstImage();
        }

        void VK_Swapchain::present()
        {
            ZoneScoped();
            auto dc = m_ctx->getDevice();

            std::vector<vk::Semaphore> semaphores;
            for (auto semaphore : m_renderFinshSemaphores)
            {
                semaphores.push_back(static_cast<VK_Semaphore*>(semaphore)->semaphore());
            }
            vk::PresentInfoKHR presentInfo{};
            presentInfo.setWaitSemaphores(semaphores)
                    .setSwapchainCount(1)
                    .setPSwapchains(&m_swapchain)
                    .setPImageIndices(&m_currentImageIndex);

            try {
                ZoneScopedN("vkQueuePresentKHR");
                auto result = m_presentQueues[m_ctx->currentSubmitFrameIndex() % m_presentQueues.size()].presentKHR(presentInfo);
                if (result == vk::Result::eSuboptimalKHR) {
                    m_recreatedFlag = true;
                    waitAndResetRenderFinishFences();
                    return;
                }
            } catch (vk::OutOfDateKHRError& e) {
                m_recreatedFlag = true;
                waitAndResetRenderFinishFences();
                return;
            } catch (vk::SystemError& e)
            {
                ferr << "fetal error in VK_Swapchain::present()" << e.what() << std::endl;
                return;
            }
            {
                ZoneScopedN("WaitRenderFinishFences");
                waitAndResetRenderFinishFences();
            }
            try {
                ZoneScopedN("vkAcquireNextImageKHR");
                auto nextResult = dc.acquireNextImageKHR(
                        m_swapchain,
                        UINT64_MAX,
                        m_prensentFinshSemphore ? m_prensentFinshSemphore->semaphore() : nullptr,
                        nullptr,
                        &m_currentImageIndex
                );

                if (nextResult == vk::Result::eSuboptimalKHR) {
                    m_recreatedFlag = true;
                    return;
                }
            } catch (vk::OutOfDateKHRError& e) {
                m_recreatedFlag = true;
                return;
            } catch (vk::SystemError& e) {
                ferr << "fetal error in VK_Swapchain::present() - acquireNextImage" << e.what() << std::endl;
                return;
            }

            updateCurrentImageIndex(m_currentImageIndex);
        }

        bool VK_Swapchain::acquireFirstImage() {
            auto dc = m_ctx->getDevice();
            VK_Fence* fence = nullptr;
            try
            {
                if (m_prensentFinshSemphore)
                {
                    m_prensentFinshSemphore->create();
                    auto nextResult = dc.acquireNextImageKHR(
                            m_swapchain,
                            UINT64_MAX,
                            m_prensentFinshSemphore->semaphore(),
                            nullptr,
                            &m_currentImageIndex
                    );
                    if (nextResult == vk::Result::eSuboptimalKHR) {
                        return false;
                    }
                } else
                {
                    fence = FCT_NEW(VK_Fence,m_ctx);
                    fence->create();
                    auto nextResult = dc.acquireNextImageKHR(
                            m_swapchain,
                            UINT64_MAX,
                            nullptr,
                            fence->fence(),
                            &m_currentImageIndex
                    );
                    fence->waitFor();
                    fence->release();
                    if (nextResult == vk::Result::eSuboptimalKHR) {
                        return false;
                    }
                }
            } catch (vk::OutOfDateKHRError& e)
            {
                return false;
            }
            updateCurrentImageIndex(m_currentImageIndex);
            return true;
        }

        bool VK_Swapchain::processRecreate(bool waitFence)
        {
            if (m_needRecreated)
            {
                if (waitFence)
                {
                    waitAndResetRenderFinishFences();
                }
                create();
                m_needRecreated = false;
                m_recreated = true;
                return true;
            }
            return false;
        }

        vk::Extent2D VK_Swapchain::getExtent() const {
            return m_extent;
        }

        Format VK_Swapchain::getFormat() const
        {
            return FromVkFormat(m_imageFormat);
        }

        void VK_Swapchain::setPresentFinshSemaphore(RHI::Semaphore* semaphore)
        {
            m_prensentFinshSemphore = static_cast<VK_Semaphore*>(semaphore);
        }

        RHI::Semaphore* VK_Swapchain::getImageAvailableSemaphore()
        {
            return m_prensentFinshSemphore;
        }

        void VK_Swapchain::enableDepthBuffer(Format format)
        {
            createDepthBuffer(m_ctx, format, static_cast<uint32_t>(m_images.size()));
        }
    }
}


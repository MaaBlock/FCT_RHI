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
            if (m_fctImage)
            {
                m_fctImage->create(m_fctImages);
            } else
            {
                m_fctImage = FCT_NEW(MutilBufferImage,m_ctx);
                m_fctImage->renderTargetType(RenderTargetType::WindowTarget);
                m_fctImage->as(ImageUsage::RenderTarget);
                m_fctImage->create(m_fctImages);
            }
            if (m_target)
            {

            } else
            {
                m_target = FCT_NEW(ImageRenderTarget,m_ctx);
                m_target->renderTargetType(RenderTargetType::WindowTarget);
                m_target->bindTarget(m_fctImage);
            }

        }

        void VK_Swapchain::create(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
            m_dstWidth = width;
            m_dstHeight = height;
            //dst size != real size
            cleanupOldResources();
            m_surface = surface;
            setupQueueFamilies();
            createSwapchain(m_dstWidth, m_dstHeight);
            createImagesAndTargets();

            if (m_depthStencilImage) {
                m_depthStencilImage->resize(m_width, m_height);
            }
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


        uint32_t VK_Swapchain::getCurrentImageIndex() const {
            return m_currentImageIndex;
        }

        bool VK_Swapchain::doRecreate()
        {
            //reset all render finsh semaphores
            auto dc = m_ctx->getDevice();
            dc.waitIdle();
            for (auto semaphore : m_renderFinshSemaphores)
            {
                semaphore->create();
            }
            create();
            return acquireFirstImage();
        }

        void VK_Swapchain::sync()
        {
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
                    if (timeSinceRequest > RECREATE_DEBOUNCE_MS && m_pendingWidth == m_dstWidth && m_pendingHeight == m_dstHeight)
                        m_recreatedFlag = false;
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

        void VK_Swapchain::present()
        {
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
                auto result = m_presentQueues[m_ctx->currentSubmitFrameIndex() % m_presentQueues.size()].presentKHR(presentInfo);
                if (result == vk::Result::eSuboptimalKHR) {
                    m_recreatedFlag = true;
                    for (auto fence : m_renderFinshFences)
                    {
                        fence->waitFor();
                        fence->reset();
                    }
                    return;
                }
            } catch (vk::OutOfDateKHRError& e) {
                m_recreatedFlag = true;
                for (auto fence : m_renderFinshFences)
                {
                    fence->waitFor();
                    fence->reset();
                }
                return;
            } catch (vk::SystemError& e)
            {
                ferr << "fetal error in VK_Swapchain::present()" << e.what() << std::endl;
                return;
            }
            for (auto fence : m_renderFinshFences)
            {
                fence->waitFor();
                fence->reset();
                //fout << "nomarl reset fence" << std::endl;
            }
            try {
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

            m_fctImage->changeCurrentIndex(m_currentImageIndex);
            if (m_depthStencilImage)
            {
                m_depthStencilImage->changeCurrentIndex(m_currentImageIndex);
            }
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
            m_fctImage->changeCurrentIndex(m_currentImageIndex);
            if (m_depthStencilImage)
            {
                m_depthStencilImage->changeCurrentIndex(m_currentImageIndex);
            }
            return true;
        }

        void VK_Swapchain::needRecreate(int width, int height)
        {
            m_lastRecreateRequestTime = std::chrono::high_resolution_clock::now();
            m_recreatedFlag = true;
            m_pendingWidth = width;
            m_pendingHeight = height;
            //m_needRecreated = true;
            //m_recreated = false;
            //FCT_WAIT_FOR(m_recreated);
        }

        bool VK_Swapchain::processRecreate(bool waitFence)
        {
            auto dc = m_ctx->getDevice();
            if (m_needRecreated)
            {
                if (waitFence)
                {
                    for (auto fence : m_renderFinshFences)
                    {
                        fence->waitFor();
                        fence->reset();
                    }
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

        Samples VK_Swapchain::getSamples() const
        {
            return Samples::sample_1;
        }

        ImageRenderTarget* VK_Swapchain::target() const
        {
            return m_target;
        }

        void VK_Swapchain::setPresentFinshSemaphore(RHI::Semaphore* semaphore)
        {
            m_prensentFinshSemphore = static_cast<VK_Semaphore*>(semaphore);
        }

        RHI::Semaphore* VK_Swapchain::getImageAvailableSemaphore()
        {
            return m_prensentFinshSemphore;
        }

        Samples VK_Swapchain::getSampleCount() const
        {
            return Samples::sample_1;
        }


        void VK_Swapchain::enableDepthBuffer(Format format)
        {
            m_depthStencilImage = m_ctx->createResource<MutilBufferImage>();
            m_depthStencilImage->width(m_width);
            m_depthStencilImage->height(m_height);
            m_depthStencilImage->format(format);
            m_depthStencilImage->samples(Samples::sample_1);
            m_depthStencilImage->as(ImageUsage::DepthStencil);
            m_depthStencilImage->imageCount(m_images.size());
            m_depthStencilImage->create();
            m_target->setDepthStencilBuffer(m_depthStencilImage);
        }
    }
}


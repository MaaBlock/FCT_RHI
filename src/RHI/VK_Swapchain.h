//
// Created by Administrator on 2025/3/4.
//
#include "../ThirdParty.h"
#include "../ToolDefine.h"
#include "../Context//VK_Context.h"
#include "./Swapcain.h"
#include "./VK_Image.h"
#include "VK_Semaphore.h"
#include "../Context/ImageRenderTarget.h"
#include "../Context/MutilBufferImage.h"
#include "../Base/IEventSystem.h"
#ifndef FCT_VK_SWAPCHAIN_H
#define FCT_VK_SWAPCHAIN_H
namespace FCT{
    class VK_Context;
    namespace RHI
    {
        class VK_Swapchain : public Swapchain {
        public:
            VK_Swapchain(VK_Context* ctx);
            ~VK_Swapchain();
            void create(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
            void create() override;
            void destroy();
            void present() override;
            bool acquireFirstImage() override;
            bool processRecreate(bool waitFence = false);
            void enableDepthBuffer(Format format) override;
            vk::Fence getInFlightFence() const;
            vk::Extent2D getExtent() const;
            Format getFormat() const override;
            void setPresentFinshSemaphore(RHI::Semaphore* semaphore) override;
            RHI::Semaphore* getImageAvailableSemaphore() override;
            bool doRecreate() override;
        private:
            void cleanupOldResources();
            void setupQueueFamilies();
            vk::SwapchainCreateInfoKHR buildSwapchainCreateInfo(const vk::SurfaceFormatKHR& format,
                                                                vk::PresentModeKHR presentMode,
                                                                const vk::Extent2D& extent, uint32_t imageCount,
                                                                const vk::SurfaceCapabilitiesKHR& capabilities,
                                                                uint32_t graphicsQueueFamily);
            void createSwapchain(uint32_t width, uint32_t height);
            void createImagesAndTargets();
            bool m_recreated = false;
            bool m_needRecreated = false;
            vk::Format m_imageFormat;
            std::vector<vk::Queue> m_presentQueues;
            vk::Queue m_presentQueue;
            uint32_t m_presentQueueFamilyIndex = 0;
            VK_Context* m_ctx;
            vk::SurfaceKHR m_surface;
            vk::SwapchainKHR m_swapchain;
            vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
            vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
            std::vector<vk::Image> m_images;
            size_t m_currentFrame = 0;
            vk::Extent2D m_extent;
            RHI::VK_Semaphore* m_prensentFinshSemphore;
        };

    }
}

#endif //FCT_VK_SWAPCHAIN_H
//
// Created by Administrator on 2025/3/31.
//

#include "../FCTAPI.h"
namespace FCT
{
    namespace RHI
    {
        VK_CommandBuffer::VK_CommandBuffer(VK_CommandPool* pool) : m_allocateInfo{}
        {
            m_pool = pool;
        }

        VK_CommandBuffer::~VK_CommandBuffer()
        {
            if (m_commandBuffer)
            {
                m_pool->context()->device().freeCommandBuffers(m_pool->pool(), 1, &m_commandBuffer);
            }
        }

        void VK_CommandBuffer::create()
        {
            switch (m_level)
            {
            case CommandBufferLevel::Primary:
                m_allocateInfo.level = vk::CommandBufferLevel::ePrimary;
                break;
            case CommandBufferLevel::Secondary:
                m_allocateInfo.level = vk::CommandBufferLevel::eSecondary;
                break;
            }
            m_allocateInfo.commandPool = m_pool->pool();
            m_allocateInfo.commandBufferCount = 1;
            m_commandBuffer = m_pool->context()->device().allocateCommandBuffers(m_allocateInfo)[0];

        }

        void VK_CommandBuffer::nextPass()
        {
            m_commandBuffer.nextSubpass(vk::SubpassContents::eInline);
        }

        void VK_CommandBuffer::scissor(Vec2 lt, Vec2 rb)
        {
            vk::Rect2D scissor{};
            scissor.offset.x = static_cast<int32_t>(lt.x);
            scissor.offset.y = static_cast<int32_t>(lt.y);
            scissor.extent.width = static_cast<uint32_t>(rb.x - lt.x);
            scissor.extent.height = static_cast<uint32_t>(rb.y - lt.y);
            m_commandBuffer.setScissor(0,1, &scissor);
        }

        void VK_CommandBuffer::bindPipieline(RasterizationPipeline* pipeline)
        {
            switch (pipeline->getType())
            {
            case PipelineType::Traditional:
                {
                    auto vkPipeline = static_cast<VK_TraditionalPipeline*>(pipeline);
                    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,vkPipeline->pipeline());
                }
                break;
            }
        }


        void VK_CommandBuffer::submit()
        {
            ZoneScoped();
            vk::SubmitInfo submitInfo{};
            submitInfo.setCommandBuffers(m_commandBuffer);
            std::vector<vk::Semaphore> waitSemaphores;
            std::vector<vk::PipelineStageFlags> waitStages;
            for (auto& desc : m_waitSemaphores)
            {
                waitSemaphores.push_back(static_cast<VK_Semaphore*>(desc.semaphore)->semaphore());
                waitStages.push_back(ToVkPipelineStageFlags(desc.stages));
            }
            submitInfo.setWaitSemaphores(waitSemaphores);
            submitInfo.setWaitDstStageMask(waitStages);
            std::vector<vk::Semaphore> signalSemaphores;
            for (auto& signalSemaphore : m_signalSemaphores)
            {
                signalSemaphores.push_back(static_cast<VK_Semaphore*>(signalSemaphore)->semaphore());
            }
            submitInfo.setSignalSemaphores(signalSemaphores);

            m_pool->context()->getGraphicsQueue().submit(submitInfo,
                static_cast<VK_Fence*>(m_fence)->fence());
        }

        void VK_CommandBuffer::barrier(FCT::Image* image, ImageLayout oldLayout, ImageLayout newLayout,
            PipelineStages srcStage, PipelineStages dstStage, AccessFlags srcAccess, AccessFlags dstAccess,
            ImageAspects aspectMask)
        {
            RHI::Image* fctImage = image->currentImage();
            auto vkImage = static_cast<VK_Image*>(fctImage);
            auto imageVk = vkImage->image();
            auto newLayoutVk = ToVkImageLayout(newLayout);
            auto oldLayoutVk = ToVkImageLayout(oldLayout);
            vk::ImageMemoryBarrier barrier{};
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.image = imageVk;
            barrier.oldLayout = oldLayoutVk;
            barrier.newLayout = newLayoutVk;
            barrier.srcAccessMask = ToVkAccessFlags(srcAccess);
            barrier.dstAccessMask = ToVkAccessFlags(dstAccess);
            barrier.subresourceRange.aspectMask = ToVkImageAspects(aspectMask);
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            vk::PipelineStageFlags srcStageMask = ToVkPipelineStageFlags(srcStage);
            vk::PipelineStageFlags dstStageMask = ToVkPipelineStageFlags(dstStage);
            std::vector<vk::ImageMemoryBarrier> barriers{barrier};
            m_commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, vk::DependencyFlags(),
                0, nullptr, 0, nullptr, 1, barriers.data());
        }
    }
}

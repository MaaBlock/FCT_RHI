//
// Created by Administrator on 2025/3/31.
//
#include "./CommandBuffer.h"
#include "VK_CommandPool.h"
#include "AccessFlags.h"
#include "PipelineStage.h"
#include "ImageAspect.h"
#ifndef VK_COMMANDBUFFER_H
#define VK_COMMANDBUFFER_H

namespace FCT
{
    namespace RHI
    {
        class RasterizationPipeline;
        class VK_CommandBuffer : public CommandBuffer{
        public:
            VK_CommandBuffer(VK_CommandPool* pool);
            ~VK_CommandBuffer() override;
            void create() override;
            vk::CommandBuffer commandBuffer()
            {
                return m_commandBuffer;
            }
            void reset() override
            {
                m_commandBuffer.reset();
            }
            void begin() override
            {
                vk::CommandBufferBeginInfo beginInfo{};
                m_commandBuffer.begin(beginInfo);
            }
            void nextPass() override;
            void viewport(Vec2 lt, Vec2 rb) override
            {
                vk::Viewport viewport{};
                viewport.x = static_cast<float>(lt.x);
                viewport.y = static_cast<float>(rb.y);
                viewport.width = static_cast<float>(rb.x - lt.x);
                viewport.height = -static_cast<float>(rb.y - lt.y);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                m_commandBuffer.setViewport(0,1, &viewport);
            }
            void scissor(Vec2 lt, Vec2 rb) override;
            void bindPipieline(RasterizationPipeline* pipeline) override;
            void draw(size_t vertexBegin, size_t instanceBegin, size_t vertexSize, size_t instanceSize) override
            {
                m_commandBuffer.draw(static_cast<uint32_t>(vertexSize),static_cast<uint32_t>(instanceSize) ,static_cast<uint32_t>(vertexBegin), static_cast<uint32_t>(instanceBegin));
            }
            void end() override
            {
                m_commandBuffer.end();
            }
            void submit() override;
            vk::CommandBuffer commandBuffer() const
            {
                return m_commandBuffer;
            }
            void drawIndex(size_t indexBegin, size_t instanceBegin, size_t indexCount, size_t instanceCount, size_t firstVertex) override
            {
                m_commandBuffer.drawIndexed(static_cast<uint32_t>(indexCount),static_cast<uint32_t>(instanceCount), static_cast<uint32_t>(indexBegin), static_cast<int32_t>(firstVertex), static_cast<uint32_t>(instanceBegin));
            }
            void drawIndexedIndirect(void* indirectBuffer, size_t byteOffset, uint32_t drawCount, uint32_t stride) override
            {
                if (!indirectBuffer || drawCount == 0) {
                    return;
                }
                vk::Buffer vkBuf(reinterpret_cast<VkBuffer>(indirectBuffer));
                m_commandBuffer.drawIndexedIndirect(vkBuf, static_cast<vk::DeviceSize>(byteOffset), drawCount, stride);
            }
            void barrier(FCT::Image* image, ImageLayout oldLayout, ImageLayout newLayout, PipelineStages srcStage, PipelineStages dstStage, AccessFlags srcAccess, AccessFlags dstAccess, ImageAspects aspectMask) override;
        protected:
            VK_CommandPool* m_pool;
            vk::CommandBufferAllocateInfo m_allocateInfo;
            //vk::CommandBuffer> m_commandBuffers;
            vk::CommandBuffer m_commandBuffer;
        };
    }
}



#endif //VK_COMMANDBUFFER_H

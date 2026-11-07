//
// Created by Administrator on 2025/3/31.
//
#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include "../MutilThreadBase/RefCount.h"
#include "../Base/Flags.h"
#include "../Context/DataTypes.h"
#include "./PipelineStage.h"
#include "./ImageAspect.h"
#include "./ImageLayout.h"
#include "./AccessFlags.h"

namespace FCT
{
    class Image;
    namespace RHI
    {
        class RasterizationPipeline;
        class Semaphore;
        class Fence;
        enum class CommandBufferLevel
        {
            Primary,
            Secondary
        };
        struct WaitSemaphoreDescription
        {
            Semaphore* semaphore;
            PipelineStages stages;
        };
        struct DrawIndexedIndirectCommand
        {
            uint32_t indexCount = 0;
            uint32_t instanceCount = 0;
            uint32_t firstIndex = 0;
            int32_t vertexOffset = 0;
            uint32_t firstInstance = 0;
        };
        class CommandBuffer : public RefCount
        {
        public:
            CommandBuffer()
            {
                level(CommandBufferLevel::Primary);
                m_fence = nullptr;
            }
            ~CommandBuffer();

            void level(CommandBufferLevel level)
            {
                m_level = level;
            }
            virtual void reset() = 0;
            virtual void create() = 0;
            virtual void begin() = 0;
            virtual void viewport(Vec2 lt,Vec2 rb) = 0;
            virtual void scissor(Vec2 lt,Vec2 rb) = 0;
            virtual void draw(size_t vertexBegin,size_t instanceBegin,size_t vertexSize,size_t instanceSize) = 0;
            virtual void drawIndex(size_t indexBegin, size_t instanceBegin, size_t indexCount, size_t instanceCount, size_t firstVertex = 0) = 0;
            virtual void drawIndexedIndirect(void* indirectBuffer, size_t byteOffset, uint32_t drawCount = 1, uint32_t stride = sizeof(DrawIndexedIndirectCommand))
            {
                (void)indirectBuffer;
                (void)byteOffset;
                (void)drawCount;
                (void)stride;
            }
            virtual void bindPipieline(RasterizationPipeline* pipeline) = 0;
            virtual void end() = 0;
            virtual void submit() = 0;
            virtual void nextPass() = 0;
            virtual void barrier(FCT::Image* image, ImageLayout oldLayout, ImageLayout newLayout,
                        PipelineStages srcStage, PipelineStages dstStage,
                        AccessFlags srcAccess, AccessFlags dstAccess,
                        ImageAspects aspectMask = ImageAspect::color) = 0;
            Fence* fence() const { return m_fence; }
            void fence(Fence* fence);
            std::vector<WaitSemaphoreDescription>& waitSemaphores() { return m_waitSemaphores; }
            std::vector<Semaphore*>& signalSemaphores() { return m_signalSemaphores; }
            void clearWaitSemaphores();
            void addWaitSemaphore(Semaphore* semaphore,PipelineStages stage = PipelineStage::colorAttachmentOutput);
            void addSignalSemaphore(Semaphore* semaphore);
        protected:
            CommandBufferLevel m_level;
            Fence* m_fence;
            std::vector<WaitSemaphoreDescription> m_waitSemaphores;
            std::vector<Semaphore*> m_signalSemaphores;
            //std::vector<Fence*> m_submitFence;
        };

    }
}
#endif //COMMANDBUFFER_H

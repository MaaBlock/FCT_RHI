#ifndef FCT_D3D12_COMMANDBUFFER_H
#define FCT_D3D12_COMMANDBUFFER_H

#include "./CommandBuffer.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_CommandPool;
        class D3D12_PassGroup;
        class D3D12_TraditionalPipeline;

        class D3D12_CommandBuffer : public CommandBuffer
        {
        public:
            explicit D3D12_CommandBuffer(D3D12_CommandPool* pool);
            ~D3D12_CommandBuffer() override;

            void create() override;
            void reset() override;
            void begin() override;
            void viewport(Vec2 lt, Vec2 rb) override;
            void scissor(Vec2 lt, Vec2 rb) override;
            void draw(size_t vertexStart, size_t instanceStart, size_t vertexSize, size_t instanceCount) override;
            void drawIndex(size_t indexStart, size_t instanceStart, size_t indexCount, size_t instanceCount, size_t firstVertex = 0) override;
            void bindPipieline(RasterizationPipeline* pipeline) override;
            void end() override;
            void submit() override;
            void nextPass() override;
            void barrier(
                FCT::Image* image,
                ImageLayout oldLayout,
                ImageLayout newLayout,
                PipelineStages srcStage,
                PipelineStages dstStage,
                AccessFlags srcAccess,
                AccessFlags dstAccess,
                ImageAspects aspectMask = ImageAspect::color) override;

            ID3D12GraphicsCommandList* commandList() const { return m_commandList.Get(); }
            D3D12_Context* context() const { return m_ctx; }

            void setCurrentPassGroup(D3D12_PassGroup* passGroup) { m_currentPassGroup = passGroup; }
            D3D12_PassGroup* currentPassGroup() const { return m_currentPassGroup; }

            void setBoundPipeline(D3D12_TraditionalPipeline* pipeline) { m_boundPipeline = pipeline; }
            D3D12_TraditionalPipeline* boundPipeline() const { return m_boundPipeline; }

            void bindDescriptorHeaps();

        private:
            D3D12_CommandPool* m_pool;
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_allocator;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
            bool m_isOpen;
            D3D12_PassGroup* m_currentPassGroup;
            D3D12_TraditionalPipeline* m_boundPipeline;
        };
    }
}

#endif // FCT_D3D12_COMMANDBUFFER_H

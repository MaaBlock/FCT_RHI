#ifndef FCT_D3D12_VERTEXBUFFER_H
#define FCT_D3D12_VERTEXBUFFER_H

#include "./VertexBuffer.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_VertexBuffer : public VertexBuffer
        {
        public:
            explicit D3D12_VertexBuffer(D3D12_Context* ctx);
            ~D3D12_VertexBuffer();

            void createSingleBuffer(size_t index);
            void allocateBufferIfNeeded(size_t index);
            void create() override;
            void mapBuffer(void* data, uint32_t size) override;
            void resize(uint32_t newSize) override;
            void checkAndResizeIfNeeded(uint32_t frameIndex);
            void bind(CommandBuffer* srcCmd, uint32_t slot = 0, uint32_t offset = 0) override;

        private:
            void destroyBuffers();

        private:
            D3D12_Context* m_ctx;
            std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffers;
            std::vector<size_t> m_allocatedSizes;
            std::vector<uint8_t> m_dirtyFlags;
        };
    }
}

#endif // FCT_D3D12_VERTEXBUFFER_H

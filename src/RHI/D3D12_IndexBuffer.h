#ifndef FCT_D3D12_INDEXBUFFER_H
#define FCT_D3D12_INDEXBUFFER_H

#include "./IndexBuffer.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_IndexBuffer : public IndexBuffer
        {
        public:
            explicit D3D12_IndexBuffer(D3D12_Context* ctx);
            ~D3D12_IndexBuffer() override;

            void createSingleBuffer(size_t index);
            void allocateBufferIfNeeded(size_t index);
            void create() override;
            void mapBuffer(void* data, uint32_t size) override;
            void resize(size_t size) override;
            void checkAndResizeIfNeeded(uint32_t frameIndex);
            void bind(CommandBuffer* srcCmd) override;

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

#endif // FCT_D3D12_INDEXBUFFER_H

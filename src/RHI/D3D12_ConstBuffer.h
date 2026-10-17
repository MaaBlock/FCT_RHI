#ifndef FCT_D3D12_CONSTBUFFER_H
#define FCT_D3D12_CONSTBUFFER_H

#include "./ConstBuffer.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_ConstBuffer : public ConstBuffer
        {
        public:
            explicit D3D12_ConstBuffer(D3D12_Context* ctx);
            ~D3D12_ConstBuffer() override;

            void mapData() override;
            void create() override;

            D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t frameIdx) const;
            D3D12_GPU_VIRTUAL_ADDRESS getGpuVirtualAddress(uint32_t frameIdx) const;

        private:
            void destroyResources();

        private:
            D3D12_Context* m_ctx;
            std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffers;
            std::vector<void*> m_mappedData;
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpuHandles;
            std::vector<uint32_t> m_descriptorIndices;
            size_t m_alignedSize;
        };
    }
}

#endif // FCT_D3D12_CONSTBUFFER_H

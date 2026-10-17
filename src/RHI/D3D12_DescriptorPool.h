#ifndef FCT_D3D12_DESCRIPTORPOOL_H
#define FCT_D3D12_DESCRIPTORPOOL_H

#include "./DescriptorPool.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_DescriptorPool : public DescriptorPool
        {
        public:
            explicit D3D12_DescriptorPool(D3D12_Context* ctx);
            ~D3D12_DescriptorPool();

            void create() override;
            void destroy();

            ID3D12DescriptorHeap* getCbvSrvUavHeap() const { return m_cbvSrvUavHeap.Get(); }
            ID3D12DescriptorHeap* getSamplerHeap() const { return m_samplerHeap.Get(); }

            uint32_t getCbvSrvUavDescriptorSize() const { return m_cbvSrvUavDescriptorSize; }
            uint32_t getSamplerDescriptorSize() const { return m_samplerDescriptorSize; }

            bool allocCbvSrvUavRange(
                uint32_t count,
                D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle,
                D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle,
                uint32_t& outBaseIndex);

            bool allocSamplerRange(
                uint32_t count,
                D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle,
                D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle,
                uint32_t& outBaseIndex);

            D3D12_CPU_DESCRIPTOR_HANDLE getCbvSrvUavCpuHandle(uint32_t index) const;
            D3D12_GPU_DESCRIPTOR_HANDLE getCbvSrvUavGpuHandle(uint32_t index) const;
            D3D12_CPU_DESCRIPTOR_HANDLE getSamplerCpuHandle(uint32_t index) const;
            D3D12_GPU_DESCRIPTOR_HANDLE getSamplerGpuHandle(uint32_t index) const;

        private:
            void createCbvSrvUavHeap();
            void createSamplerHeap();

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerHeap;

            D3D12_CPU_DESCRIPTOR_HANDLE m_cbvSrvUavCpuStart{0};
            D3D12_GPU_DESCRIPTOR_HANDLE m_cbvSrvUavGpuStart{0};
            uint32_t m_cbvSrvUavDescriptorSize;
            uint32_t m_cbvSrvUavCapacity;
            uint32_t m_cbvSrvUavNextIndex;
            std::mutex m_cbvSrvUavMutex;

            D3D12_CPU_DESCRIPTOR_HANDLE m_samplerCpuStart{0};
            D3D12_GPU_DESCRIPTOR_HANDLE m_samplerGpuStart{0};
            uint32_t m_samplerDescriptorSize;
            uint32_t m_samplerCapacity;
            uint32_t m_samplerNextIndex;
            std::mutex m_samplerMutex;
        };
    }
}

#endif // FCT_D3D12_DESCRIPTORPOOL_H

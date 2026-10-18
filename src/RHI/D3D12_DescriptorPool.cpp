#include "../FCTAPI.h"
#include "./D3D12_DescriptorPool.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_DescriptorPool::D3D12_DescriptorPool(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_cbvSrvUavDescriptorSize(0)
            , m_cbvSrvUavCapacity(65536)
            , m_cbvSrvUavNextIndex(0)
            , m_samplerDescriptorSize(0)
            , m_samplerCapacity(2048)
            , m_samplerNextIndex(0)
        {
        }

        D3D12_DescriptorPool::~D3D12_DescriptorPool()
        {
            destroy();
        }

        void D3D12_DescriptorPool::destroy()
        {
            m_cbvSrvUavHeap.Reset();
            m_samplerHeap.Reset();
            m_cbvSrvUavNextIndex = 0;
            m_samplerNextIndex = 0;
        }

        void D3D12_DescriptorPool::createCbvSrvUavHeap()
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = m_cbvSrvUavCapacity;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NodeMask = 0;

            HRESULT hr = m_ctx->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 shader-visible CBV_SRV_UAV heap, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }
            m_cbvSrvUavCpuStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
            m_cbvSrvUavGpuStart = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
            m_cbvSrvUavDescriptorSize = m_ctx->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void D3D12_DescriptorPool::createSamplerHeap()
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            desc.NumDescriptors = m_samplerCapacity;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NodeMask = 0;

            HRESULT hr = m_ctx->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_samplerHeap));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 shader-visible SAMPLER heap, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }
            m_samplerCpuStart = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
            m_samplerGpuStart = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
            m_samplerDescriptorSize = m_ctx->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }

        void D3D12_DescriptorPool::create()
        {
            destroy();
            createCbvSrvUavHeap();
            createSamplerHeap();
        }

        bool D3D12_DescriptorPool::allocCbvSrvUavRange(
            uint32_t count,
            D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle,
            uint32_t& outBaseIndex)
        {
            if (count == 0)
            {
                outCpuHandle = {0};
                outGpuHandle = {0};
                outBaseIndex = 0;
                return true;
            }
            uint32_t baseIndex = m_cbvSrvUavNextIndex.fetch_add(count, std::memory_order_relaxed);
            if (baseIndex + count > m_cbvSrvUavCapacity)
            {
                ferr << "D3D12 shader-visible CBV_SRV_UAV descriptor heap exhausted!" << std::endl;
                return false;
            }
            outBaseIndex = baseIndex;
            outCpuHandle = getCbvSrvUavCpuHandle(outBaseIndex);
            outGpuHandle = getCbvSrvUavGpuHandle(outBaseIndex);
            return true;
        }

        bool D3D12_DescriptorPool::allocSamplerRange(
            uint32_t count,
            D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle,
            uint32_t& outBaseIndex)
        {
            if (count == 0)
            {
                outCpuHandle = {0};
                outGpuHandle = {0};
                outBaseIndex = 0;
                return true;
            }
            uint32_t baseIndex = m_samplerNextIndex.fetch_add(count, std::memory_order_relaxed);
            if (baseIndex + count > m_samplerCapacity)
            {
                ferr << "D3D12 shader-visible SAMPLER descriptor heap exhausted!" << std::endl;
                return false;
            }
            outBaseIndex = baseIndex;
            outCpuHandle = getSamplerCpuHandle(outBaseIndex);
            outGpuHandle = getSamplerGpuHandle(outBaseIndex);
            return true;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE D3D12_DescriptorPool::getCbvSrvUavCpuHandle(uint32_t index) const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbvSrvUavCpuStart;
            handle.ptr += static_cast<SIZE_T>(index) * m_cbvSrvUavDescriptorSize;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE D3D12_DescriptorPool::getCbvSrvUavGpuHandle(uint32_t index) const
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = m_cbvSrvUavGpuStart;
            handle.ptr += static_cast<UINT64>(index) * m_cbvSrvUavDescriptorSize;
            return handle;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE D3D12_DescriptorPool::getSamplerCpuHandle(uint32_t index) const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = m_samplerCpuStart;
            handle.ptr += static_cast<SIZE_T>(index) * m_samplerDescriptorSize;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE D3D12_DescriptorPool::getSamplerGpuHandle(uint32_t index) const
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = m_samplerGpuStart;
            handle.ptr += static_cast<UINT64>(index) * m_samplerDescriptorSize;
            return handle;
        }
    }
}

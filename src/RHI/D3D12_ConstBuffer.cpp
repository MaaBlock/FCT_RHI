#include "../FCTAPI.h"
#include "./D3D12_ConstBuffer.h"
#include "../Context/D3D12_Context.h"
#include "../Context/ConstBuffer.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_ConstBuffer::D3D12_ConstBuffer(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_alignedSize(256)
        {
        }

        D3D12_ConstBuffer::~D3D12_ConstBuffer()
        {
            destroyResources();
        }

        void D3D12_ConstBuffer::destroyResources()
        {
            for (size_t i = 0; i < m_buffers.size(); ++i)
            {
                if (m_buffers[i] && i < m_mappedData.size() && m_mappedData[i])
                {
                    m_buffers[i]->Unmap(0, nullptr);
                }
                m_buffers[i].Reset();
            }
            for (uint32_t idx : m_descriptorIndices)
            {
                if (idx != UINT32_MAX)
                {
                    m_ctx->freeStagingCbvSrvUavDescriptor(idx);
                }
            }
            m_buffers.clear();
            m_mappedData.clear();
            m_cpuHandles.clear();
            m_descriptorIndices.clear();
        }

        void D3D12_ConstBuffer::create()
        {
            destroyResources();

            size_t rawSize = m_layout.getTotalSize();
            m_alignedSize = (rawSize == 0) ? 256 : ((rawSize + 255) & ~size_t(255));

            size_t frameCount = (m_layout.getUpdateFrequency() == UpdateFrequency::Static)
                ? 1
                : m_ctx->maxFrameInFlight();
            if (frameCount == 0)
            {
                frameCount = 1;
            }

            m_buffers.resize(frameCount);
            m_mappedData.resize(frameCount, nullptr);
            m_cpuHandles.resize(frameCount, D3D12_CPU_DESCRIPTOR_HANDLE{0});
            m_descriptorIndices.resize(frameCount, UINT32_MAX);

            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = m_alignedSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            ID3D12Device* device = m_ctx->getDevice();
            for (size_t i = 0; i < frameCount; ++i)
            {
                HRESULT hr = device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_buffers[i]));
                if (FAILED(hr))
                {
                    ferr << "Failed to create D3D12 ConstBuffer, hr = " << std::hex << hr << std::dec << std::endl;
                    continue;
                }

                D3D12_RANGE readRange{ 0, 0 };
                m_buffers[i]->Map(0, &readRange, &m_mappedData[i]);
                if (m_mappedData[i])
                {
                    memset(m_mappedData[i], 0, m_alignedSize);
                }

                m_descriptorIndices[i] = m_ctx->allocStagingCbvSrvUavDescriptor(m_cpuHandles[i]);
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
                cbvDesc.BufferLocation = m_buffers[i]->GetGPUVirtualAddress();
                cbvDesc.SizeInBytes = static_cast<UINT>(m_alignedSize);
                device->CreateConstantBufferView(&cbvDesc, m_cpuHandles[i]);
            }

            if (m_buffer)
            {
                for (size_t i = 0; i < frameCount; ++i)
                {
                    if (m_mappedData[i])
                    {
                        memcpy(m_mappedData[i], m_buffer->getData(), m_buffer->getSize());
                    }
                }
            }
        }

        void D3D12_ConstBuffer::mapData()
        {
            if (!m_buffer || m_mappedData.empty())
            {
                return;
            }

            uint32_t frameIdx = (m_layout.getUpdateFrequency() == UpdateFrequency::Static)
                ? 0
                : m_ctx->currentSubmitFrameIndex();
            if (frameIdx >= m_mappedData.size())
            {
                frameIdx = 0;
            }

            if (m_mappedData[frameIdx])
            {
                memcpy(m_mappedData[frameIdx], m_buffer->getData(), m_buffer->getSize());
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE D3D12_ConstBuffer::getCpuHandle(uint32_t frameIdx) const
        {
            if (m_cpuHandles.empty())
            {
                return D3D12_CPU_DESCRIPTOR_HANDLE{0};
            }
            if (frameIdx >= m_cpuHandles.size())
            {
                frameIdx = 0;
            }
            return m_cpuHandles[frameIdx];
        }

        D3D12_GPU_VIRTUAL_ADDRESS D3D12_ConstBuffer::getGpuVirtualAddress(uint32_t frameIdx) const
        {
            if (m_buffers.empty())
            {
                return 0;
            }
            if (frameIdx >= m_buffers.size())
            {
                frameIdx = 0;
            }
            return m_buffers[frameIdx] ? m_buffers[frameIdx]->GetGPUVirtualAddress() : 0;
        }
    }
}

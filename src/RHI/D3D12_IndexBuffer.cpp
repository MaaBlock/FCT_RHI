#include "../FCTAPI.h"
#include "./D3D12_IndexBuffer.h"
#include "./D3D12_CommandBuffer.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_IndexBuffer::D3D12_IndexBuffer(D3D12_Context* ctx)
            : m_ctx(ctx)
        {
        }

        D3D12_IndexBuffer::~D3D12_IndexBuffer()
        {
            destroyBuffers();
        }

        void D3D12_IndexBuffer::destroyBuffers()
        {
            for (auto& buffer : m_buffers)
            {
                buffer.Reset();
            }
            m_buffers.clear();
            m_allocatedSizes.clear();
            m_dirtyFlags.clear();
        }

        void D3D12_IndexBuffer::createSingleBuffer(size_t index)
        {
            if (index >= m_buffers.size())
            {
                return;
            }
            m_buffers[index].Reset();

            size_t stride = m_indexData ? m_indexData->getStride() : m_stride;
            if (stride == 0)
            {
                stride = sizeof(uint32_t);
            }
            m_stride = stride;

            size_t bufferByteSize = m_size * stride;
            if (bufferByteSize == 0)
            {
                m_allocatedSizes[index] = 0;
                return;
            }

            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = bufferByteSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = m_ctx->getDevice()->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_buffers[index]));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 IndexBuffer, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }

            m_allocatedSizes[index] = m_size;
        }

        void D3D12_IndexBuffer::allocateBufferIfNeeded(size_t index)
        {
            if (index < m_buffers.size() && !m_buffers[index] && m_size > 0)
            {
                createSingleBuffer(index);
            }
        }

        void D3D12_IndexBuffer::create()
        {
            size_t count = (m_updateFrequency == UpdateFrequency::Static) ? 1 : m_ctx->maxFrameInFlight();
            if (count == 0)
            {
                count = 1;
            }

            m_buffers.resize(count);
            m_allocatedSizes.resize(count, 0);
            m_dirtyFlags.resize(count, 0);

            if (m_indexData)
            {
                m_stride = m_indexData->getStride();
                m_size = m_indexData->getSize();
            }

            if (m_size > 0)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    createSingleBuffer(i);
                }
                if (m_indexData)
                {
                    updataBuffer();
                }
            }
        }

        void D3D12_IndexBuffer::resize(size_t newSize)
        {
            if (newSize == m_size)
            {
                return;
            }
            m_size = newSize;
            for (uint8_t& flag : m_dirtyFlags)
            {
                flag = 1;
            }
        }

        void D3D12_IndexBuffer::checkAndResizeIfNeeded(uint32_t frameIndex)
        {
            if (frameIndex >= m_buffers.size())
            {
                return;
            }
            if (!m_buffers[frameIndex] || m_dirtyFlags[frameIndex] || m_allocatedSizes[frameIndex] < m_size)
            {
                createSingleBuffer(frameIndex);
                m_dirtyFlags[frameIndex] = 0;
            }
        }

        void D3D12_IndexBuffer::mapBuffer(void* data, uint32_t size)
        {
            if (!data || size == 0 || m_buffers.empty())
            {
                return;
            }

            uint32_t frameIndex = (m_updateFrequency == UpdateFrequency::Static)
                ? 0
                : m_ctx->currentFrameIndex();
            if (frameIndex >= m_buffers.size())
            {
                frameIndex = 0;
            }

            checkAndResizeIfNeeded(frameIndex);
            if (!m_buffers[frameIndex])
            {
                return;
            }

            size_t maxBytes = m_allocatedSizes[frameIndex] * m_stride;
            size_t copyByteSize = (size <= maxBytes) ? size : maxBytes;
            if (copyByteSize == 0)
            {
                return;
            }

            void* mappedData = nullptr;
            D3D12_RANGE readRange{ 0, 0 };
            if (SUCCEEDED(m_buffers[frameIndex]->Map(0, &readRange, &mappedData)))
            {
                memcpy(mappedData, data, copyByteSize);
                m_buffers[frameIndex]->Unmap(0, nullptr);
            }
        }

        void D3D12_IndexBuffer::bind(CommandBuffer* srcCmd)
        {
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(srcCmd);
            if (!d3dCmd || !d3dCmd->commandList() || m_buffers.empty())
            {
                return;
            }

            uint32_t frameIndex = (m_updateFrequency == UpdateFrequency::Static)
                ? 0
                : m_ctx->currentSubmitFrameIndex();
            if (frameIndex >= m_buffers.size())
            {
                frameIndex = 0;
            }

            checkAndResizeIfNeeded(frameIndex);
            ID3D12Resource* resource = m_buffers[frameIndex].Get();
            if (!resource)
            {
                return;
            }

            size_t stride = m_indexData ? m_indexData->getStride() : m_stride;
            if (stride == 0)
            {
                stride = sizeof(uint32_t);
            }

            D3D12_INDEX_BUFFER_VIEW ibv{};
            ibv.BufferLocation = resource->GetGPUVirtualAddress();
            ibv.SizeInBytes = static_cast<UINT>(m_allocatedSizes[frameIndex] * stride);
            ibv.Format = (stride == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

            d3dCmd->commandList()->IASetIndexBuffer(&ibv);
        }
    }
}

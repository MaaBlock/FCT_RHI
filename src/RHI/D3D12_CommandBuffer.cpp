#include "../FCTAPI.h"
#include "./D3D12_CommandBuffer.h"
#include "./D3D12_CommandPool.h"
#include "./D3D12_DescriptorPool.h"
#include "./D3D12_Fence.h"
#include "./D3D12_Semaphore.h"
#include "./D3D12_Image.h"
#include "./D3D12_PassGroup.h"
#include "./D3D12_TraditionalPipeline.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_CommandBuffer::D3D12_CommandBuffer(D3D12_CommandPool* pool)
            : m_pool(pool)
            , m_ctx(pool ? pool->context() : nullptr)
            , m_isOpen(false)
            , m_currentPassGroup(nullptr)
            , m_boundPipeline(nullptr)
        {
        }

        D3D12_CommandBuffer::~D3D12_CommandBuffer()
        {
            if (m_isOpen && m_commandList)
            {
                m_commandList->Close();
                m_isOpen = false;
            }
            m_commandList.Reset();
            m_allocator.Reset();
        }

        void D3D12_CommandBuffer::create()
        {
            ID3D12Device* device = m_ctx->getDevice();
            HRESULT hr = device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_allocator));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 CommandAllocator, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }

            hr = device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_allocator.Get(),
                nullptr,
                IID_PPV_ARGS(&m_commandList));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 GraphicsCommandList, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }

            m_commandList->Close();
            m_isOpen = false;
        }

        void D3D12_CommandBuffer::reset()
        {
            if (m_isOpen && m_commandList)
            {
                m_commandList->Close();
                m_isOpen = false;
            }
            if (m_allocator)
            {
                m_allocator->Reset();
            }
            m_currentPassGroup = nullptr;
            m_boundPipeline = nullptr;
        }

        void D3D12_CommandBuffer::bindDescriptorHeaps()
        {
            auto* pool = static_cast<D3D12_DescriptorPool*>(m_ctx->getDescriptorPool());
            if (!pool)
            {
                return;
            }
            ID3D12DescriptorHeap* heaps[2] = {
                pool->getCbvSrvUavHeap(),
                pool->getSamplerHeap()
            };
            UINT heapCount = 0;
            ID3D12DescriptorHeap* validHeaps[2] = { nullptr, nullptr };
            for (ID3D12DescriptorHeap* h : heaps)
            {
                if (h)
                {
                    validHeaps[heapCount++] = h;
                }
            }
            if (heapCount > 0)
            {
                m_commandList->SetDescriptorHeaps(heapCount, validHeaps);
            }
        }

        void D3D12_CommandBuffer::begin()
        {
            if (!m_isOpen && m_commandList && m_allocator)
            {
                m_commandList->Reset(m_allocator.Get(), nullptr);
                m_isOpen = true;
                bindDescriptorHeaps();
            }
        }

        void D3D12_CommandBuffer::viewport(Vec2 lt, Vec2 rb)
        {
            D3D12_VIEWPORT vp{};
            vp.TopLeftX = lt.x;
            vp.TopLeftY = lt.y;
            vp.Width = rb.x - lt.x;
            vp.Height = rb.y - lt.y;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            m_commandList->RSSetViewports(1, &vp);
        }

        void D3D12_CommandBuffer::scissor(Vec2 lt, Vec2 rb)
        {
            D3D12_RECT rect{};
            rect.left = static_cast<LONG>(lt.x);
            rect.top = static_cast<LONG>(lt.y);
            rect.right = static_cast<LONG>(rb.x);
            rect.bottom = static_cast<LONG>(rb.y);
            m_commandList->RSSetScissorRects(1, &rect);
        }

        void D3D12_CommandBuffer::draw(size_t vertexStart, size_t instanceStart, size_t vertexSize, size_t instanceCount)
        {
            m_commandList->DrawInstanced(
                static_cast<UINT>(vertexSize),
                static_cast<UINT>(instanceCount),
                static_cast<UINT>(vertexStart),
                static_cast<UINT>(instanceStart));
        }

        void D3D12_CommandBuffer::drawIndex(size_t indexStart, size_t instanceStart, size_t indexCount, size_t instanceCount, size_t firstVertex)
        {
            m_commandList->DrawIndexedInstanced(
                static_cast<UINT>(indexCount),
                static_cast<UINT>(instanceCount),
                static_cast<UINT>(indexStart),
                static_cast<INT>(firstVertex),
                static_cast<UINT>(instanceStart));
        }

        void D3D12_CommandBuffer::bindPipieline(RasterizationPipeline* pipeline)
        {
            auto* d3dPipeline = static_cast<D3D12_TraditionalPipeline*>(pipeline);
            if (d3dPipeline)
            {
                d3dPipeline->bind(this);
            }
        }

        void D3D12_CommandBuffer::end()
        {
            if (m_isOpen && m_commandList)
            {
                m_commandList->Close();
                m_isOpen = false;
            }
        }

        void D3D12_CommandBuffer::submit()
        {
            ID3D12CommandQueue* queue = m_ctx->getGraphicsQueue();
            if (!queue || !m_commandList)
            {
                return;
            }

            std::lock_guard<std::mutex> lock(m_ctx->uploadMutex());

            for (auto& waitDesc : m_waitSemaphores)
            {
                if (waitDesc.semaphore)
                {
                    static_cast<D3D12_Semaphore*>(waitDesc.semaphore)->waitOnQueue(queue);
                }
            }

            ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
            queue->ExecuteCommandLists(1, cmdLists);

            for (auto* signalSem : m_signalSemaphores)
            {
                if (signalSem)
                {
                    static_cast<D3D12_Semaphore*>(signalSem)->signalOnQueue(queue);
                }
            }

            if (m_fence)
            {
                static_cast<D3D12_Fence*>(m_fence)->signalOnQueue(queue);
            }
        }

        void D3D12_CommandBuffer::nextPass()
        {
            if (m_currentPassGroup)
            {
                m_currentPassGroup->nextPass(this);
            }
        }

        void D3D12_CommandBuffer::barrier(
            FCT::Image* image,
            ImageLayout oldLayout,
            ImageLayout newLayout,
            PipelineStages srcStage,
            PipelineStages dstStage,
            AccessFlags srcAccess,
            AccessFlags dstAccess,
            ImageAspects aspectMask)
        {
            if (!image || !image->currentImage() || !m_commandList)
            {
                return;
            }
            auto* d3dImage = static_cast<D3D12_Image*>(image->currentImage());
            d3dImage->transitionState(m_commandList.Get(), ToD3D12ResourceState(newLayout));
        }
    }
}

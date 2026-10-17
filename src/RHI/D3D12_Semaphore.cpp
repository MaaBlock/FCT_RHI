#include "../FCTAPI.h"
#include "./D3D12_Semaphore.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_Semaphore::D3D12_Semaphore(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_value(0)
        {
        }

        D3D12_Semaphore::~D3D12_Semaphore()
        {
            D3D12_Semaphore::destroy();
        }

        void D3D12_Semaphore::destroy()
        {
            m_fence.Reset();
            m_value = 0;
        }

        void D3D12_Semaphore::create()
        {
            destroy();
            HRESULT hr = m_ctx->getDevice()->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&m_fence));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 Semaphore fence, hr = " << std::hex << hr << std::dec << std::endl;
            }
            m_value = 0;
            addRef();
        }

        void D3D12_Semaphore::signalOnQueue(ID3D12CommandQueue* queue)
        {
            if (!m_fence || !queue)
            {
                return;
            }
            ++m_value;
            queue->Signal(m_fence.Get(), m_value);
        }

        void D3D12_Semaphore::waitOnQueue(ID3D12CommandQueue* queue)
        {
            if (!m_fence || !queue || m_value == 0)
            {
                return;
            }
            queue->Wait(m_fence.Get(), m_value);
        }
    }
}

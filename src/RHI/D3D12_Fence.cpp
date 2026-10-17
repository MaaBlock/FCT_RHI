#include "../FCTAPI.h"
#include "./D3D12_Fence.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_Fence::D3D12_Fence(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_event(nullptr)
            , m_lastSignaledValue(0)
            , m_targetValue(1)
        {
        }

        D3D12_Fence::~D3D12_Fence()
        {
            D3D12_Fence::destroy();
        }

        void D3D12_Fence::destroy()
        {
            if (m_event)
            {
                CloseHandle(m_event);
                m_event = nullptr;
            }
            m_fence.Reset();
        }

        void D3D12_Fence::create()
        {
            destroy();
            uint64_t initialValue = m_signaled ? 1 : 0;
            m_lastSignaledValue = initialValue;
            m_targetValue = 1;

            HRESULT hr = m_ctx->getDevice()->CreateFence(
                initialValue,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&m_fence));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 Fence, hr = " << std::hex << hr << std::dec << std::endl;
            }

            m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            addRef();
        }

        void D3D12_Fence::reset()
        {
            if (m_targetValue <= m_lastSignaledValue)
            {
                m_targetValue = m_lastSignaledValue + 1;
            }
        }

        void D3D12_Fence::signalOnQueue(ID3D12CommandQueue* queue)
        {
            if (!m_fence || !queue)
            {
                return;
            }
            if (m_targetValue <= m_lastSignaledValue)
            {
                m_targetValue = m_lastSignaledValue + 1;
            }
            m_lastSignaledValue = m_targetValue;
            queue->Signal(m_fence.Get(), m_lastSignaledValue);
        }

        void D3D12_Fence::waitFor()
        {
            if (!m_fence)
            {
                return;
            }
            if (m_fence->GetCompletedValue() < m_targetValue)
            {
                m_fence->SetEventOnCompletion(m_targetValue, m_event);
                WaitForSingleObject(m_event, INFINITE);
            }
        }
    }
}

#ifndef FCT_D3D12_FENCE_H
#define FCT_D3D12_FENCE_H

#include "./Fence.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_Fence : public Fence
        {
        public:
            explicit D3D12_Fence(D3D12_Context* ctx);
            ~D3D12_Fence() override;

            void reset() override;
            void create() override;
            void waitFor() override;
            void destroy() override;

            void signalOnQueue(ID3D12CommandQueue* queue);
            ID3D12Fence* fence() const { return m_fence.Get(); }
            uint64_t targetValue() const { return m_targetValue; }

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
            HANDLE m_event;
            uint64_t m_lastSignaledValue;
            uint64_t m_targetValue;
        };
    }
}

#endif // FCT_D3D12_FENCE_H

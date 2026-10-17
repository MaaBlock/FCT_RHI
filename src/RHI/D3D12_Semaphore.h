#ifndef FCT_D3D12_SEMAPHORE_H
#define FCT_D3D12_SEMAPHORE_H

#include "./Semaphore.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_Semaphore : public Semaphore
        {
        public:
            explicit D3D12_Semaphore(D3D12_Context* ctx);
            ~D3D12_Semaphore() override;

            void create() override;
            void destroy() override;

            void signalOnQueue(ID3D12CommandQueue* queue);
            void waitOnQueue(ID3D12CommandQueue* queue);

            ID3D12Fence* fence() const { return m_fence.Get(); }
            uint64_t currentValue() const { return m_value; }

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
            uint64_t m_value;
        };
    }
}

#endif // FCT_D3D12_SEMAPHORE_H

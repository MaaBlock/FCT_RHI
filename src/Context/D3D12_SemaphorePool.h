#ifndef FCT_D3D12_SEMAPHOREPOOL_H
#define FCT_D3D12_SEMAPHOREPOOL_H

#include "./SemaphorePool.h"
#include "../RHI/D3D12_Semaphore.h"
#include "../Memory/ObjectPool.h"

namespace FCT
{
    class D3D12_SemaphorePool : public SemaphorePool
    {
    protected:
        ObjectPool<RHI::D3D12_Semaphore, D3D12_Context*> m_semaphorePool;
    public:
        explicit D3D12_SemaphorePool(D3D12_Context* ctx)
            : m_semaphorePool(ctx)
        {
            m_semaphorePool.setDestroyCallback([](RHI::D3D12_Semaphore* semaphore)
            {
                semaphore->destroy();
            });
        }

        void setDestroyCallback(const std::function<void(RHI::Semaphore*)>& callback) override
        {
            m_semaphorePool.setDestroyCallback([callback](RHI::D3D12_Semaphore* semaphore)
            {
                semaphore->destroy();
                callback(semaphore);
            });
        }

        RHI::Semaphore* alloc() override
        {
            return m_semaphorePool.alloc();
        }
    };
}

#endif // FCT_D3D12_SEMAPHOREPOOL_H

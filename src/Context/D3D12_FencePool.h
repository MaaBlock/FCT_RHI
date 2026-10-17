#ifndef FCT_D3D12_FENCEPOOL_H
#define FCT_D3D12_FENCEPOOL_H

#include "../Memory/ObjectPool.h"
#include "../RHI/D3D12_Fence.h"
#include "./FencePool.h"

namespace FCT
{
    class D3D12_FencePool : public FencePool
    {
    protected:
        ObjectPool<RHI::D3D12_Fence, D3D12_Context*> m_fencePool;
    public:
        explicit D3D12_FencePool(D3D12_Context* ctx)
            : m_fencePool(ctx)
        {
            m_fencePool.setDestroyCallback([](RHI::D3D12_Fence* fence)
            {
                fence->destroy();
            });
        }

        void setDestroyCallback(const std::function<void(RHI::Fence*)>& callback) override
        {
            m_fencePool.setDestroyCallback([callback](RHI::D3D12_Fence* fence)
            {
                fence->destroy();
                callback(fence);
            });
        }

        RHI::Fence* alloc() override
        {
            return m_fencePool.alloc();
        }
    };
}

#endif // FCT_D3D12_FENCEPOOL_H

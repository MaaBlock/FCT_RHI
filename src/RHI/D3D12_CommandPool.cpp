#include "../FCTAPI.h"
#include "./D3D12_CommandPool.h"
#include "./D3D12_CommandBuffer.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_CommandPool::D3D12_CommandPool(D3D12_Context* ctx)
            : m_ctx(ctx)
        {
        }

        void D3D12_CommandPool::create()
        {
        }

        CommandBuffer* D3D12_CommandPool::createCommandBuffer()
        {
            return new D3D12_CommandBuffer(this);
        }
    }
}

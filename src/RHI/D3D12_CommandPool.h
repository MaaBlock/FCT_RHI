#ifndef FCT_D3D12_COMMANDPOOL_H
#define FCT_D3D12_COMMANDPOOL_H

#include "./CommandPool.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_CommandPool : public CommandPool
        {
        public:
            explicit D3D12_CommandPool(D3D12_Context* ctx);
            ~D3D12_CommandPool() override = default;

            void create() override;
            CommandBuffer* createCommandBuffer() override;

            D3D12_Context* context() const { return m_ctx; }

        private:
            D3D12_Context* m_ctx;
        };
    }
}

#endif // FCT_D3D12_COMMANDPOOL_H

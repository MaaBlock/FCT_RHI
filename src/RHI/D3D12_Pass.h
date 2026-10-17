#ifndef FCT_D3D12_PASS_H
#define FCT_D3D12_PASS_H

#include "./Pass.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_PassGroup;
        class D3D12_CommandBuffer;

        class D3D12_Pass : public Pass
        {
        public:
            explicit D3D12_Pass(D3D12_Context* ctx);
            ~D3D12_Pass() override = default;

            void create(PassGroup* group) override;
            void executeClear(CommandBuffer* cmdBuf) override;
            void beginSubmit(CommandBuffer* cmdBuf) override;
            void endSubmit() override;

            void bindTargets(D3D12_CommandBuffer* d3dCmd);

        private:
            D3D12_Context* m_ctx;
        };
    }
}

#endif // FCT_D3D12_PASS_H

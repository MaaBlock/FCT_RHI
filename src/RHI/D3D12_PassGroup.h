#ifndef FCT_D3D12_PASSGROUP_H
#define FCT_D3D12_PASSGROUP_H

#include "./PassGroup.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_Pass;
        class D3D12_CommandBuffer;

        class D3D12_PassGroup : public PassGroup
        {
        public:
            explicit D3D12_PassGroup(D3D12_Context* ctx);
            ~D3D12_PassGroup() override;

            void create() override;
            void beginSubmit(CommandBuffer* cmdBuf) override;
            void endSubmit(CommandBuffer* cmdBuf) override;
            void nextPass(CommandBuffer* cmdBuf) override;

            void onPassBegin(D3D12_Pass* pass, D3D12_CommandBuffer* d3dCmd);

        private:
            D3D12_Context* m_ctx;
            size_t m_currentPassIndex;
            D3D12_Pass* m_activeBoundPass;
            bool m_transitionsToPresent;
        };
    }
}

#endif // FCT_D3D12_PASSGROUP_H

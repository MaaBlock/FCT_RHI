#ifndef FCT_D3D12_BLENDSTATE_H
#define FCT_D3D12_BLENDSTATE_H

#include "./BlendState.h"

namespace FCT
{
    class D3D12_Context;

    class D3D12_BlendState : public BlendState
    {
    public:
        explicit D3D12_BlendState(D3D12_Context* ctx);
        ~D3D12_BlendState() override = default;

        void create() override;

        const D3D12_BLEND_DESC& getBlendDesc() const { return m_blendDesc; }

    private:
        D3D12_BLEND convertBlendFactor(BlendFactor factor, bool isAlpha) const;
        D3D12_BLEND_OP convertBlendOp(BlendOp op) const;
        D3D12_LOGIC_OP convertLogicOp(LogicOp op) const;

    private:
        D3D12_Context* m_ctx;
        D3D12_BLEND_DESC m_blendDesc;
    };
}

#endif // FCT_D3D12_BLENDSTATE_H

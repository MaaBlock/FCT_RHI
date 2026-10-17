#ifndef FCT_D3D12_DEPTHSTENCILSTATE_H
#define FCT_D3D12_DEPTHSTENCILSTATE_H

#include "./DepthStencilState.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_DepthStencilState : public DepthStencilState
        {
        public:
            explicit D3D12_DepthStencilState(D3D12_Context* ctx);
            ~D3D12_DepthStencilState() override = default;

            void create() override;

            const D3D12_DEPTH_STENCIL_DESC& getDepthStencilDesc() const { return m_depthStencilDesc; }

        private:
            D3D12_COMPARISON_FUNC convertCompareOp(CompareOp op) const;
            D3D12_STENCIL_OP convertStencilOp(StencilOp op) const;
            D3D12_DEPTH_STENCILOP_DESC convertStencilOpState(const StencilOpState& state) const;

        private:
            D3D12_Context* m_ctx;
            D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc;
        };
    }
}

#endif // FCT_D3D12_DEPTHSTENCILSTATE_H

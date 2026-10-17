#ifndef FCT_D3D12_RASTERIZATIONSTATE_H
#define FCT_D3D12_RASTERIZATIONSTATE_H

#include "./RasterizationState.h"

namespace FCT
{
    class D3D12_Context;

    class D3D12_RasterizationState : public RasterizationState
    {
    public:
        explicit D3D12_RasterizationState(D3D12_Context* ctx);
        ~D3D12_RasterizationState() override = default;

        void create() override;

        const D3D12_RASTERIZER_DESC& getRasterizerDesc() const { return m_rasterizerDesc; }

    private:
        D3D12_FILL_MODE convertFillMode(PolygonMode mode) const;
        D3D12_CULL_MODE convertCullMode(CullMode mode) const;

    private:
        D3D12_Context* m_ctx;
        D3D12_RASTERIZER_DESC m_rasterizerDesc;
    };
}

#endif // FCT_D3D12_RASTERIZATIONSTATE_H

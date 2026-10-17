#include "../FCTAPI.h"
#include "./D3D12_RasterizationState.h"

namespace FCT
{
    D3D12_RasterizationState::D3D12_RasterizationState(D3D12_Context* ctx)
        : m_ctx(ctx)
        , m_rasterizerDesc{}
    {
        D3D12_RasterizationState::create();
    }

    D3D12_FILL_MODE D3D12_RasterizationState::convertFillMode(PolygonMode mode) const
    {
        switch (mode)
        {
        case PolygonMode::Line:
            return D3D12_FILL_MODE_WIREFRAME;
        case PolygonMode::Fill:
        case PolygonMode::Point:
        default:
            return D3D12_FILL_MODE_SOLID;
        }
    }

    D3D12_CULL_MODE D3D12_RasterizationState::convertCullMode(CullMode mode) const
    {
        switch (mode)
        {
        case CullMode::None:
            return D3D12_CULL_MODE_NONE;
        case CullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case CullMode::Back:
        case CullMode::FrontAndBack:
        default:
            return D3D12_CULL_MODE_BACK;
        }
    }

    void D3D12_RasterizationState::create()
    {
        m_rasterizerDesc = {};
        m_rasterizerDesc.FillMode = convertFillMode(m_polygonMode);
        m_rasterizerDesc.CullMode = convertCullMode(m_cullMode);
        m_rasterizerDesc.FrontCounterClockwise = (m_frontFace == FrontFace::CounterClockwise) ? TRUE : FALSE;
        m_rasterizerDesc.DepthBias = m_depthBiasEnable ? static_cast<INT>(m_depthBiasConstantFactor) : D3D12_DEFAULT_DEPTH_BIAS;
        m_rasterizerDesc.DepthBiasClamp = m_depthBiasEnable ? m_depthBiasClamp : D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        m_rasterizerDesc.SlopeScaledDepthBias = m_depthBiasEnable ? m_depthBiasSlopeFactor : D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        m_rasterizerDesc.DepthClipEnable = m_depthClampEnable ? FALSE : TRUE;
        m_rasterizerDesc.MultisampleEnable = m_multisampleEnable ? TRUE : FALSE;
        m_rasterizerDesc.AntialiasedLineEnable = FALSE;
        m_rasterizerDesc.ForcedSampleCount = 0;
        m_rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
}

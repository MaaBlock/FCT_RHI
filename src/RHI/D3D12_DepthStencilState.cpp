#include "../FCTAPI.h"
#include "./D3D12_DepthStencilState.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_DepthStencilState::D3D12_DepthStencilState(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_depthStencilDesc{}
        {
            D3D12_DepthStencilState::create();
        }

        D3D12_COMPARISON_FUNC D3D12_DepthStencilState::convertCompareOp(CompareOp op) const
        {
            switch (op)
            {
            case CompareOp::Never:
                return D3D12_COMPARISON_FUNC_NEVER;
            case CompareOp::Less:
                return D3D12_COMPARISON_FUNC_LESS;
            case CompareOp::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case CompareOp::LessOrEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case CompareOp::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case CompareOp::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case CompareOp::GreaterOrEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case CompareOp::Always:
            default:
                return D3D12_COMPARISON_FUNC_ALWAYS;
            }
        }

        D3D12_STENCIL_OP D3D12_DepthStencilState::convertStencilOp(StencilOp op) const
        {
            switch (op)
            {
            case StencilOp::Keep:
                return D3D12_STENCIL_OP_KEEP;
            case StencilOp::Zero:
                return D3D12_STENCIL_OP_ZERO;
            case StencilOp::Replace:
                return D3D12_STENCIL_OP_REPLACE;
            case StencilOp::IncrementClamp:
                return D3D12_STENCIL_OP_INCR_SAT;
            case StencilOp::DecrementClamp:
                return D3D12_STENCIL_OP_DECR_SAT;
            case StencilOp::Invert:
                return D3D12_STENCIL_OP_INVERT;
            case StencilOp::IncrementWrap:
                return D3D12_STENCIL_OP_INCR;
            case StencilOp::DecrementWrap:
                return D3D12_STENCIL_OP_DECR;
            default:
                return D3D12_STENCIL_OP_KEEP;
            }
        }

        D3D12_DEPTH_STENCILOP_DESC D3D12_DepthStencilState::convertStencilOpState(const StencilOpState& state) const
        {
            D3D12_DEPTH_STENCILOP_DESC desc{};
            desc.StencilFailOp = convertStencilOp(state.failOp);
            desc.StencilDepthFailOp = convertStencilOp(state.depthFailOp);
            desc.StencilPassOp = convertStencilOp(state.passOp);
            desc.StencilFunc = convertCompareOp(state.compareOp);
            return desc;
        }

        void D3D12_DepthStencilState::create()
        {
            m_depthStencilDesc = {};
            m_depthStencilDesc.DepthEnable = m_depthTestEnable ? TRUE : FALSE;
            m_depthStencilDesc.DepthWriteMask = m_depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            m_depthStencilDesc.DepthFunc = convertCompareOp(m_depthCompareOp);
            m_depthStencilDesc.StencilEnable = m_stencilTestEnable ? TRUE : FALSE;
            m_depthStencilDesc.StencilReadMask = static_cast<UINT8>(m_front.compareMask & 0xFF);
            m_depthStencilDesc.StencilWriteMask = static_cast<UINT8>(m_front.writeMask & 0xFF);
            m_depthStencilDesc.FrontFace = convertStencilOpState(m_front);
            m_depthStencilDesc.BackFace = convertStencilOpState(m_back);
        }
    }
}

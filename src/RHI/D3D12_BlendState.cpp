#include "../FCTAPI.h"
#include "./D3D12_BlendState.h"

namespace FCT
{
    D3D12_BlendState::D3D12_BlendState(D3D12_Context* ctx)
        : m_ctx(ctx)
        , m_blendDesc{}
    {
        D3D12_BlendState::create();
    }

    D3D12_BLEND D3D12_BlendState::convertBlendFactor(BlendFactor factor, bool isAlpha) const
    {
        switch (factor)
        {
        case BlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case BlendFactor::One:
            return D3D12_BLEND_ONE;
        case BlendFactor::SrcColor:
            return isAlpha ? D3D12_BLEND_SRC_ALPHA : D3D12_BLEND_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return isAlpha ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactor::DstColor:
            return isAlpha ? D3D12_BLEND_DEST_ALPHA : D3D12_BLEND_DEST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return isAlpha ? D3D12_BLEND_INV_DEST_ALPHA : D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case BlendFactor::ConstantColor:
            return D3D12_BLEND_BLEND_FACTOR;
        case BlendFactor::OneMinusConstantColor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case BlendFactor::Src1Color:
            return isAlpha ? D3D12_BLEND_SRC1_ALPHA : D3D12_BLEND_SRC1_COLOR;
        case BlendFactor::OneMinusSrc1Color:
            return isAlpha ? D3D12_BLEND_INV_SRC1_ALPHA : D3D12_BLEND_INV_SRC1_COLOR;
        case BlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case BlendFactor::OneMinusSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ONE;
        }
    }

    D3D12_BLEND_OP D3D12_BlendState::convertBlendOp(BlendOp op) const
    {
        switch (op)
        {
        case BlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case BlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case BlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
        }
    }

    D3D12_LOGIC_OP D3D12_BlendState::convertLogicOp(LogicOp op) const
    {
        switch (op)
        {
        case LogicOp::Clear:
            return D3D12_LOGIC_OP_CLEAR;
        case LogicOp::And:
            return D3D12_LOGIC_OP_AND;
        case LogicOp::AndReverse:
            return D3D12_LOGIC_OP_AND_REVERSE;
        case LogicOp::Copy:
            return D3D12_LOGIC_OP_COPY;
        case LogicOp::AndInverted:
            return D3D12_LOGIC_OP_AND_INVERTED;
        case LogicOp::NoOp:
            return D3D12_LOGIC_OP_NOOP;
        case LogicOp::Xor:
            return D3D12_LOGIC_OP_XOR;
        case LogicOp::Or:
            return D3D12_LOGIC_OP_OR;
        case LogicOp::Nor:
            return D3D12_LOGIC_OP_NOR;
        case LogicOp::Equivalent:
            return D3D12_LOGIC_OP_EQUIV;
        case LogicOp::Invert:
            return D3D12_LOGIC_OP_INVERT;
        case LogicOp::OrReverse:
            return D3D12_LOGIC_OP_OR_REVERSE;
        case LogicOp::CopyInverted:
            return D3D12_LOGIC_OP_COPY_INVERTED;
        case LogicOp::OrInverted:
            return D3D12_LOGIC_OP_OR_INVERTED;
        case LogicOp::Nand:
            return D3D12_LOGIC_OP_NAND;
        case LogicOp::Set:
            return D3D12_LOGIC_OP_SET;
        default:
            return D3D12_LOGIC_OP_NOOP;
        }
    }

    void D3D12_BlendState::create()
    {
        m_blendDesc = {};
        m_blendDesc.AlphaToCoverageEnable = FALSE;
        m_blendDesc.IndependentBlendEnable = (m_targets.size() > 1) ? TRUE : FALSE;

        for (int i = 0; i < 8; ++i)
        {
            Target target = (!m_targets.empty())
                ? m_targets[static_cast<size_t>(i) < m_targets.size() ? i : 0]
                : Target{};

            D3D12_RENDER_TARGET_BLEND_DESC rtBlend{};
            rtBlend.BlendEnable = target.enable ? TRUE : FALSE;
            rtBlend.LogicOpEnable = m_logicEnable ? TRUE : FALSE;
            rtBlend.SrcBlend = convertBlendFactor(target.srcColor, false);
            rtBlend.DestBlend = convertBlendFactor(target.dstColor, false);
            rtBlend.BlendOp = convertBlendOp(target.colorOp);
            rtBlend.SrcBlendAlpha = convertBlendFactor(target.srcAlpha, true);
            rtBlend.DestBlendAlpha = convertBlendFactor(target.dstAlpha, true);
            rtBlend.BlendOpAlpha = convertBlendOp(target.alphaOp);
            rtBlend.LogicOp = convertLogicOp(m_logicOp);
            rtBlend.RenderTargetWriteMask = target.mask & 0x0F;

            m_blendDesc.RenderTarget[i] = rtBlend;
        }
    }
}

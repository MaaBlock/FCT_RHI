#include "../FCTAPI.h"
#include "./D3D12_Sampler.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    D3D12_Sampler::D3D12_Sampler(D3D12_Context* ctx)
        : m_ctx(ctx)
        , m_cpuHandle{0}
        , m_descriptorIndex(UINT32_MAX)
    {
    }

    D3D12_Sampler::~D3D12_Sampler()
    {
        releaseDescriptor();
    }

    void D3D12_Sampler::releaseDescriptor()
    {
        if (m_descriptorIndex != UINT32_MAX && m_ctx)
        {
            m_ctx->freeStagingSamplerDescriptor(m_descriptorIndex);
            m_descriptorIndex = UINT32_MAX;
            m_cpuHandle = {0};
        }
    }

    D3D12_FILTER D3D12_Sampler::toD3D12Filter() const
    {
        if (m_anisotropyEnable)
        {
            return m_compareEnable
                ? D3D12_FILTER_COMPARISON_ANISOTROPIC
                : D3D12_FILTER_ANISOTROPIC;
        }

        D3D12_FILTER_TYPE minType = (m_minFilter == FilterMode::Linear)
            ? D3D12_FILTER_TYPE_LINEAR
            : D3D12_FILTER_TYPE_POINT;
        D3D12_FILTER_TYPE magType = (m_magFilter == FilterMode::Linear)
            ? D3D12_FILTER_TYPE_LINEAR
            : D3D12_FILTER_TYPE_POINT;
        D3D12_FILTER_TYPE mipType = (m_mipmapFilter == FilterMode::Linear)
            ? D3D12_FILTER_TYPE_LINEAR
            : D3D12_FILTER_TYPE_POINT;
        D3D12_FILTER_REDUCTION_TYPE reduction = m_compareEnable
            ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON
            : D3D12_FILTER_REDUCTION_TYPE_STANDARD;

        return D3D12_ENCODE_BASIC_FILTER(minType, magType, mipType, reduction);
    }

    D3D12_TEXTURE_ADDRESS_MODE D3D12_Sampler::toD3D12AddressMode(AddressMode mode) const
    {
        switch (mode)
        {
        case AddressMode::Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case AddressMode::MirroredRepeat:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case AddressMode::ClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case AddressMode::ClampToBorder:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case AddressMode::MirrorClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    D3D12_COMPARISON_FUNC D3D12_Sampler::toD3D12CompareFunc(CompareOp op) const
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

    void D3D12_Sampler::fillBorderColor(float outBorder[4]) const
    {
        switch (m_borderColor)
        {
        case BorderColor::TransparentBlack:
            outBorder[0] = 0.0f; outBorder[1] = 0.0f; outBorder[2] = 0.0f; outBorder[3] = 0.0f;
            break;
        case BorderColor::OpaqueBlack:
            outBorder[0] = 0.0f; outBorder[1] = 0.0f; outBorder[2] = 0.0f; outBorder[3] = 1.0f;
            break;
        case BorderColor::OpaqueWhite:
        default:
            outBorder[0] = 1.0f; outBorder[1] = 1.0f; outBorder[2] = 1.0f; outBorder[3] = 1.0f;
            break;
        }
    }

    void D3D12_Sampler::create()
    {
        releaseDescriptor();
        m_descriptorIndex = m_ctx->allocStagingSamplerDescriptor(m_cpuHandle);

        D3D12_SAMPLER_DESC desc{};
        desc.Filter = toD3D12Filter();
        desc.AddressU = toD3D12AddressMode(m_addressModeU);
        desc.AddressV = toD3D12AddressMode(m_addressModeV);
        desc.AddressW = toD3D12AddressMode(m_addressModeW);
        desc.MipLODBias = m_mipLodBias;
        desc.MaxAnisotropy = static_cast<UINT>(m_maxAnisotropy > 1.0f ? m_maxAnisotropy : 1.0f);
        desc.ComparisonFunc = m_compareEnable ? toD3D12CompareFunc(m_compareOp) : D3D12_COMPARISON_FUNC_ALWAYS;
        fillBorderColor(desc.BorderColor);
        desc.MinLOD = m_minLod;
        desc.MaxLOD = m_maxLod;

        m_ctx->getDevice()->CreateSampler(&desc, m_cpuHandle);
    }
}

#include "./WGL2_Sampler.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_Sampler::WGL2_Sampler(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_Sampler::~WGL2_Sampler()
{
    if (m_samplerHandle != 0) {
        glDeleteSamplers(1, &m_samplerHandle);
        m_samplerHandle = 0;
    }
}

GLenum WGL2_Sampler::convertFilter(FilterMode mode, FilterMode mipMode, bool isMinFilter) const
{
    if (!isMinFilter) {
        return (mode == FilterMode::Nearest) ? GL_NEAREST : GL_LINEAR;
    }

    if (mode == FilterMode::Nearest) {
        return (mipMode == FilterMode::Nearest) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
    } else {
        return (mipMode == FilterMode::Nearest) ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
    }
}

GLenum WGL2_Sampler::convertAddressMode(AddressMode mode) const
{
    switch (mode) {
    case AddressMode::Repeat: return GL_REPEAT;
    case AddressMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
    case AddressMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
    case AddressMode::ClampToBorder: return GL_CLAMP_TO_EDGE; // GLES 3.0 ES 无 ClampToBorder，降级为 ClampToEdge
    default: return GL_CLAMP_TO_EDGE;
    }
}

GLenum WGL2_Sampler::convertCompareOp(CompareOp op) const
{
    switch (op) {
    case CompareOp::Never: return GL_NEVER;
    case CompareOp::Less: return GL_LESS;
    case CompareOp::Equal: return GL_EQUAL;
    case CompareOp::LessOrEqual: return GL_LEQUAL;
    case CompareOp::Greater: return GL_GREATER;
    case CompareOp::NotEqual: return GL_NOTEQUAL;
    case CompareOp::GreaterOrEqual: return GL_GEQUAL;
    case CompareOp::Always: return GL_ALWAYS;
    default: return GL_ALWAYS;
    }
}

void WGL2_Sampler::create()
{
    if (m_samplerHandle != 0) {
        return;
    }

    glGenSamplers(1, &m_samplerHandle);
    if (m_samplerHandle == 0) {
        return;
    }

    glSamplerParameteri(m_samplerHandle, GL_TEXTURE_MIN_FILTER, convertFilter(m_minFilter, m_mipmapFilter, true));
    glSamplerParameteri(m_samplerHandle, GL_TEXTURE_MAG_FILTER, convertFilter(m_magFilter, m_mipmapFilter, false));
    glSamplerParameteri(m_samplerHandle, GL_TEXTURE_WRAP_S, convertAddressMode(m_addressModeU));
    glSamplerParameteri(m_samplerHandle, GL_TEXTURE_WRAP_T, convertAddressMode(m_addressModeV));
    glSamplerParameteri(m_samplerHandle, GL_TEXTURE_WRAP_R, convertAddressMode(m_addressModeW));

    glSamplerParameterf(m_samplerHandle, GL_TEXTURE_MIN_LOD, m_minLod);
    glSamplerParameterf(m_samplerHandle, GL_TEXTURE_MAX_LOD, m_maxLod);

    if (m_compareEnable) {
        glSamplerParameteri(m_samplerHandle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(m_samplerHandle, GL_TEXTURE_COMPARE_FUNC, convertCompareOp(m_compareOp));
    } else {
        glSamplerParameteri(m_samplerHandle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }
}

void WGL2_Sampler::bind(uint32_t unit) const
{
    if (m_samplerHandle != 0) {
        glBindSampler(unit, m_samplerHandle);
    }
}

} // namespace FCT::RHI

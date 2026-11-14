#pragma once

#include "./Sampler.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Sampler : public Sampler {
public:
    explicit WGL2_Sampler(WGL2_Context* ctx);
    ~WGL2_Sampler() override;

    void create() override;
    GLuint getSamplerHandle() const noexcept { return m_samplerHandle; }
    void bind(uint32_t unit) const;

private:
    GLenum convertFilter(FilterMode mode, FilterMode mipMode = FilterMode::Linear, bool isMinFilter = false) const;
    GLenum convertAddressMode(AddressMode mode) const;
    GLenum convertCompareOp(CompareOp op) const;

    WGL2_Context* m_ctx{nullptr};
    GLuint m_samplerHandle{0};
};

} // namespace RHI
} // namespace FCT

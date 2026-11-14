#pragma once

#include "./PixelShader.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_PixelShader : public PixelShader {
public:
    explicit WGL2_PixelShader(WGL2_Context* ctx);
    ~WGL2_PixelShader() override;

    void create() override;
    GLuint getShaderHandle() const noexcept { return m_shaderHandle; }
    GLuint getShaderId() const noexcept { return m_shaderHandle; }

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_shaderHandle{0};
};

} // namespace RHI
} // namespace FCT

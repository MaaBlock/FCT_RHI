#pragma once

#include "./VertexShader.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_VertexShader : public VertexShader {
public:
    explicit WGL2_VertexShader(WGL2_Context* ctx);
    ~WGL2_VertexShader() override;

    void create() override;
    GLuint getShaderHandle() const noexcept { return m_shaderHandle; }
    GLuint getShaderId() const noexcept { return m_shaderHandle; }

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_shaderHandle{0};
};

} // namespace RHI
} // namespace FCT

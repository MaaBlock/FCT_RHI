#pragma once

#include "./TextureView.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Image;

class WGL2_TextureView : public TextureView {
public:
    explicit WGL2_TextureView(WGL2_Context* ctx);
    ~WGL2_TextureView() override = default;

    void create() override;
    GLuint getTextureHandle() const noexcept;
    GLenum getGLTarget() const noexcept;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT

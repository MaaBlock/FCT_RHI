#pragma once

#include "./RenderTargetView.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_RenderTargetView : public RenderTargetView {
public:
    explicit WGL2_RenderTargetView(WGL2_Context* ctx);
    ~WGL2_RenderTargetView() override = default;

    void create() override;
    GLuint getAttachmentHandle() const noexcept;
    bool isRenderbuffer() const noexcept;
    GLenum getGLTarget() const noexcept;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT

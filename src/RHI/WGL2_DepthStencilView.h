#pragma once

#include "./DepthStencilView.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_DepthStencilView : public DepthStencilView {
public:
    explicit WGL2_DepthStencilView(WGL2_Context* ctx);
    ~WGL2_DepthStencilView() override = default;

    bool create() override;
    GLuint getAttachmentHandle() const noexcept;
    bool isRenderbuffer() const noexcept;
    GLenum getGLTarget() const noexcept;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT

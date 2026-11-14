#include "./WGL2_RenderTargetView.h"
#include "./WGL2_Image.h"

namespace FCT::RHI {

WGL2_RenderTargetView::WGL2_RenderTargetView(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_RenderTargetView::create()
{
}

GLuint WGL2_RenderTargetView::getAttachmentHandle() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->getTextureHandle();
        }
    }
    return 0;
}

bool WGL2_RenderTargetView::isRenderbuffer() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->isRenderbuffer();
        }
    }
    return false;
}

GLenum WGL2_RenderTargetView::getGLTarget() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->getGLTarget();
        }
    }
    return GL_TEXTURE_2D;
}

} // namespace FCT::RHI

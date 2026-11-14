#include "./WGL2_DepthStencilView.h"
#include "./WGL2_Image.h"

namespace FCT::RHI {

WGL2_DepthStencilView::WGL2_DepthStencilView(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

bool WGL2_DepthStencilView::create()
{
    return true;
}

GLuint WGL2_DepthStencilView::getAttachmentHandle() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->getTextureHandle();
        }
    }
    return 0;
}

bool WGL2_DepthStencilView::isRenderbuffer() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->isRenderbuffer();
        }
    }
    return false;
}

GLenum WGL2_DepthStencilView::getGLTarget() const noexcept
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

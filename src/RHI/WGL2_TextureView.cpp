#include "./WGL2_TextureView.h"
#include "./WGL2_Image.h"

namespace FCT::RHI {

WGL2_TextureView::WGL2_TextureView(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_TextureView::create()
{
}

GLuint WGL2_TextureView::getTextureHandle() const noexcept
{
    if (m_image) {
        auto* img = dynamic_cast<WGL2_Image*>(m_image);
        if (img) {
            return img->getTextureHandle();
        }
    }
    return 0;
}

GLenum WGL2_TextureView::getGLTarget() const noexcept
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

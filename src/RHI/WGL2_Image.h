#pragma once

#include "./Image.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Image : public Image {
public:
    explicit WGL2_Image(WGL2_Context* ctx);
    ~WGL2_Image() override;

    void create() override;
    void create(bool uploadData) override;
    void updateData(const void* data, size_t dataSize) override;
    void updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion) override;
    void updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch = 0) override;
    void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) override;

    GLuint getTextureHandle() const noexcept { return m_textureHandle; }
    GLenum getGLTarget() const noexcept { return m_glTarget; }
    bool isRenderbuffer() const noexcept override { return m_isRenderbuffer; }
    void setRenderbuffer(bool renderbuffer) noexcept override;

    void createFromExisting(GLuint handle, GLenum target, bool isRenderbuffer = false);

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_textureHandle{0};
    GLenum m_glTarget{GL_TEXTURE_2D};
    bool m_isRenderbuffer{false};
    bool m_ownsHandle{true};
};

} // namespace RHI
} // namespace FCT

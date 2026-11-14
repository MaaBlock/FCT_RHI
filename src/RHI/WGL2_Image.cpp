#include "./WGL2_Image.h"
#include "../Context/WGL2_Context.h"
#include "./Fence.h"
#include <cassert>

namespace FCT::RHI {

WGL2_Image::WGL2_Image(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_Image::~WGL2_Image()
{
    if (m_textureHandle != 0 && m_ownsHandle) {
        if (m_isRenderbuffer) {
            glDeleteRenderbuffers(1, &m_textureHandle);
        } else {
            glDeleteTextures(1, &m_textureHandle);
        }
        m_textureHandle = 0;
    }
}

void WGL2_Image::createFromExisting(GLuint handle, GLenum target, bool isRenderbuffer)
{
    if (m_textureHandle != 0 && m_ownsHandle) {
        if (m_isRenderbuffer) {
            glDeleteRenderbuffers(1, &m_textureHandle);
        } else {
            glDeleteTextures(1, &m_textureHandle);
        }
    }
    m_textureHandle = handle;
    m_glTarget = target;
    m_isRenderbuffer = isRenderbuffer;
    m_ownsHandle = false;
}

void WGL2_Image::setRenderbuffer(bool renderbuffer) noexcept
{
    if (m_isRenderbuffer == renderbuffer) {
        return;
    }
    if (m_textureHandle != 0 && m_ownsHandle) {
        if (m_isRenderbuffer) {
            glDeleteRenderbuffers(1, &m_textureHandle);
        } else {
            glDeleteTextures(1, &m_textureHandle);
        }
        m_textureHandle = 0;
        m_isRenderbuffer = renderbuffer;
        create();
        return;
    }
    m_isRenderbuffer = renderbuffer;
}

void WGL2_Image::create()
{
    create(true);
}

void WGL2_Image::create(bool uploadData)
{
    if (m_textureHandle != 0) {
        return;
    }

    if (m_isRenderbuffer) {
        glGenRenderbuffers(1, &m_textureHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, m_textureHandle);
        GLenum internalFormat = WGL2_TranslateInternalFormat(m_format);

        uint32_t sampleCount = 1;
        switch (m_samples) {
        case Samples::sample_2: sampleCount = 2; break;
        case Samples::sample_4: sampleCount = 4; break;
        case Samples::sample_8: sampleCount = 8; break;
        case Samples::sample_16: sampleCount = 16; break;
        default: sampleCount = 1; break;
        }

        if (sampleCount > 1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(sampleCount), internalFormat, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return;
    }

    // 纹理对象创建
    if (m_isCubeMap) {
        m_glTarget = GL_TEXTURE_CUBE_MAP;
    } else if (m_arrayLayers > 1) {
        m_glTarget = GL_TEXTURE_2D_ARRAY;
    } else {
        m_glTarget = GL_TEXTURE_2D;
    }

    glGenTextures(1, &m_textureHandle);
    glBindTexture(m_glTarget, m_textureHandle);

    glTexParameteri(m_glTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_glTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_glTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_glTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (m_glTarget == GL_TEXTURE_2D_ARRAY || m_glTarget == GL_TEXTURE_CUBE_MAP) {
        glTexParameteri(m_glTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    GLenum internalFormat = WGL2_TranslateInternalFormat(m_format);
    GLenum format = WGL2_TranslateFormat(m_format);
    GLenum type = WGL2_TranslateType(m_format);

    const void* initialPixels = (uploadData && m_initData.data != nullptr) ? m_initData.data : nullptr;

    if (m_glTarget == GL_TEXTURE_2D_ARRAY) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, static_cast<GLint>(internalFormat),
                     static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
                     static_cast<GLsizei>(m_arrayLayers), 0, format, type, initialPixels);
    } else if (m_glTarget == GL_TEXTURE_CUBE_MAP) {
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, static_cast<GLint>(internalFormat),
                         static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
                         0, format, type, nullptr);
        }
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                     static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
                     0, format, type, initialPixels);
    }

    glBindTexture(m_glTarget, 0);
}

void WGL2_Image::updateData(const void* data, size_t dataSize)
{
    if (data == nullptr || m_textureHandle == 0 || m_isRenderbuffer) {
        return;
    }
    updateSubRegion(0, 0, m_width, m_height, 0, 0, data, 0);
}

void WGL2_Image::updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion)
{
    updateData(data, dataSize);
    if (fence != nullptr) {
        fence->createSignaled();
    }
    if (onCompletion != nullptr && *onCompletion) {
        (*onCompletion)();
    }
}

void WGL2_Image::updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch)
{
    assert(pixelData != nullptr && "Pixel data cannot be null");
    if (m_textureHandle == 0 || m_isRenderbuffer) {
        return;
    }

    GLenum format = WGL2_TranslateFormat(m_format);
    GLenum type = WGL2_TranslateType(m_format);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (rowPitch > 0) {
        size_t bpp = WGL2_GetFormatBytesPerPixel(m_format);
        if (bpp > 0) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(rowPitch / bpp));
        }
    }

    glBindTexture(m_glTarget, m_textureHandle);

    if (m_glTarget == GL_TEXTURE_2D_ARRAY) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(mipLevel),
                        static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLint>(arrayLayer),
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), 1,
                        format, type, pixelData);
    } else {
        GLenum target = m_glTarget;
        if (m_isCubeMap) {
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (arrayLayer % 6);
        }
        glTexSubImage2D(target, static_cast<GLint>(mipLevel),
                        static_cast<GLint>(x), static_cast<GLint>(y),
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                        format, type, pixelData);
    }

    glBindTexture(m_glTarget, 0);

    if (rowPitch > 0) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void WGL2_Image::uploadAsync(std::vector<uint8_t> data, std::function<void()> callback)
{
    updateData(data.data(), data.size());
    if (callback) {
        callback();
    }
}

} // namespace FCT::RHI

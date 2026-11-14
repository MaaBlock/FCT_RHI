#pragma once

#include "./IndexBuffer.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_IndexBuffer : public IndexBuffer {
public:
    explicit WGL2_IndexBuffer(WGL2_Context* ctx);
    ~WGL2_IndexBuffer() override;

    void create() override;
    void mapBuffer(void* data, uint32_t size) override;
    void bind(CommandBuffer* buffer) override;
    void resize(size_t size) override;

    GLuint getBufferHandle() const noexcept { return m_bufferHandle; }
    GLenum getIndexType() const noexcept { return (m_stride == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT; }

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_bufferHandle{0};
    size_t m_allocatedBytes{0};
};

} // namespace RHI
} // namespace FCT

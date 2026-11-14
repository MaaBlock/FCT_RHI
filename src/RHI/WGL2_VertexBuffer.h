#pragma once

#include "./VertexBuffer.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_VertexBuffer : public VertexBuffer {
public:
    explicit WGL2_VertexBuffer(WGL2_Context* ctx);
    ~WGL2_VertexBuffer() override;

    void create() override;
    void mapBuffer(void* data, uint32_t size) override;
    void bind(CommandBuffer* srcCmd, uint32_t slot = 0, uint32_t offset = 0) override;
    void resize(uint32_t newSize) override;

    GLuint getBufferHandle() const noexcept { return m_bufferHandle; }

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_bufferHandle{0};
    uint32_t m_allocatedBytes{0};
};

} // namespace RHI
} // namespace FCT

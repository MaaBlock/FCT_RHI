#include "./WGL2_VertexBuffer.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_VertexBuffer::WGL2_VertexBuffer(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_VertexBuffer::~WGL2_VertexBuffer()
{
    if (m_bufferHandle != 0) {
        glDeleteBuffers(1, &m_bufferHandle);
        m_bufferHandle = 0;
    }
}

void WGL2_VertexBuffer::create()
{
    if (m_bufferHandle != 0) {
        glDeleteBuffers(1, &m_bufferHandle);
        m_bufferHandle = 0;
    }
    glGenBuffers(1, &m_bufferHandle);
}

void WGL2_VertexBuffer::mapBuffer(void* data, uint32_t size)
{
    if (m_bufferHandle == 0) {
        create();
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferHandle);
    GLenum usage = (m_updateFrequency == UpdateFrequency::Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    if (size > m_allocatedBytes) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage);
        m_allocatedBytes = size;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WGL2_VertexBuffer::bind(CommandBuffer* srcCmd, uint32_t slot, uint32_t offset)
{
    (void)srcCmd;
    (void)slot;
    (void)offset;
    if (m_bufferHandle != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_bufferHandle);
    }
}

void WGL2_VertexBuffer::resize(uint32_t newSize)
{
    if (newSize > m_allocatedBytes) {
        if (m_bufferHandle == 0) create();
        glBindBuffer(GL_ARRAY_BUFFER, m_bufferHandle);
        GLenum usage = (m_updateFrequency == UpdateFrequency::Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(newSize), nullptr, usage);
        m_allocatedBytes = newSize;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

} // namespace FCT::RHI

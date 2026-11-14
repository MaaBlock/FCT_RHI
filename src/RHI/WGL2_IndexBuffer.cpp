#include "./WGL2_IndexBuffer.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_IndexBuffer::WGL2_IndexBuffer(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_IndexBuffer::~WGL2_IndexBuffer()
{
    if (m_bufferHandle != 0) {
        glDeleteBuffers(1, &m_bufferHandle);
        m_bufferHandle = 0;
    }
}

void WGL2_IndexBuffer::create()
{
    if (m_bufferHandle != 0) {
        glDeleteBuffers(1, &m_bufferHandle);
        m_bufferHandle = 0;
    }
    glGenBuffers(1, &m_bufferHandle);
}

void WGL2_IndexBuffer::mapBuffer(void* data, uint32_t size)
{
    if (m_bufferHandle == 0) {
        create();
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferHandle);
    GLenum usage = (m_updateFrequency == UpdateFrequency::Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    if (size > m_allocatedBytes) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage);
        m_allocatedBytes = size;
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void WGL2_IndexBuffer::bind(CommandBuffer* buffer)
{
    (void)buffer;
    if (m_bufferHandle != 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferHandle);
    }
}

void WGL2_IndexBuffer::resize(size_t size)
{
    size_t newBytes = size * (m_stride > 0 ? m_stride : 4);
    if (newBytes > m_allocatedBytes) {
        if (m_bufferHandle == 0) create();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferHandle);
        GLenum usage = (m_updateFrequency == UpdateFrequency::Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(newBytes), nullptr, usage);
        m_allocatedBytes = newBytes;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

} // namespace FCT::RHI

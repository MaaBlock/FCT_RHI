#include "./WGL2_ConstBuffer.h"
#include "../Context/WGL2_Context.h"
#include <cstring>

namespace FCT::RHI {

WGL2_ConstBuffer::WGL2_ConstBuffer(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_ConstBuffer::~WGL2_ConstBuffer()
{
    if (m_standaloneUbo != 0) {
        glDeleteBuffers(1, &m_standaloneUbo);
        m_standaloneUbo = 0;
    }
}

void WGL2_ConstBuffer::create()
{
    if (m_standaloneUbo != 0) {
        glDeleteBuffers(1, &m_standaloneUbo);
        m_standaloneUbo = 0;
    }
    glGenBuffers(1, &m_standaloneUbo);
}

void WGL2_ConstBuffer::create(const ConstLayout& layout)
{
    m_layout = layout;
    create();
}

void WGL2_ConstBuffer::upload(const void* data, size_t size)
{
    upload(data, size, 0);
}

void WGL2_ConstBuffer::upload(const void* data, size_t size, size_t offset)
{
    if (!data || size == 0) return;

    if (m_ctx) {
        // 优先使用帧环形 UBO
        auto alloc = m_ctx->getFrameRingUBO().allocate(static_cast<uint32_t>(size));
        if (alloc.cpuPtr) {
            std::memcpy(alloc.cpuPtr, data, size);
            m_offset = alloc.offset;
            m_size = alloc.size;
            m_useRingUBO = true;
            return;
        }
    }

    // 回退至独立 UBO
    m_useRingUBO = false;
    m_offset = static_cast<uint32_t>(offset);
    m_size = static_cast<uint32_t>(size);

    if (m_standaloneUbo == 0) {
        glGenBuffers(1, &m_standaloneUbo);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, m_standaloneUbo);
    if (offset == 0) {
        glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), data, GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void WGL2_ConstBuffer::bind(uint32_t slot)
{
    GLuint ubo = getBufferHandle();
    if (ubo != 0) {
        glBindBufferRange(GL_UNIFORM_BUFFER, slot, ubo, static_cast<GLintptr>(m_offset), static_cast<GLsizeiptr>(m_size));
    }
}

GLuint WGL2_ConstBuffer::getBufferHandle() const noexcept
{
    if (m_useRingUBO && m_ctx) {
        return m_ctx->getFrameRingUBO().getBufferHandle();
    }
    return m_standaloneUbo;
}

} // namespace FCT::RHI

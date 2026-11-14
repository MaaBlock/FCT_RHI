#include "./WGL2_Pass.h"
#include "./WGL2_PassGroup.h"
#include "../Context/WGL2_Context.h"
#include "./CommandBuffer.h"

namespace FCT::RHI {

WGL2_Pass::WGL2_Pass(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_Pass::create(PassGroup* group)
{
    m_group = group;
    m_wgl2Group = dynamic_cast<WGL2_PassGroup*>(group);
}

void WGL2_Pass::beginSubmit(CommandBuffer* cmdBuf)
{
    (void)cmdBuf;
}

void WGL2_Pass::endSubmit()
{
}

void WGL2_Pass::executeClear(CommandBuffer* cmdBuf)
{
    (void)cmdBuf;
    if (!m_clearValue.types) {
        return;
    }

    GLbitfield clearMask = 0;
    if (bool(m_clearValue.types & ClearType::color)) {
        glClearColor(m_clearValue.color.x, m_clearValue.color.y, m_clearValue.color.z, m_clearValue.color.w);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }

    if (bool(m_clearValue.types & ClearType::depth)) {
        glClearDepthf(m_clearValue.depth);
        // 确保深度写入开启以执行清除
        glDepthMask(GL_TRUE);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    if (bool(m_clearValue.types & ClearType::stencil)) {
        glClearStencil(static_cast<GLint>(m_clearValue.stencil));
        glStencilMask(0xFF);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }

    if (clearMask != 0) {
        glClear(clearMask);
    }
}

} // namespace FCT::RHI

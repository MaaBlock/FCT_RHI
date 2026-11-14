#include "WGL2_TransformFeedback.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_TransformFeedback::WGL2_TransformFeedback(WGL2_Context* ctx)
    : m_ctx(ctx) {
}

WGL2_TransformFeedback::~WGL2_TransformFeedback() {
    destroy();
}

Status WGL2_TransformFeedback::create() {
    if (m_xfbHandle != 0) {
        return OkStatus();
    }
    glGenTransformFeedbacks(1, &m_xfbHandle);
    if (m_xfbHandle == 0) {
        return InternalError("glGenTransformFeedbacks 失败");
    }
    return OkStatus();
}

void WGL2_TransformFeedback::destroy() {
    if (m_isActive) {
        if (m_isPaused) {
            (void)resume();
        }
        (void)end();
    }
    if (m_xfbHandle != 0) {
        if (m_ctx && m_ctx->getCurrentBoundTransformFeedback() == m_xfbHandle) {
            m_ctx->bindTransformFeedback(0);
        }
        glDeleteTransformFeedbacks(1, &m_xfbHandle);
        m_xfbHandle = 0;
    }
    m_boundBuffers.fill(0);
    m_isActive = false;
    m_isPaused = false;
}

void WGL2_TransformFeedback::bind() {
    if (m_ctx && m_xfbHandle != 0) {
        m_ctx->bindTransformFeedback(m_xfbHandle);
    }
}

void WGL2_TransformFeedback::unbind() {
    if (m_ctx) {
        m_ctx->bindTransformFeedback(0);
    }
}

Status WGL2_TransformFeedback::bindBufferBase(uint32_t slot, uint32_t bufferHandle) {
    if (slot >= m_boundBuffers.size()) {
        return InvalidArgumentError("XFB 槽位索引越界 (必须 < 4)");
    }
    bind();
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, slot, bufferHandle);
    m_boundBuffers[slot] = bufferHandle;
    return OkStatus();
}

Status WGL2_TransformFeedback::bindBufferRange(uint32_t slot, uint32_t bufferHandle, size_t offset, size_t size) {
    if (slot >= m_boundBuffers.size()) {
        return InvalidArgumentError("XFB 槽位索引越界 (必须 < 4)");
    }
    bind();
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, slot, bufferHandle,
                      static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    m_boundBuffers[slot] = bufferHandle;
    return OkStatus();
}

Status WGL2_TransformFeedback::begin(FeedbackPrimitiveMode mode) {
    if (m_isActive) {
        return FailedPreconditionError("Transform feedback 已经处于激活状态");
    }
    bind();
    GLenum glMode = GL_POINTS;
    switch (mode) {
    case FeedbackPrimitiveMode::Points:
        glMode = GL_POINTS;
        break;
    case FeedbackPrimitiveMode::Lines:
        glMode = GL_LINES;
        break;
    case FeedbackPrimitiveMode::Triangles:
        glMode = GL_TRIANGLES;
        break;
    }
    glBeginTransformFeedback(glMode);
    m_isActive = true;
    m_isPaused = false;
    m_currentMode = mode;
    return OkStatus();
}

Status WGL2_TransformFeedback::end() {
    if (!m_isActive) {
        return FailedPreconditionError("Transform feedback 未激活");
    }
    if (m_isPaused) {
        return FailedPreconditionError("Transform feedback 处于暂停状态，必须 resume 后方可 end");
    }
    bind();
    glEndTransformFeedback();
    m_isActive = false;
    m_isPaused = false;
    return OkStatus();
}

Status WGL2_TransformFeedback::pause() {
    if (!m_isActive) {
        return FailedPreconditionError("Transform feedback 未激活，无法暂停");
    }
    if (m_isPaused) {
        return FailedPreconditionError("Transform feedback 已处于暂停状态");
    }
    bind();
    glPauseTransformFeedback();
    m_isPaused = true;
    return OkStatus();
}

Status WGL2_TransformFeedback::resume() {
    if (!m_isActive) {
        return FailedPreconditionError("Transform feedback 未激活，无法恢复");
    }
    if (!m_isPaused) {
        return FailedPreconditionError("Transform feedback 未处于暂停状态");
    }
    bind();
    glResumeTransformFeedback();
    m_isPaused = false;
    return OkStatus();
}

} // namespace FCT::RHI

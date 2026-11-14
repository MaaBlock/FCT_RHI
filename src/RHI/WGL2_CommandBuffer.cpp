#include "./WGL2_CommandBuffer.h"
#include "./WGL2_TraditionalPipeline.h"
#include "./Fence.h"
#include "../Context/WGL2_Context.h"
#include <cassert>

namespace FCT::RHI {

WGL2_CommandBuffer::WGL2_CommandBuffer(WGL2_Context* ctx)
    : m_ctx(ctx)
{
    // 预分配 64KB 字节流空间，保证录制过程零堆分配 (FEAT-27)
    m_commandStream.reserve(64 * 1024);
}

void WGL2_CommandBuffer::reset()
{
    m_commandStream.clear();
    m_isRecording = false;
    clearWaitSemaphores();
}

void WGL2_CommandBuffer::create()
{
    reset();
}

void WGL2_CommandBuffer::begin()
{
    reset();
    m_isRecording = true;
}

void WGL2_CommandBuffer::viewport(Vec2 lt, Vec2 rb)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_SetViewport cmd;
    cmd.x = lt.x;
    cmd.y = lt.y;
    cmd.width = rb.x - lt.x;
    cmd.height = rb.y - lt.y;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::scissor(Vec2 lt, Vec2 rb)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_SetScissor cmd;
    cmd.x = static_cast<int32_t>(lt.x);
    cmd.y = static_cast<int32_t>(lt.y);
    cmd.width = static_cast<uint32_t>(rb.x - lt.x);
    cmd.height = static_cast<uint32_t>(rb.y - lt.y);
    emitCommand(cmd);
}

void WGL2_CommandBuffer::draw(size_t vertexBegin, size_t instanceBegin, size_t vertexSize, size_t instanceSize)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_Draw cmd;
    cmd.vertexBegin = vertexBegin;
    cmd.instanceBegin = instanceBegin;
    cmd.vertexSize = vertexSize;
    cmd.instanceSize = instanceSize;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::drawIndex(size_t indexBegin, size_t instanceBegin, size_t indexCount, size_t instanceCount, size_t firstVertex)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_DrawIndex cmd;
    cmd.indexBegin = indexBegin;
    cmd.instanceBegin = instanceBegin;
    cmd.indexCount = indexCount;
    cmd.instanceCount = instanceCount;
    cmd.firstVertex = firstVertex;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::drawIndexedIndirect(void* indirectBuffer, size_t byteOffset, uint32_t drawCount, uint32_t stride)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_DrawIndexedIndirect cmd;
    cmd.indirectBuffer = indirectBuffer;
    cmd.byteOffset = byteOffset;
    cmd.drawCount = drawCount;
    cmd.stride = stride;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::bindPipeline(Pipeline* pipeline)
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_BindPipeline cmd;
    cmd.pipeline = pipeline;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::end()
{
    m_isRecording = false;
}

void WGL2_CommandBuffer::nextPass()
{
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_NextPass cmd;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::barrier(FCT::Image* image, ImageLayout oldLayout, ImageLayout newLayout,
                                 PipelineStages srcStage, PipelineStages dstStage,
                                 AccessFlags srcAccess, AccessFlags dstAccess,
                                 ImageAspects aspectMask)
{
    (void)srcStage; (void)dstStage; (void)srcAccess; (void)dstAccess; (void)aspectMask;
    assert(m_isRecording && "CommandBuffer must be recording");
    Cmd_Barrier cmd;
    cmd.image = image;
    cmd.oldLayout = oldLayout;
    cmd.newLayout = newLayout;
    emitCommand(cmd);
}

void WGL2_CommandBuffer::submit()
{
    const uint8_t* ptr = m_commandStream.data();
    const uint8_t* end = ptr + m_commandStream.size();

    while (ptr < end) {
        auto opcode = *reinterpret_cast<const WGL2_CmdOpcode*>(ptr);
        switch (opcode) {
        case WGL2_CmdOpcode::BindPipeline: {
            const auto* cmd = reinterpret_cast<const Cmd_BindPipeline*>(ptr);
            if (cmd->pipeline) {
                cmd->pipeline->bind(this);
            }
            ptr += sizeof(Cmd_BindPipeline);
            break;
        }
        case WGL2_CmdOpcode::SetViewport: {
            const auto* cmd = reinterpret_cast<const Cmd_SetViewport*>(ptr);
            glViewport(static_cast<GLint>(cmd->x), static_cast<GLint>(cmd->y),
                       static_cast<GLsizei>(cmd->width), static_cast<GLsizei>(cmd->height));
            ptr += sizeof(Cmd_SetViewport);
            break;
        }
        case WGL2_CmdOpcode::SetScissor: {
            const auto* cmd = reinterpret_cast<const Cmd_SetScissor*>(ptr);
            glScissor(cmd->x, cmd->y, static_cast<GLsizei>(cmd->width), static_cast<GLsizei>(cmd->height));
            ptr += sizeof(Cmd_SetScissor);
            break;
        }
        case WGL2_CmdOpcode::Draw: {
            const auto* cmd = reinterpret_cast<const Cmd_Draw*>(ptr);
            if (cmd->instanceSize > 1) {
                glDrawArraysInstanced(GL_TRIANGLES, static_cast<GLint>(cmd->vertexBegin),
                                      static_cast<GLsizei>(cmd->vertexSize), static_cast<GLsizei>(cmd->instanceSize));
            } else {
                glDrawArrays(GL_TRIANGLES, static_cast<GLint>(cmd->vertexBegin), static_cast<GLsizei>(cmd->vertexSize));
            }
            ptr += sizeof(Cmd_Draw);
            break;
        }
        case WGL2_CmdOpcode::DrawIndex: {
            const auto* cmd = reinterpret_cast<const Cmd_DrawIndex*>(ptr);
            const void* offset = reinterpret_cast<const void*>(cmd->indexBegin * sizeof(uint32_t));
            if (cmd->instanceCount > 1) {
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(cmd->indexCount),
                                        GL_UNSIGNED_INT, offset, static_cast<GLsizei>(cmd->instanceCount));
            } else {
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cmd->indexCount),
                               GL_UNSIGNED_INT, offset);
            }
            ptr += sizeof(Cmd_DrawIndex);
            break;
        }
        case WGL2_CmdOpcode::DrawIndexedIndirect: {
            // WebGL 2.0 基础规范不包含 indirect draw，降级或者使用模拟
            ptr += sizeof(Cmd_DrawIndexedIndirect);
            break;
        }
        case WGL2_CmdOpcode::Barrier: {
            ptr += sizeof(Cmd_Barrier);
            break;
        }
        case WGL2_CmdOpcode::NextPass: {
            ptr += sizeof(Cmd_NextPass);
            break;
        }
        default:
            assert(false && "Unknown WGL2 command opcode");
            return;
        }
    }

    if (m_fence != nullptr) {
        m_fence->create();
    }

    clearWaitSemaphores();
}

} // namespace FCT::RHI

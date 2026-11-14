#pragma once

#include "./CommandBuffer.h"
#include "../Context/WGL2_ContextCommon.h"
#include <vector>

namespace FCT {

class WGL2_Context;

namespace RHI {

class Pipeline;
class TraditionalPipeline;

enum class WGL2_CmdOpcode : uint8_t {
    BindPipeline,
    SetViewport,
    SetScissor,
    Draw,
    DrawIndex,
    DrawIndexedIndirect,
    Barrier,
    NextPass,
};

#pragma pack(push, 1)
struct Cmd_BindPipeline {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::BindPipeline};
    Pipeline* pipeline{nullptr};
};

struct Cmd_SetViewport {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::SetViewport};
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
};

struct Cmd_SetScissor {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::SetScissor};
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
};

struct Cmd_Draw {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::Draw};
    size_t vertexBegin{0};
    size_t instanceBegin{0};
    size_t vertexSize{0};
    size_t instanceSize{0};
};

struct Cmd_DrawIndex {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::DrawIndex};
    size_t indexBegin{0};
    size_t instanceBegin{0};
    size_t indexCount{0};
    size_t instanceCount{0};
    size_t firstVertex{0};
};

struct Cmd_DrawIndexedIndirect {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::DrawIndexedIndirect};
    void* indirectBuffer{nullptr};
    size_t byteOffset{0};
    uint32_t drawCount{1};
    uint32_t stride{sizeof(DrawIndexedIndirectCommand)};
};

struct Cmd_Barrier {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::Barrier};
    Image* image{nullptr};
    ImageLayout oldLayout{ImageLayout::undefined};
    ImageLayout newLayout{ImageLayout::undefined};
};

struct Cmd_NextPass {
    WGL2_CmdOpcode opcode{WGL2_CmdOpcode::NextPass};
};
#pragma pack(pop)

/**
 * @brief WebGL 2.0 / GLES 3.0 命令缓冲实现 (FEAT-27)
 * 基于紧凑线性 POD 字节流录制，录制期零堆内存分配，折叠批量提交
 */
class WGL2_CommandBuffer : public CommandBuffer {
public:
    explicit WGL2_CommandBuffer(WGL2_Context* ctx);
    ~WGL2_CommandBuffer() override = default;

    void reset() override;
    void create() override;
    void begin() override;
    void viewport(Vec2 lt, Vec2 rb) override;
    void scissor(Vec2 lt, Vec2 rb) override;
    void draw(size_t vertexBegin, size_t instanceBegin, size_t vertexSize, size_t instanceSize) override;
    void drawIndex(size_t indexBegin, size_t instanceBegin, size_t indexCount, size_t instanceCount, size_t firstVertex = 0) override;
    void drawIndexedIndirect(void* indirectBuffer, size_t byteOffset, uint32_t drawCount = 1, uint32_t stride = sizeof(DrawIndexedIndirectCommand)) override;
    void bindPipeline(Pipeline* pipeline) override;
    void end() override;
    void submit() override;
    void nextPass() override;
    void barrier(FCT::Image* image, ImageLayout oldLayout, ImageLayout newLayout,
                 PipelineStages srcStage, PipelineStages dstStage,
                 AccessFlags srcAccess, AccessFlags dstAccess,
                 ImageAspects aspectMask = ImageAspect::color) override;

    const std::vector<uint8_t>& getBytecodeStream() const noexcept { return m_commandStream; }

private:
    template <typename T>
    void emitCommand(const T& cmd) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&cmd);
        m_commandStream.insert(m_commandStream.end(), ptr, ptr + sizeof(T));
    }

    WGL2_Context* m_ctx{nullptr};
    std::vector<uint8_t> m_commandStream;
    bool m_isRecording{false};
};

} // namespace RHI
} // namespace FCT

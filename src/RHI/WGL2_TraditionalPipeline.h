#pragma once

#include "./RasterizationPipeline.h"
#include "../Context/WGL2_ContextCommon.h"
#include "./WGL2_VertexShader.h"
#include "./WGL2_PixelShader.h"
#include "./BlendState.h"
#include "./RasterizationState.h"
#include "./DepthStencilState.h"
#include "./ViewportState.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Pass;

/**
 * @brief WebGL 2.0 / GLES 3.0 传统光栅化管线 (FEAT-24)
 * 采用 64 位整型紧凑编码全部高频管线状态，驱动级 XOR 差分应用
 */
class WGL2_TraditionalPipeline : public RasterizationPipeline {
public:
    explicit WGL2_TraditionalPipeline(WGL2_Context* ctx);
    ~WGL2_TraditionalPipeline() override;

    PipelineType getType() const noexcept override { return PipelineType::Traditional; }

    void addResources(IPipelineResource* resource) override;
    Status create() override;
    void bindPass(Pass* pass) override;
    void bind(CommandBuffer* cmdBuf) override;

    uint64_t getNativeHandle() const noexcept override { return static_cast<uint64_t>(m_programHandle); }
    uint64_t getNativeLayoutHandle() const noexcept override { return 0; }

    uint64_t getStateKey() const noexcept { return m_stateKey; }
    void setStateKey(uint64_t key) noexcept { m_stateKey = key; }

    GLuint getProgramHandle() const noexcept { return m_programHandle; }

    void setShaders(VertexShader* vs, PixelShader* ps);
    void setBlendState(BlendState* bs);
    void setRasterizationState(RasterizationState* rs);
    void setDepthStencilState(DepthStencilState* ds);
    void setViewportState(ViewportState* vs);

    void encodeStateKey();

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_programHandle{0};
    uint64_t m_stateKey{0};

    VertexShader* m_vertexShader{nullptr};
    PixelShader* m_pixelShader{nullptr};
    BlendState* m_blendState{nullptr};
    RasterizationState* m_rasterState{nullptr};
    DepthStencilState* m_depthStencilState{nullptr};
    ViewportState* m_viewportState{nullptr};
};

} // namespace RHI
} // namespace FCT

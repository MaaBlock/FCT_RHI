#include "./WGL2_TraditionalPipeline.h"
#include "../Context/WGL2_Context.h"
#include "./WGL2_Pass.h"
#include "./CommandBuffer.h"
#include "./WGL2_BlendState.h"
#include "./WGL2_VertexShader.h"
#include "./WGL2_PixelShader.h"

namespace FCT::RHI {

WGL2_TraditionalPipeline::WGL2_TraditionalPipeline(WGL2_Context* ctx)
    : m_ctx(ctx)
{
    // 默认状态：深度测试开启、Cull Back 开启、深度写入开启
    m_stateKey = (1ULL << 0) | (1ULL << 5) | (1ULL << 6) | (3ULL << 7) | (0xFULL << 54);
}

WGL2_TraditionalPipeline::~WGL2_TraditionalPipeline()
{
    if (m_programHandle != 0) {
        glDeleteProgram(m_programHandle);
        m_programHandle = 0;
    }
}

void WGL2_TraditionalPipeline::setShaders(VertexShader* vs, PixelShader* ps)
{
    m_vertexShader = vs;
    m_pixelShader = ps;
}

void WGL2_TraditionalPipeline::setBlendState(BlendState* bs)
{
    m_blendState = bs;
    encodeStateKey();
}

void WGL2_TraditionalPipeline::setRasterizationState(RasterizationState* rs)
{
    m_rasterState = rs;
    encodeStateKey();
}

void WGL2_TraditionalPipeline::setDepthStencilState(DepthStencilState* ds)
{
    m_depthStencilState = ds;
    encodeStateKey();
}

void WGL2_TraditionalPipeline::setViewportState(ViewportState* vs)
{
    m_viewportState = vs;
}

void WGL2_TraditionalPipeline::encodeStateKey()
{
    uint64_t key = 0;

    // 1. 光栅化状态 (Bit 0~4)
    if (m_rasterState) {
        if (m_rasterState->cullMode() != CullMode::None) {
            key |= (1ULL << 0); // cullEnable
            if (m_rasterState->cullMode() == CullMode::Front) {
                key |= (1ULL << 1); // cullFace = GL_FRONT
            }
        }
        if (m_rasterState->frontFace() == FrontFace::Clockwise) {
            key |= (1ULL << 2); // frontFace = GL_CW
        }
        if (m_rasterState->depthBiasEnable()) {
            key |= (1ULL << 3);
        }
        if (m_rasterState->rasterizerDiscardEnable()) {
            key |= (1ULL << 4);
        }
    } else {
        key |= (1ULL << 0); // 默认开启 Cull Back
    }

    // 2. 深度测试状态 (Bit 5~9)
    if (m_depthStencilState) {
        if (m_depthStencilState->depthTestEnable()) {
            key |= (1ULL << 5);
        }
        if (m_depthStencilState->depthWriteEnable()) {
            key |= (1ULL << 6);
        }
        uint64_t funcVal = static_cast<uint64_t>(m_depthStencilState->depthCompareOp()) & 0x7ULL;
        key |= (funcVal << 7);

        // 3. 模板状态 (Bit 10~30)
        if (m_depthStencilState->stencilTestEnable()) {
            key |= (1ULL << 10);
            uint64_t stFunc = static_cast<uint64_t>(m_depthStencilState->front().compareOp) & 0x7ULL;
            key |= (stFunc << 11);
            uint64_t failOp = static_cast<uint64_t>(m_depthStencilState->front().failOp) & 0x7ULL;
            uint64_t zfailOp = static_cast<uint64_t>(m_depthStencilState->front().depthFailOp) & 0x7ULL;
            uint64_t zpassOp = static_cast<uint64_t>(m_depthStencilState->front().passOp) & 0x7ULL;
            key |= (failOp << 14);
            key |= (zfailOp << 17);
            key |= (zpassOp << 20);
            uint64_t writeMask = static_cast<uint64_t>(m_depthStencilState->front().writeMask) & 0xFFULL;
            key |= (writeMask << 23);
        }
    } else {
        key |= (1ULL << 5) | (1ULL << 6) | (3ULL << 7); // 默认开启深度测试 LessEqual
    }

    // 4. 混合状态 (Bit 31~58)
    if (m_blendState) {
        auto* wglBlend = dynamic_cast<WGL2_BlendState*>(m_blendState);
        if (wglBlend && !wglBlend->getTargets().empty()) {
            const auto& tgt = wglBlend->getTarget(0);
            if (tgt.enable) {
                key |= (1ULL << 31);
                uint64_t colOp = static_cast<uint64_t>(tgt.colorOp) & 0x7ULL;
                uint64_t alpOp = static_cast<uint64_t>(tgt.alphaOp) & 0x7ULL;
                key |= (colOp << 32);
                key |= (alpOp << 35);
                uint64_t sc = static_cast<uint64_t>(tgt.srcColor) & 0xFULL;
                uint64_t dc = static_cast<uint64_t>(tgt.dstColor) & 0xFULL;
                uint64_t sa = static_cast<uint64_t>(tgt.srcAlpha) & 0xFULL;
                uint64_t da = static_cast<uint64_t>(tgt.dstAlpha) & 0xFULL;
                key |= (sc << 38);
                key |= (dc << 42);
                key |= (sa << 46);
                key |= (da << 50);
            }
            uint64_t colMask = static_cast<uint64_t>(tgt.mask) & 0xFULL;
            key |= (colMask << 54);
        } else if (m_blendState->blendEnable()) {
            key |= (1ULL << 31);
            key |= (0xFULL << 54);
        }
    } else {
        key |= (0xFULL << 54); // 默认写入 RGBA
    }

    m_stateKey = key;
}

void WGL2_TraditionalPipeline::addResources(IPipelineResource* resource)
{
    if (!resource) return;
    if (auto* vs = dynamic_cast<VertexShader*>(resource)) {
        m_vertexShader = vs;
    } else if (auto* ps = dynamic_cast<PixelShader*>(resource)) {
        m_pixelShader = ps;
    } else if (auto* bs = dynamic_cast<BlendState*>(resource)) {
        setBlendState(bs);
    } else if (auto* rs = dynamic_cast<RasterizationState*>(resource)) {
        setRasterizationState(rs);
    } else if (auto* ds = dynamic_cast<DepthStencilState*>(resource)) {
        setDepthStencilState(ds);
    } else if (auto* vs_state = dynamic_cast<ViewportState*>(resource)) {
        setViewportState(vs_state);
    }
}

Status WGL2_TraditionalPipeline::create()
{
    encodeStateKey();

    if (m_programHandle != 0) {
        glDeleteProgram(m_programHandle);
        m_programHandle = 0;
    }

    m_programHandle = glCreateProgram();
    if (m_programHandle == 0) {
        return InternalError("WGL2_TraditionalPipeline: glCreateProgram 分配失败。");
    }

    if (m_vertexShader) {
        auto* wglVs = dynamic_cast<WGL2_VertexShader*>(m_vertexShader);
        if (wglVs && wglVs->getShaderId() != 0) {
            glAttachShader(m_programHandle, wglVs->getShaderId());
        }
    }

    if (m_pixelShader) {
        auto* wglPs = dynamic_cast<WGL2_PixelShader*>(m_pixelShader);
        if (wglPs && wglPs->getShaderId() != 0) {
            glAttachShader(m_programHandle, wglPs->getShaderId());
        }
    }

    glLinkProgram(m_programHandle);

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(m_programHandle, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(m_programHandle, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog;
        if (logLen > 0) {
            infoLog.resize(logLen);
            glGetProgramInfoLog(m_programHandle, logLen, nullptr, infoLog.data());
        }
        return InternalError("WGL2_TraditionalPipeline 链接着色器程序失败:\n" + infoLog);
    }

    return OkStatus();
}

void WGL2_TraditionalPipeline::bindPass(Pass* pass)
{
    (void)pass;
}

void WGL2_TraditionalPipeline::bind(CommandBuffer* cmdBuf)
{
    (void)cmdBuf;
    if (m_ctx) {
        m_ctx->applyGraphicsState(m_stateKey);
    }
    if (m_programHandle != 0) {
        glUseProgram(m_programHandle);
    }
}

} // namespace FCT::RHI

#include "./WGL2_Context.h"
#include "./WGL2_Device.h"
#include "../RHI/WGL2_TraditionalPipeline.h"
#include "../RHI/WGL2_StorageBuffer.h"
#include "../RHI/WGL2_ConstBuffer.h"
#include "../RHI/WGL2_Sampler.h"

namespace FCT {

// OpenGL 比较操作映射表 (0~7)
static constexpr GLenum s_glCompareOpTable[8] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS
};

// OpenGL 模板操作映射表 (0~7)
static constexpr GLenum s_glStencilOpTable[8] = {
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR,
    GL_DECR,
    GL_INVERT,
    GL_INCR_WRAP,
    GL_DECR_WRAP
};

// OpenGL 混合方程映射表 (0~4)
static constexpr GLenum s_glBlendOpTable[5] = {
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX
};

// OpenGL 混合因子映射表 (0~15)
static constexpr GLenum s_glBlendFactorTable[16] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA_SATURATE,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
    GL_ONE
};

// =============================================================================
// WGL2_FrameRingUBO
// =============================================================================

WGL2_FrameRingUBO::~WGL2_FrameRingUBO()
{
    destroy();
}

Status WGL2_FrameRingUBO::init(uint32_t capacityBytes, uint32_t alignment)
{
    destroy();
    m_capacity = capacityBytes;
    m_alignment = (alignment > 0) ? alignment : 256;
    m_currentOffset = 0;
    m_highWaterMark = 0;
    m_stagingMemory.resize(m_capacity, 0);

    if (glGenBuffers) {
        glGenBuffers(1, &m_uboHandle);
        if (m_uboHandle != 0) {
            glBindBuffer(GL_UNIFORM_BUFFER, m_uboHandle);
            glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(m_capacity), nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    }
    return OkStatus();
}

void WGL2_FrameRingUBO::destroy()
{
    if (m_uboHandle != 0 && glDeleteBuffers) {
        glDeleteBuffers(1, &m_uboHandle);
        m_uboHandle = 0;
    }
    m_currentOffset = 0;
    m_highWaterMark = 0;
    m_stagingMemory.clear();
}

WGL2_FrameRingUBO::Allocation WGL2_FrameRingUBO::allocate(uint32_t sizeBytes)
{
    assert(sizeBytes <= m_capacity && "Allocation size exceeds FrameRingUBO capacity");
    uint32_t alignedOffset = (m_currentOffset + m_alignment - 1) & ~(m_alignment - 1);
    if (alignedOffset + sizeBytes > m_capacity) {
        // 容量超限回退
        alignedOffset = 0;
    }
    m_currentOffset = alignedOffset + sizeBytes;
    m_highWaterMark = std::max(m_highWaterMark, m_currentOffset);

    Allocation alloc;
    alloc.offset = alignedOffset;
    alloc.size = sizeBytes;
    alloc.cpuPtr = m_stagingMemory.data() + alignedOffset;
    return alloc;
}

void WGL2_FrameRingUBO::reset()
{
    m_currentOffset = 0;
    m_highWaterMark = 0;
}

void WGL2_FrameRingUBO::flushToGpu()
{
    if (m_highWaterMark > 0 && m_uboHandle != 0 && glBindBuffer && glBufferSubData) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_uboHandle);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(m_highWaterMark), m_stagingMemory.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

// =============================================================================
// WGL2_Context
// =============================================================================

WGL2_Context::WGL2_Context(Runtime* runtime)
    : Context(runtime)
{
    initWGL2Functions();
}

WGL2_Context::~WGL2_Context()
{
    for (auto& [key, vao] : m_vaoCache) {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
    }
    m_vaoCache.clear();
    m_ringUBO.destroy();
}

void WGL2_Context::createPlatform(ContextCreateFlags flag)
{
    initWGL2Functions();
    m_ringUBO.init(4 * 1024 * 1024, getUniformBufferOffsetAlignment());
    m_resourceDevice = FCT_NEW(WGL2_Device, this);
}

void WGL2_Context::waitGpu()
{
    // WebGL 2.0 / GLES 3.0 无显式 CPU-GPU 阻塞队列，调用 glFinish 保证管道排空
}

uint32_t WGL2_Context::getUniformBufferOffsetAlignment() const noexcept
{
    GLint align = 256;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
    return align > 0 ? static_cast<uint32_t>(align) : 256;
}

uint32_t WGL2_Context::getMaxUniformBufferRange() const noexcept
{
    GLint maxRange = 65536;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxRange);
    return maxRange > 0 ? static_cast<uint32_t>(maxRange) : 65536;
}

RHI::RasterizationPipeline* WGL2_Context::createTraditionPipeline()
{
    return FCT_NEW(RHI::WGL2_TraditionalPipeline, this);
}

UniquePtr<RHI::Pipeline> WGL2_Context::createRhiPipeline(PipelineType type)
{
    return makeUnique<RHI::WGL2_TraditionalPipeline>(this);
}

RHI::StorageBuffer* WGL2_Context::createStorageBuffer(size_t size)
{
    return FCT_NEW(RHI::WGL2_StorageBuffer, this, size);
}

RHI::ConstBuffer* WGL2_Context::getEmptyConstBuffer(const ConstLayout& layout)
{
    auto* cb = FCT_NEW(RHI::WGL2_ConstBuffer, this);
    cb->create(layout);
    return cb;
}

Sampler* WGL2_Context::getEmptySampler()
{
    auto* s = FCT_NEW(RHI::WGL2_Sampler, this);
    s->create();
    return s;
}

void WGL2_Context::applyGraphicsState(uint64_t targetKey)
{
    uint64_t diff = m_currentStateKey ^ targetKey;
    if (diff == 0) [[likely]] {
        return;
    }

    // 1. 光栅化状态差分
    if (diff & (1ULL << 0)) {
        if (targetKey & (1ULL << 0)) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
    }
    if (diff & (1ULL << 1)) {
        glCullFace((targetKey & (1ULL << 1)) ? GL_FRONT : GL_BACK);
    }
    if (diff & (1ULL << 2)) {
        glFrontFace((targetKey & (1ULL << 2)) ? GL_CW : GL_CCW);
    }
    if (diff & (1ULL << 3)) {
        if (targetKey & (1ULL << 3)) glEnable(GL_POLYGON_OFFSET_FILL);
        else glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (diff & (1ULL << 4)) {
        if (targetKey & (1ULL << 4)) glEnable(GL_RASTERIZER_DISCARD);
        else glDisable(GL_RASTERIZER_DISCARD);
    }

    // 2. 深度测试差分
    if (diff & (1ULL << 5)) {
        if (targetKey & (1ULL << 5)) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }
    if (diff & (1ULL << 6)) {
        glDepthMask((targetKey & (1ULL << 6)) ? GL_TRUE : GL_FALSE);
    }
    if (diff & (0x7ULL << 7)) {
        uint32_t funcIdx = (targetKey >> 7) & 0x7;
        glDepthFunc(s_glCompareOpTable[funcIdx]);
    }

    // 3. 模板测试差分
    if (diff & (1ULL << 10)) {
        if (targetKey & (1ULL << 10)) glEnable(GL_STENCIL_TEST);
        else glDisable(GL_STENCIL_TEST);
    }
    if (diff & (0x7ULL << 11)) {
        uint32_t funcIdx = (targetKey >> 11) & 0x7;
        glStencilFunc(s_glCompareOpTable[funcIdx], m_currentStencilRef, m_currentStencilCompareMask);
    }
    if (diff & (0x1FFULL << 14)) {
        uint32_t failIdx = (targetKey >> 14) & 0x7;
        uint32_t zfailIdx = (targetKey >> 17) & 0x7;
        uint32_t zpassIdx = (targetKey >> 20) & 0x7;
        glStencilOp(s_glStencilOpTable[failIdx], s_glStencilOpTable[zfailIdx], s_glStencilOpTable[zpassIdx]);
    }
    if (diff & (0xFFULL << 23)) {
        uint32_t mask = (targetKey >> 23) & 0xFF;
        glStencilMask(mask);
    }

    // 4. 混合状态差分
    if (diff & (1ULL << 31)) {
        if (targetKey & (1ULL << 31)) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
    }
    if (diff & (0x3FULL << 32)) {
        uint32_t colOp = (targetKey >> 32) & 0x7;
        uint32_t alpOp = (targetKey >> 35) & 0x7;
        if (colOp < 5 && alpOp < 5) {
            glBlendEquationSeparate(s_glBlendOpTable[colOp], s_glBlendOpTable[alpOp]);
        }
    }
    if (diff & (0xFFFFULL << 38)) {
        uint32_t sc = (targetKey >> 38) & 0xF;
        uint32_t dc = (targetKey >> 42) & 0xF;
        uint32_t sa = (targetKey >> 46) & 0xF;
        uint32_t da = (targetKey >> 50) & 0xF;
        glBlendFuncSeparate(s_glBlendFactorTable[sc], s_glBlendFactorTable[dc],
                            s_glBlendFactorTable[sa], s_glBlendFactorTable[da]);
    }
    if (diff & (0xFULL << 54)) {
        uint32_t mask = (targetKey >> 54) & 0xF;
        glColorMask((mask & 1) ? GL_TRUE : GL_FALSE,
                    (mask & 2) ? GL_TRUE : GL_FALSE,
                    (mask & 4) ? GL_TRUE : GL_FALSE,
                    (mask & 8) ? GL_TRUE : GL_FALSE);
    }
    if (diff & (1ULL << 58)) {
        if (targetKey & (1ULL << 58)) glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        else glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    m_currentStateKey = targetKey;
}

GLuint WGL2_Context::getOrCreateVAO(const VAOCacheKey& key, const std::function<void()>& setupFunc)
{
    auto it = m_vaoCache.find(key);
    if (it != m_vaoCache.end()) {
        return it->second;
    }

    GLuint newVAO = 0;
    glGenVertexArrays(1, &newVAO);
    glBindVertexArray(newVAO);
    if (setupFunc) {
        setupFunc();
    }
    m_vaoCache[key] = newVAO;
    return newVAO;
}

void WGL2_Context::bindTransformFeedback(GLuint handle)
{
    if (m_currentBoundXFB != handle) {
        m_currentBoundXFB = handle;
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, handle);
    }
}

} // namespace FCT

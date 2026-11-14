#pragma once

#include "./Context.h"
#include "./WGL2_ContextCommon.h"
#include <unordered_map>
#include <vector>
#include <array>
#include <boost/functional/hash.hpp>

namespace FCT {

class WGL2_Device;

namespace RHI {
    class WGL2_TraditionalPipeline;
    class WGL2_StorageBuffer;
}

/**
 * @brief VAO 缓存键值 (InputLayout + VBOs + IBO)
 */
struct VAOCacheKey {
    size_t inputLayoutHash{0};
    std::array<GLuint, 4> vbos{0, 0, 0, 0};
    GLuint ibo{0};

    bool operator==(const VAOCacheKey& other) const noexcept {
        return inputLayoutHash == other.inputLayoutHash &&
               vbos == other.vbos &&
               ibo == other.ibo;
    }
};

struct VAOCacheHasher {
    size_t operator()(const VAOCacheKey& k) const noexcept {
        size_t seed = k.inputLayoutHash;
        for (auto vbo : k.vbos) {
            boost::hash_combine(seed, static_cast<size_t>(vbo));
        }
        boost::hash_combine(seed, static_cast<size_t>(k.ibo));
        return seed;
    }
};

/**
 * @brief 帧环形大 UBO (FEAT-26)
 */
class WGL2_FrameRingUBO {
public:
    WGL2_FrameRingUBO() = default;
    ~WGL2_FrameRingUBO();

    Status init(uint32_t capacityBytes = 4 * 1024 * 1024, uint32_t alignment = 256);
    void destroy();

    struct Allocation {
        uint32_t offset{0};
        uint32_t size{0};
        uint8_t* cpuPtr{nullptr};
    };

    Allocation allocate(uint32_t sizeBytes);
    void reset();
    void flushToGpu();

    GLuint getBufferHandle() const noexcept { return m_uboHandle; }
    uint32_t getAlignment() const noexcept { return m_alignment; }
    uint32_t getUsedBytes() const noexcept { return m_currentOffset; }
    uint32_t getHighWaterMark() const noexcept { return m_highWaterMark; }

private:
    GLuint m_uboHandle{0};
    uint32_t m_capacity{0};
    uint32_t m_alignment{256};
    uint32_t m_currentOffset{0};
    uint32_t m_highWaterMark{0};
    std::vector<uint8_t> m_stagingMemory;
};

/**
 * @brief WebGL 2.0 / OpenGL ES 3.0 上下文实现 (FEAT-23, FEAT-24, FEAT-26)
 */
class WGL2_Context : public Context {
public:
    explicit WGL2_Context(Runtime* runtime);
    ~WGL2_Context() override;

    void createPlatform(ContextCreateFlags flag) override;
    void waitGpu() override;

    // 硬件能力正交自省契约 (FEAT-23)
    bool isIntegrateGpu() override { return true; }
    bool isRayTracingSupported() const noexcept override { return false; }
    bool isMeshShaderSupported() const noexcept override { return false; }
    bool isComputeSupported() const noexcept override { return false; }
    bool isBindlessSupported() const noexcept override { return false; }
    bool isReadWriteStorageBufferSupported() const noexcept override { return false; }
    bool isIndirectDrawSupported() const noexcept override { return false; }
    bool isBaseInstanceSupported() const noexcept override { return false; }
    uint32_t getUniformBufferOffsetAlignment() const noexcept override;
    uint32_t getMaxUniformBufferRange() const noexcept override;

    // 管线与资源工厂接口
    RHI::RasterizationPipeline* createTraditionPipeline() override;
    UniquePtr<RHI::Pipeline> createRhiPipeline(PipelineType type) override;
    RHI::StorageBuffer* createStorageBuffer(size_t size = 0) override;
    RHI::ConstBuffer* getEmptyConstBuffer(const ConstLayout& layout) override;
    Sampler* getEmptySampler() override;

    // 64-bit 状态差分应用 (FEAT-24)
    void applyGraphicsState(uint64_t targetKey);
    uint64_t getCurrentStateKey() const noexcept { return m_currentStateKey; }

    // VAO 缓存查找与创建
    GLuint getOrCreateVAO(const VAOCacheKey& key, const std::function<void()>& setupFunc);

    // 帧环形 UBO
    WGL2_FrameRingUBO& getFrameRingUBO() noexcept { return m_ringUBO; }

    // TransformFeedback 状态跟踪与绑定 (FEAT-39)
    void bindTransformFeedback(GLuint handle);
    GLuint getCurrentBoundTransformFeedback() const noexcept { return m_currentBoundXFB; }

private:
    uint64_t m_currentStateKey{0};
    uint32_t m_currentStencilRef{0};
    uint32_t m_currentStencilCompareMask{0xFF};
    GLuint m_currentBoundXFB{0};

    WGL2_FrameRingUBO m_ringUBO;
    std::unordered_map<VAOCacheKey, GLuint, VAOCacheHasher> m_vaoCache;
};

} // namespace FCT

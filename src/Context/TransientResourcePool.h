#pragma once

#include "./Format.h"
#include "./Image.h"
#include "./Device.h"
#include "../RHI/ImageUsageFlags.h"
#include <vector>
#include <cstdint>

namespace FCT {

/**
 * @brief 瞬态物理资源规格描述符 (FEAT-32, FEAT-33)
 */
struct TransientResourceSpec {
    uint32_t width{0};
    uint32_t height{0};
    Format format{Format::UNDEFINED};
    Samples samples{Samples::sample_1};
    uint32_t mipLevels{1};
    uint32_t arrayLayers{1};
    bool isSampled{true};                  // 是否需要被采样；若为 false 则触发 FEAT-33 降级
    bool isDepthStencil{false};            // 是否为深度模板目标
    bool allowRenderbufferDowngrade{true};  // 是否允许降级为 WebGLRenderbuffer
    ImageUsages usage{0};

    bool matches(const TransientResourceSpec& other) const noexcept {
        return width == other.width &&
               height == other.height &&
               format == other.format &&
               samples == other.samples &&
               mipLevels == other.mipLevels &&
               arrayLayers == other.arrayLayers &&
               isSampled == other.isSampled &&
               isDepthStencil == other.isDepthStencil &&
               allowRenderbufferDowngrade == other.allowRenderbufferDowngrade;
    }
};

/**
 * @brief 池化托管的物理资源项
 */
struct PooledPhysicalResource {
    Image* physicalImage{nullptr};
    TransientResourceSpec spec;
    uint32_t expiredPassIdx{0};       // 当前帧内占用该资源的最后 Pass 索引
    uint32_t lastUsedFrameIndex{0};   // 跨帧 LRU/TTL 跟踪
    bool isRenderbufferOnly{false};   // 标识底层为 WebGLRenderbuffer
};

/**
 * @brief RenderGraph 瞬态物理资源池 (FEAT-32, FEAT-33)
 * 采用生命周期区间贪心复用算法，达成 >= 60% 复用率，120 帧闲置 TTL 淘汰
 */
class TransientResourcePool {
public:
    explicit TransientResourcePool(Device* device) noexcept
        : m_device(device) {}

    ~TransientResourcePool() {
        clear();
    }

    TransientResourcePool(const TransientResourcePool&) = delete;
    TransientResourcePool& operator=(const TransientResourcePool&) = delete;

    Image* acquire(const TransientResourceSpec& spec,
                   uint32_t currentPassIdx,
                   uint32_t lastPassIdx,
                   uint32_t frameIndex);

    void tick(uint32_t currentFrameIndex);
    void evictUnused();
    void clear();

    struct PoolStats {
        uint32_t totalVirtualRequests{0};
        uint32_t physicalAllocations{0};
        uint32_t reuseHits{0};

        float getReuseRate() const noexcept {
            return totalVirtualRequests > 0 ? static_cast<float>(reuseHits) / static_cast<float>(totalVirtualRequests) : 0.0f;
        }
    };

    PoolStats getStats() const noexcept { return m_stats; }
    void resetStats() noexcept { m_stats = PoolStats{}; }
    size_t getPoolSize() const noexcept { return m_pool.size(); }

private:
    Device* m_device{nullptr};
    std::vector<PooledPhysicalResource> m_pool;
    PoolStats m_stats;

    static constexpr uint32_t kEvictionTTL = 120; // 连续 120 帧未复用彻底销毁
};

} // namespace FCT

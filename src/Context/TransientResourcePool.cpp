#include "./TransientResourcePool.h"
#include "./SingleBufferImage.h"
#include "../RHI/WGL2_Image.h"

namespace FCT {

Image* TransientResourcePool::acquire(const TransientResourceSpec& spec,
                                     uint32_t currentPassIdx,
                                     uint32_t lastPassIdx,
                                     uint32_t frameIndex)
{
    m_stats.totalVirtualRequests++;

    // 1. 线性遍历查找已过期且规格完全一致的空闲物理缓冲（含跨帧已完结复用与帧内后继 Pass 复用）
    for (auto& entry : m_pool) {
        if ((entry.lastUsedFrameIndex < frameIndex || entry.expiredPassIdx < currentPassIdx) && entry.spec.matches(spec)) {
            // 命中复用
            entry.expiredPassIdx = lastPassIdx;
            entry.lastUsedFrameIndex = frameIndex;
            m_stats.reuseHits++;
            return entry.physicalImage;
        }
    }

    // 2. 未命中空闲缓冲，分配新的物理显存
    PooledPhysicalResource newEntry;
    newEntry.spec = spec;
    newEntry.expiredPassIdx = lastPassIdx;
    newEntry.lastUsedFrameIndex = frameIndex;
    newEntry.isRenderbufferOnly = (!spec.isSampled && spec.allowRenderbufferDowngrade);

    SingleBufferImage* img = m_device->newSingleBufferImage();
    img->format(spec.format);
    img->samples(spec.samples);
    img->width(static_cast<int>(spec.width));
    img->height(static_cast<int>(spec.height));
    img->arrayLayers(static_cast<int>(spec.arrayLayers));
    img->as(spec.usage);

    // 如果满足纯拓扑 Renderbuffer 降级条件，标记底层 RHI 对象 (FEAT-33)
    if (newEntry.isRenderbufferOnly) {
        img->setIsRenderbuffer(true);
    }

    img->create();

    // 防御性后置保障：若底层 RHI 镜像已就绪，保持标志严格一致
    if (newEntry.isRenderbufferOnly && img->currentImage()) {
        img->currentImage()->setRenderbuffer(true);
    }

    newEntry.physicalImage = img;
    Image* rawPtr = newEntry.physicalImage;
    m_pool.push_back(newEntry);
    m_stats.physicalAllocations++;

    return rawPtr;
}

void TransientResourcePool::tick(uint32_t currentFrameIndex)
{
    for (auto it = m_pool.begin(); it != m_pool.end(); ) {
        if (currentFrameIndex > it->lastUsedFrameIndex &&
            (currentFrameIndex - it->lastUsedFrameIndex) > kEvictionTTL) {
            if (it->physicalImage) {
                it->physicalImage->release();
                it->physicalImage = nullptr;
            }
            it = m_pool.erase(it);
        } else {
            ++it;
        }
    }
}

void TransientResourcePool::evictUnused()
{
    clear();
}

void TransientResourcePool::clear()
{
    for (auto& entry : m_pool) {
        if (entry.physicalImage) {
            entry.physicalImage->release();
            entry.physicalImage = nullptr;
        }
    }
    m_pool.clear();
}

} // namespace FCT

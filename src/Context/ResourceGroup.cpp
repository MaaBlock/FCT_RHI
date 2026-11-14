#include "ResourceGroup.h"
#include "Context.h"
#include "SingleBufferImage.h"

namespace FCT
{
    ResourceGroup::ResourceGroup(std::string name)
        : m_name(std::move(name))
    {
    }

    ResourceGroup::~ResourceGroup()
    {
        clear(nullptr);
    }

    Image* ResourceGroup::require(Context* ctx, const ResourceRequireDesc& desc)
    {
        // Note: 遵循零互斥锁规范（Zero mutexes）

        // 1. 若外部提供了现成图像，直接注册并返回
        if (desc.externalImage) {
            m_activeImages[desc.resourceId] = desc.externalImage;
            m_descs[desc.resourceId] = desc;
            if (desc.subRect.width > 0 && desc.subRect.height > 0) {
                m_subRects[desc.resourceId] = desc.subRect;
            }
            return desc.externalImage;
        }

        // 2. 若已有该 resourceId 的活跃图像且参数一致，直接复用
        auto activeIt = m_activeImages.find(desc.resourceId);
        if (activeIt != m_activeImages.end() && activeIt->second) {
            Image* cur = activeIt->second;
            if (static_cast<uint32_t>(cur->width()) == desc.width &&
                static_cast<uint32_t>(cur->height()) == desc.height &&
                cur->format() == desc.format) {
                if (desc.subRect.width > 0 && desc.subRect.height > 0) {
                    m_subRects[desc.resourceId] = desc.subRect;
                }
                return cur;
            }
        }

        // 3. 从空闲池中查找规格匹配的可用图像
        for (auto it = m_idlePool.begin(); it != m_idlePool.end(); ++it) {
            Image* candidate = *it;
            if (candidate &&
                static_cast<uint32_t>(candidate->width()) == desc.width &&
                static_cast<uint32_t>(candidate->height()) == desc.height &&
                candidate->format() == desc.format) {
                m_idlePool.erase(it);
                m_activeImages[desc.resourceId] = candidate;
                m_descs[desc.resourceId] = desc;
                if (desc.subRect.width > 0 && desc.subRect.height > 0) {
                    m_subRects[desc.resourceId] = desc.subRect;
                }
                return candidate;
            }
        }

        // 4. 空闲池无合适图像，通过 Context 创建新图像
        if (!ctx) {
            return nullptr;
        }

        auto* newImage = new SingleBufferImage(ctx);
        newImage->width(desc.width);
        newImage->height(desc.height);
        newImage->format(desc.format);
        newImage->as(desc.usage);
        newImage->arrayLayers(desc.arrayLayers);
        newImage->create();

        m_activeImages[desc.resourceId] = newImage;
        m_descs[desc.resourceId] = desc;
        if (desc.subRect.width > 0 && desc.subRect.height > 0) {
            m_subRects[desc.resourceId] = desc.subRect;
        }
        return newImage;
    }

    Buffer* ResourceGroup::requireBuffer(Context* ctx, const BufferRequireDesc& desc)
    {
        // 1. 若外部提供了现成 Buffer，直接登记并返回
        if (desc.externalBuffer) {
            m_activeBuffers[desc.resourceId] = desc.externalBuffer;
            m_bufferDescs[desc.resourceId] = desc;
            return desc.externalBuffer;
        }

        // 2. 若已有该 resourceId 的活跃 Buffer 且容量足够，直接复用
        auto activeIt = m_activeBuffers.find(desc.resourceId);
        if (activeIt != m_activeBuffers.end() && activeIt->second) {
            Buffer* cur = activeIt->second;
            if (cur->size() >= desc.size) {
                return cur;
            }
        }

        // 3. 从空闲池中查找容量足够的可用 Buffer
        for (auto it = m_idleBufferPool.begin(); it != m_idleBufferPool.end(); ++it) {
            Buffer* candidate = *it;
            if (candidate && candidate->size() >= desc.size) {
                m_idleBufferPool.erase(it);
                m_activeBuffers[desc.resourceId] = candidate;
                m_bufferDescs[desc.resourceId] = desc;
                return candidate;
            }
        }

        // 4. 创建新 Buffer
        auto* newBuf = new Buffer(ctx);
        newBuf->setSize(desc.size);
        m_activeBuffers[desc.resourceId] = newBuf;
        m_bufferDescs[desc.resourceId] = desc;
        return newBuf;
    }

    void ResourceGroup::release(const std::string& resourceId, Context* /*ctx*/)
    {
        auto it = m_activeImages.find(resourceId);
        if (it == m_activeImages.end()) {
            return;
        }

        Image* img = it->second;
        auto descIt = m_descs.find(resourceId);
        const bool isExternal = (descIt != m_descs.end() && descIt->second.externalImage != nullptr);

        m_activeImages.erase(it);
        if (descIt != m_descs.end()) {
            m_descs.erase(descIt);
        }
        m_subRects.erase(resourceId);

        if (img && !isExternal) {
            m_idlePool.push_back(img);
        }
    }

    void ResourceGroup::releaseBuffer(const std::string& resourceId, Context* /*ctx*/)
    {
        auto it = m_activeBuffers.find(resourceId);
        if (it == m_activeBuffers.end()) {
            return;
        }

        Buffer* buf = it->second;
        auto descIt = m_bufferDescs.find(resourceId);
        const bool isExternal = (descIt != m_bufferDescs.end() && descIt->second.externalBuffer != nullptr);

        m_activeBuffers.erase(it);
        if (descIt != m_bufferDescs.end()) {
            m_bufferDescs.erase(descIt);
        }

        if (buf && !isExternal) {
            m_idleBufferPool.push_back(buf);
        }
    }

    std::vector<Image*> ResourceGroup::getActiveImages() const
    {
        std::vector<Image*> result;
        result.reserve(m_activeImages.size());
        for (const auto& [id, img] : m_activeImages) {
            if (img) {
                result.push_back(img);
            }
        }
        return result;
    }

    std::vector<Buffer*> ResourceGroup::getActiveBuffers() const
    {
        std::vector<Buffer*> result;
        result.reserve(m_activeBuffers.size());
        for (const auto& [id, buf] : m_activeBuffers) {
            if (buf) {
                result.push_back(buf);
            }
        }
        return result;
    }

    Image* ResourceGroup::getImage(const std::string& resourceId) const
    {
        auto it = m_activeImages.find(resourceId);
        return (it != m_activeImages.end()) ? it->second : nullptr;
    }

    Buffer* ResourceGroup::getBuffer(const std::string& resourceId) const
    {
        auto it = m_activeBuffers.find(resourceId);
        return (it != m_activeBuffers.end()) ? it->second : nullptr;
    }

    Rect2D ResourceGroup::getSubRect(const std::string& resourceId) const
    {
        auto it = m_subRects.find(resourceId);
        return (it != m_subRects.end()) ? it->second : Rect2D{};
    }

    void ResourceGroup::setSubRect(const std::string& resourceId, const Rect2D& rect)
    {
        m_subRects[resourceId] = rect;
    }

    bool ResourceGroup::hasSubRect(const std::string& resourceId) const
    {
        return m_subRects.find(resourceId) != m_subRects.end();
    }

    void ResourceGroup::clear(Context* /*ctx*/)
    {
        for (auto& [id, img] : m_activeImages) {
            auto descIt = m_descs.find(id);
            const bool isExternal = (descIt != m_descs.end() && descIt->second.externalImage != nullptr);
            if (img && !isExternal) {
                img->release();
            }
        }
        m_activeImages.clear();
        m_descs.clear();
        m_subRects.clear();

        for (auto* img : m_idlePool) {
            if (img) {
                img->release();
            }
        }
        m_idlePool.clear();

        for (auto& [id, buf] : m_activeBuffers) {
            auto descIt = m_bufferDescs.find(id);
            const bool isExternal = (descIt != m_bufferDescs.end() && descIt->second.externalBuffer != nullptr);
            if (buf && !isExternal) {
                buf->release();
            }
        }
        m_activeBuffers.clear();
        m_bufferDescs.clear();

        for (auto* buf : m_idleBufferPool) {
            if (buf) {
                buf->release();
            }
        }
        m_idleBufferPool.clear();

        if (m_atlasImage) {
            m_atlasImage->release();
            m_atlasImage = nullptr;
        }
    }
} // namespace FCT

#ifndef FCT_RESOURCEGROUP_H
#define FCT_RESOURCEGROUP_H

#include "../ThirdParty.h"
#include "./Format.h"
#include "../RHI/ImageUsageFlags.h"
#include "../MutilThreadBase/RefCount.h"
#include "../MutilThreadBase/UniquePtr.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace FCT
{
    class Context;
    class Image;

    // Note: ShadowAtlas 子视口矩形分配结构体
    struct Rect2D
    {
        uint32_t viewportX = 0;
        uint32_t viewportY = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    /**
     * @brief GPU 缓冲资源基类
     * Note: 遵循 RefCount 引用计数规范，支持原生 GPU 缓冲句柄与尺寸管理
     */
    class Buffer : public RefCount
    {
    public:
        Buffer(Context* ctx = nullptr) : m_ctx(ctx), m_size(0), m_nativeHandle(nullptr) {}
        virtual ~Buffer() = default;

        size_t size() const noexcept { return m_size; }
        void setSize(size_t sz) noexcept { m_size = sz; }
        void* nativeHandle() const noexcept { return m_nativeHandle; }
        void* nativeBuffer() const noexcept { return m_nativeHandle; }
        void setNativeHandle(void* handle) noexcept { m_nativeHandle = handle; }
        Context* getContext() const noexcept { return m_ctx; }

    protected:
        Context* m_ctx = nullptr;
        size_t m_size = 0;
        void* m_nativeHandle = nullptr;
    };

    struct ResourceRequireDesc
    {
        std::string resourceId;
        Format format = Format::D32_SFLOAT;
        uint32_t width = 2048;
        uint32_t height = 2048;
        uint32_t arrayLayers = 1;
        ImageUsages usage = ImageUsage::DepthStencil | ImageUsage::Texture;
        Image* externalImage = nullptr;
        Rect2D subRect{}; // Note: 可选的 ShadowAtlas 子视口矩形分配
    };

    struct BufferRequireDesc
    {
        std::string resourceId;
        size_t size = 0;
        Buffer* externalBuffer = nullptr;
    };

    class ResourceGroup
    {
    public:
        explicit ResourceGroup(std::string name);
        ~ResourceGroup();

        ResourceGroup(const ResourceGroup&) = delete;
        ResourceGroup& operator=(const ResourceGroup&) = delete;

        const std::string& getName() const noexcept { return m_name; }

        /**
         * @brief 动态申请并归入该资源组：若已有匹配的空闲图像则复用，否则从 Context 分配
         */
        Image* require(Context* ctx, const ResourceRequireDesc& desc);

        /**
         * @brief 动态申请并归入该资源组的 Buffer 资源
         */
        Buffer* requireBuffer(Context* ctx, const BufferRequireDesc& desc);

        /**
         * @brief 归还图像资源：从组内活跃列表摘除，放入空闲池
         */
        void release(const std::string& resourceId, Context* ctx = nullptr);

        /**
         * @brief 归还 Buffer 资源
         */
        void releaseBuffer(const std::string& resourceId, Context* ctx = nullptr);

        /**
         * @brief 查询所有当前活跃的图像资源
         */
        std::vector<Image*> getActiveImages() const;

        /**
         * @brief 查询所有当前活跃的 Buffer 资源
         */
        std::vector<Buffer*> getActiveBuffers() const;

        /**
         * @brief 根据唯一 ID 获取对应图像
         */
        Image* getImage(const std::string& resourceId) const;

        /**
         * @brief 根据唯一 ID 获取对应 Buffer
         */
        Buffer* getBuffer(const std::string& resourceId) const;

        /**
         * @brief 查询 ShadowAtlas 子视口矩形
         */
        Rect2D getSubRect(const std::string& resourceId) const;

        /**
         * @brief 设置或记录 ShadowAtlas 子视口矩形
         */
        void setSubRect(const std::string& resourceId, const Rect2D& rect);

        /**
         * @brief 检查是否存在子视口矩形分配
         */
        bool hasSubRect(const std::string& resourceId) const;

        /**
         * @brief 设置主图集（用于 ShadowAtlas 模式）
         */
        void setAtlasImage(Image* atlas) noexcept { m_atlasImage = atlas; }
        Image* getAtlasImage() const noexcept { return m_atlasImage; }

        /**
         * @brief 清空组内所有资源
         */
        void clear(Context* ctx = nullptr);

    private:
        std::string m_name;
        // Note: 严格遵守零互斥锁规范（Zero mutexes），运行期由单向数据流与管线阶段调度保障安全
        std::unordered_map<std::string, Image*> m_activeImages;
        std::unordered_map<std::string, ResourceRequireDesc> m_descs;
        std::unordered_map<std::string, Rect2D> m_subRects;
        std::vector<Image*> m_idlePool;

        std::unordered_map<std::string, Buffer*> m_activeBuffers;
        std::unordered_map<std::string, BufferRequireDesc> m_bufferDescs;
        std::vector<Buffer*> m_idleBufferPool;

        Image* m_atlasImage = nullptr;
    };
} // namespace FCT

#endif // FCT_RESOURCEGROUP_H

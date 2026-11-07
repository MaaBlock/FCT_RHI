//
// Created by Administrator on 2025/8/10.
//

#ifndef RENDERGRAPHRENDERGRAPHBUFFERNODE_H
#define RENDERGRAPHRENDERGRAPHBUFFERNODE_H
namespace FCT
{
    class RenderGraphBufferNode : public RenderGraphImageNode
    {
    private:
        bool m_filled;
        Format m_format;
        Samples m_samples;
        ImageUsages m_usage;
        TextureSize m_size;
        Image* m_allocatedImage = nullptr;
        bool m_isExternal = false;
    public:
        RenderGraphBufferNode() : RenderGraphImageNode(), m_filled(false), m_format(Format::UNDEFINED),
                            m_samples(Samples::sample_undefined), m_usage(0),
                            m_size(TextureSize())
        {
        }
        /**
            * @cond CHINESE
            * 设置分配的图像
            * @param image 分配的图像指针
            * @endcond
            */
        bool isExternal() const noexcept {
            return m_isExternal;
        }
        void setAllocatedImage(Image* image) {
            m_allocatedImage = image;
        }
        Image* getImage() const override
        {
            return m_allocatedImage;
        }

        RenderGraphBufferNode(const Texture& other) : RenderGraphImageNode(other.name), m_filled(true),
                                                    m_format(other.format), m_samples(other.samples),
                                                    m_usage(other.isStorage ? (ImageUsage::Texture | ImageUsage::Storage) : ImageUsage(ImageUsage::Texture)), m_size(other.size),
                                                    m_allocatedImage(other.externalImage), m_isExternal(other.isExternal)
        {
            if (other.size.type == TextureSize::Type::Absolute) {
                setFixedSize(other.size.width, other.size.height);
            }
        }

        RenderGraphBufferNode(const Target& other) : RenderGraphImageNode(other.name), m_filled(true),
                                                   m_format(other.format), m_samples(other.samples),
                                                   m_usage(ImageUsage::RenderTarget), m_size(TextureSize()),
                                                   m_allocatedImage(other.externalImage), m_isExternal(other.isExternal)
        {
            if (other.hasFixedSize) {
                setFixedSize(other.width, other.height);
            }
        }

        RenderGraphBufferNode(const DepthStencil& other) : RenderGraphImageNode(other.name), m_filled(true),
                                                         m_format(other.format), m_samples(other.samples),
                                                         m_usage(ImageUsage::DepthStencil), m_size(TextureSize()),
                                                         m_allocatedImage(other.externalImage), m_isExternal(other.isExternal)
        {
            if (other.hasFixedSize) {
                setFixedSize(other.width, other.height);
            }
        }
        RenderGraphImageNode& operator|=(const Texture& other) override
        {
            if (m_filled) {
                checkCompatibilityAndMerge(other.name, other.format, other.samples,
                                         other.isStorage ? ImageUsage::Storage : ImageUsage::Texture, other.size);
                if (other.isStorage) { m_usage |= ImageUsage::Texture; }
                if (other.isExternal && other.externalImage) { m_allocatedImage = other.externalImage; m_isExternal = true; }
                return *this;
            }
            *this = RenderGraphBufferNode(other);
            return *this;
        }

        RenderGraphImageNode& operator|=(const Target& other) override
        {
            if (m_filled) {
                checkCompatibilityAndMerge(other.name, other.format, other.samples,
                                         ImageUsage::RenderTarget, other.hasFixedSize,
                                         other.width, other.height);
                if (other.isExternal && other.externalImage) { m_allocatedImage = other.externalImage; m_isExternal = true; }
                if (false) checkCompatibilityAndMerge(other.name, other.format, other.samples, ImageUsage::RenderTarget, other.hasFixedSize,
                                         other.width, other.height);
                return *this;
            }
            *this = RenderGraphBufferNode(other);
            return *this;
        }

        RenderGraphImageNode& operator|=(const DepthStencil& other) override
        {
            if (m_filled) {
                checkCompatibilityAndMerge(other.name, other.format, other.samples,
                                         ImageUsage::DepthStencil, other.hasFixedSize,
                                         other.width, other.height);
                if (other.isExternal && other.externalImage) { m_allocatedImage = other.externalImage; m_isExternal = true; }
                if (false) checkCompatibilityAndMerge(other.name, other.format, other.samples, ImageUsage::DepthStencil, other.hasFixedSize,
                                         other.width, other.height);
                return *this;
            }
            *this = RenderGraphBufferNode(other);
            return *this;
        }
        Format getFormat() const noexcept {
            return m_format;
        }

        Samples getSamples() const noexcept {
            return m_samples;
        }

        ImageUsages getUsage() const noexcept {
            return m_usage;
        }

        bool isFilled() const noexcept {
            return m_filled;
        }
        void fillDefaultData() override
        {
            if (m_format == Format::UNDEFINED)
            {
                m_format = Format::R8G8B8A8_UNORM;
            }
            if (m_samples == Samples::sample_undefined)
            {
                m_samples = Samples::sample_1;
            }
        }

    private:
        void checkCompatibilityAndMerge(const std::string& name, Format format, Samples samples,
                                       ImageUsage usage, const TextureSize& size) {
            checkCompatibilityAndMergeImpl(name, format, samples, usage, false, 0, 0, &size);
        }

        void checkCompatibilityAndMerge(const std::string& name, Format format, Samples samples,
                                       ImageUsage usage, bool hasFixedSize, uint32_t width, uint32_t height) {
            checkCompatibilityAndMergeImpl(name, format, samples, usage, hasFixedSize, width, height, nullptr);
        }

        void checkCompatibilityAndMergeImpl(const std::string& name, Format format, Samples samples,
                                          ImageUsage usage, bool hasFixedSize, uint32_t width, uint32_t height,
                                          const TextureSize* textureSize) {
            //检查是否冲突
            if (m_name != name) {
                throw std::runtime_error("RenderGraph conflict: Different names for same resource - existing: '" +
                                        m_name + "', new: '" + name + "'");
            }
            if (m_format != Format::UNDEFINED && format != Format::UNDEFINED && m_format != format) {
                throw std::runtime_error("RenderGraph conflict: Incompatible formats for resource '" + name +
                                        "' - existing format conflicts with new format");
            }
            if (m_samples != Samples::sample_undefined && samples != Samples::sample_undefined && m_samples != samples) {
                throw std::runtime_error("RenderGraph conflict: Incompatible sample counts for resource '" + name +
                                        "' - existing samples conflict with new samples");
            }

            m_usage |= usage;

            // 更新未定义的属性
            if (m_format == Format::UNDEFINED && format != Format::UNDEFINED) {
                m_format = format;
            }
            if (m_samples == Samples::sample_undefined && samples != Samples::sample_undefined) {
                m_samples = samples;
            }

            // 如果有固定大小，设置固定大小
            if (hasFixedSize) {
                if (!setFixedSize(width, height)) {
                    throw std::runtime_error("RenderGraph conflict: Incompatible sizes for resource '" + name + "'");
                }
            }
            if (textureSize && (usage == ImageUsage::Texture || usage == ImageUsage::Storage)) {
                if (textureSize->type == TextureSize::Type::Absolute) {
                    if (!setFixedSize(textureSize->width, textureSize->height)) {
                        throw std::runtime_error("RenderGraph conflict: Incompatible sizes for texture resource '" + name + "'");
                    }
                }
            }

        }
    };
    class RenderGraphWindowTargetNode : public RenderGraphImageNode
    {
    private:
        bool m_filled;
        Window* m_window;

    public:
        void fillDefaultData() override
        {

        }
        RenderGraphWindowTargetNode() : RenderGraphImageNode(), m_filled(false), m_window(nullptr)
        {
        }

        RenderGraphWindowTargetNode(const Target& other) : RenderGraphImageNode(other.name), m_filled(true),
                                                          m_window(other.wnd)
        {
            if (!other.isWindow) {
                throw std::runtime_error("RenderGraphWindowTargetNode can only be created from window Target");
            }
            setFixed();
        }

        RenderGraphImageNode& operator|=(const Target& other) override
        {
            if (!other.isWindow) {
                throw std::runtime_error("RenderGraphWindowTargetNode can only merge with window Target");
            }

            if (m_filled) {
                checkCompatibilityAndMerge(other.name, other.wnd);
                return *this;
            }
            *this = RenderGraphWindowTargetNode(other);
            return *this;
        }

        Window* getWindow() const noexcept {
            return m_window;
        }

        bool isValidWindowTarget() const noexcept {
            return m_filled && m_window != nullptr;
        }
        Image* getImage() const override;

    private:
        void checkCompatibilityAndMerge(const std::string& name, Window* window) {
            // 检查是否冲突
            if (m_name != name) {
                throw std::runtime_error("RenderGraph conflict: Different names for same window target - existing: '" +
                                        m_name + "', new: '" + name + "'");
            }
            if (m_window != window) {
                throw std::runtime_error("RenderGraph conflict: Different windows for same target '" + name + "'");
            }
        }

    public:
        RenderGraphImageNode& operator|=(const Texture& other) override
        {
            throw std::runtime_error("RenderGraphWindowTargetNode can't merge with texture");
        }
        RenderGraphImageNode& operator|=(const DepthStencil& other) override
        {
            throw std::runtime_error("RenderGraphWindowDepthStencilNode can't merge with depth stencil");
        }
    };

    class RenderGraphWindowDepthStencilNode : public RenderGraphImageNode
    {
    private:
        bool m_filled;
        Window* m_window;
    public:
        void fillDefaultData() override
        {

        }
        RenderGraphWindowDepthStencilNode() : RenderGraphImageNode(), m_filled(false), m_window(nullptr)
        {
        }

        RenderGraphWindowDepthStencilNode(const DepthStencil& other) : RenderGraphImageNode(other.name), m_filled(true),
                                                                      m_window(other.wnd)
        {
            if (!other.isWindow) {
                throw std::runtime_error("RenderGraphWindowDepthStencilNode can only be created from window DepthStencil");
            }
            setFixed();
        }

        RenderGraphImageNode& operator|=(const DepthStencil& other) override
        {
            if (!other.isWindow) {
                throw std::runtime_error("RenderGraphWindowDepthStencilNode can only merge with window DepthStencil");
            }

            if (m_filled) {
                checkCompatibilityAndMerge(other.name, other.wnd);
                return *this;
            }
            *this = RenderGraphWindowDepthStencilNode(other);
            return *this;
        }

        Window* getWindow() const noexcept {
            return m_window;
        }
        Image* getImage() const override;

        bool isValidWindowDepthStencil() const noexcept {
            return m_filled && m_window != nullptr;
        }

    private:
        void checkCompatibilityAndMerge(const std::string& name, Window* window) {
            // 检查是否冲突
            if (m_name != name) {
                throw std::runtime_error("RenderGraph conflict: Different names for same window depth stencil - existing: '" +
                                        m_name + "', new: '" + name + "'");
            }
            if (m_window != window) {
                throw std::runtime_error("RenderGraph conflict: Different windows for same depth stencil '" + name + "'");
            }
        }

    public:
        RenderGraphImageNode& operator|=(const Texture& other) override
        {
            throw std::runtime_error("RenderGraphWindowDepthStencilNode cannot merge with texture");
        }
        RenderGraphImageNode& operator|=(const Target& other) override
        {
            throw std::runtime_error("RenderGraphWindowDepthStencilNode cannot merge with window Target");
        }
    };

}
#endif //RENDERGRAPHRENDERGRAPHBUFFERNODE_H

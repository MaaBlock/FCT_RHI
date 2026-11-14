//
// Created by Administrator on 2025/3/5.
//

#ifndef FCT_IMAGE_H
#define FCT_IMAGE_H
#include "./IRenderTarget.h"
#include "../MutilThreadBase/RefCount.h"
#include "../RHI/Fence.h"
#include "../RHI/ImageUsageFlags.h"
#include "../Type/type.h"
#include <vector>
#include <functional>

namespace FCT {
    class Context;
    namespace RHI
    {
        class Image;
        class Fence;
        class TextureView;
        class DepthStencilView;
        class RenderTargetView;
    }

    class Image;
    class ImageBehavior
    {
    public:
        virtual Format format() const = 0;
        virtual Samples samples() const = 0;
    protected:
        Image* m_image;
    };
    class BeforeCreateImageBehavior : public ImageBehavior
    {
    public:
        BeforeCreateImageBehavior(Image* image);
        Format format() const;
        Samples samples() const;
    private:

    };
    struct UpdateResult
    {
        RHI::Fence* fence;
        std::function<void()> cleanUpCallback;
        void waitFor();
        uint8_t currentFrame;
    };
    class Image : public RefCount, public IRenderTarget {
    public:
        friend class BeforeCreateImageBehavior;
        //friend class AfterCreateImageBehavior;

        Image(Context* ctx);
        virtual ~Image();

        RenderTargetType getType() const override { return m_renderTargetType; }
        void renderTargetType(RenderTargetType type) { m_renderTargetType = type; }

        virtual void create() = 0;
        virtual void create(bool uploadData) { create(); } // Added
        
        virtual void as(ImageUsages usage) = 0;
        virtual void bind(Context* ctx) {}

        Format format() const { return m_behavior->format(); }
        Samples samples() const { return m_behavior->samples(); }

        virtual RHI::Image* currentImage() = 0;

        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual Image* getImage() const { return nullptr; }
        virtual std::vector<Image*> getTargetImages() = 0;
        virtual RHI::RenderTargetView* currentTargetView() = 0;
        virtual RHI::TextureView* currentTextureView() = 0;
        virtual RHI::DepthStencilView* currentDepthStencilView() = 0;
        int width() const { return m_width; }
        int height() const { return m_height; }
        void width(int width) { m_width = width; }
        void height(int height) { m_height = height; }
        int arrayLayers() const { return m_arrayLayers; }
        void arrayLayers(int layers) { m_arrayLayers = layers; }
        bool isCubeMap() const { return m_isCubeMap; }
        void isCubeMap(bool isCube) { m_isCubeMap = isCube; }
        void format(Format format) { m_format = format; }
        void samples(Samples samples) { m_samples = samples; }
        virtual UpdateResult* updateToCurrent(void* data,size_t size)
        {
            return nullptr;
        };
        virtual void updateData(const void* data, size_t dataSize) {}
        virtual void updateData(const void* data, size_t dataSize, RHI::Fence* fence, std::function<void()>* onCompletion) {}
        virtual void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) {}
        virtual void updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch = 0) = 0;

        virtual size_t textureViewHash() const = 0;

        virtual bool isRenderbuffer() const noexcept { return m_isRenderbuffer; }
        virtual void setIsRenderbuffer(bool renderbuffer) noexcept { m_isRenderbuffer = renderbuffer; }
    protected:
        ImageBehavior* m_behavior;
        Context* m_ctx;
        int m_width;
        int m_height;
        int m_arrayLayers = 1;
        bool m_isCubeMap = false;
        bool m_isRenderbuffer{false};
        Format m_format;
        Samples m_samples;
        RenderTargetType m_renderTargetType;
    };
} // namespace FCT

#endif //FCT_IMAGE_H
//
// Created by Administrator on 2025/3/30.
//
#include "./Image.h"
#include "../RHI/TextureView.h"
#ifndef SINGLEBUFFERIMAGE_H
#define SINGLEBUFFERIMAGE_H
namespace FCT
{

    class SingleBufferAfterCreateImageBehavior : public ImageBehavior
    {
    public:
        SingleBufferAfterCreateImageBehavior(Image* image);
        Format format() const;
        Samples samples() const;
    };
    class SingleBufferImage : public Image {
    public:
        friend class SingleBufferAfterCreateImageBehavior;
        SingleBufferImage(Context* ctx);
        ~SingleBufferImage() override;
        void initData(void* data,int size)
        {
            m_initData.data = data;
            m_initData.size = size;
        }
        void create() override;
        void create(bool uploadData) override;
        void create(RHI::Image* image);
        void as(ImageUsages usage) override;
        void bind(Context* ctx) override;
        void resize(uint32_t width, uint32_t height) override;
        
        void updateData(const void* data, size_t dataSize) override;
        void updateData(const void* data, size_t dataSize, RHI::Fence* fence, std::function<void()>* onCompletion) override;
        void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) override;
        void updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch = 0) override;

        Image* getImage() const override { return const_cast<SingleBufferImage*>(this); }
        std::vector<Image*> getTargetImages() override;

        RHI::RenderTargetView* currentTargetView() override;
        RHI::TextureView* currentTextureView() override;
        RHI::DepthStencilView* currentDepthStencilView() override;
        RHI::Image* currentImage() override;
        size_t textureViewHash() const override;

        bool isRenderbuffer() const noexcept override;
        void setIsRenderbuffer(bool renderbuffer) noexcept override;
    protected:
        RHI::Image* m_image;
        RHI::RenderTargetView* m_rtv;
        RHI::DepthStencilView* m_dsv = nullptr;
        RHI::TextureView* m_srv;
        ImageInitData m_initData;
        ImageUsages m_usage;
    };
}


#endif //SINGLEBUFFERIMAGE_H

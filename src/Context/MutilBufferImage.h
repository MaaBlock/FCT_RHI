//
// Created by Administrator on 2025/3/30.
//
#include "./Image.h"
#ifndef MUTILBUFFERIMAGE_H
#define MUTILBUFFERIMAGE_H


namespace FCT
{
    class MutilBufferImage;
    class Context;
    class MutilBufferAffterCreateImageBehavior : public ImageBehavior
    {
    public:
        MutilBufferAffterCreateImageBehavior(MutilBufferImage* image);
        Format format() const override;
        Samples samples() const override;
    private:
    };
    class MutilBufferImage : public Image{
    public:
        friend class MutilBufferAffterCreateImageBehavior;
        MutilBufferImage(Context* ctx);
        ~MutilBufferImage() override;
        void imageCount(size_t count) { m_imageCount = count; }
        void create() override;
        void resize(uint32_t width, uint32_t height);
        void create(std::vector<RHI::Image*> images);
        void as(ImageUsages usage) override;
        void bind(Context* ctx) override;
        Image* getImage() const override { return const_cast<MutilBufferImage*>(this); }
        std::vector<Image*> getTargetImages() override;
        RHI::RenderTargetView* currentTargetView() override;
        RHI::TextureView* currentTextureView() override;
        RHI::DepthStencilView* currentDepthStencilView() override;
        RHI::Image* currentImage() override;
        void changeCurrentIndex(int index)
        {
            m_currentIndex = index % m_images.size();
        }
        void updateData(const void* data, size_t dataSize) override;
        UpdateResult* updateToCurrent(void* data, size_t size) override;
        uint32_t imageCount() const
        {
            return m_imageCount;
        }
        std::vector<RHI::Image*> images() const { return m_images; }
        std::vector<RHI::RenderTargetView*> rtvs() const { return m_rtvs; }
        std::vector<RHI::DepthStencilView*> dsvs() const { return m_dsvs; }
        std::vector<RHI::TextureView*> tvs() const { return m_tvs; }
        size_t textureViewHash() const override;
    private:
        size_t m_imageCount = 0;
        ImageUsages m_usage;
        size_t m_currentIndex = 0;
        std::vector<RHI::Image*> m_images;
        std::vector<RHI::RenderTargetView*> m_rtvs;
        std::vector<RHI::DepthStencilView*> m_dsvs;
        std::vector<RHI::TextureView*> m_tvs;
    };

}


#endif //MUTILBUFFERIMAGE_H

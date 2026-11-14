//
// Created by Administrator on 2025/3/30.
//

#include "../headers.h"
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace FCT
{
    namespace
    {
        std::atomic<size_t> g_nextSingleBufferTextureViewId{1};
        std::mutex g_singleBufferViewIdMutex;
        std::unordered_map<const SingleBufferImage*, size_t> g_singleBufferViewIds;

        void assignNewTextureViewId(const SingleBufferImage* img)
        {
            size_t newId = g_nextSingleBufferTextureViewId.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(g_singleBufferViewIdMutex);
            g_singleBufferViewIds[img] = newId;
        }

        void removeTextureViewId(const SingleBufferImage* img)
        {
            std::lock_guard<std::mutex> lock(g_singleBufferViewIdMutex);
            g_singleBufferViewIds.erase(img);
        }

        size_t lookupTextureViewId(const SingleBufferImage* img, RHI::TextureView* fallbackSrv)
        {
            std::lock_guard<std::mutex> lock(g_singleBufferViewIdMutex);
            auto it = g_singleBufferViewIds.find(img);
            if (it != g_singleBufferViewIds.end()) {
                return it->second;
            }
            return reinterpret_cast<size_t>(fallbackSrv);
        }
    }

    SingleBufferAfterCreateImageBehavior::SingleBufferAfterCreateImageBehavior(Image* image) {
        m_image = image;
    }

    Format SingleBufferAfterCreateImageBehavior::format() const {
        return static_cast<SingleBufferImage*>(m_image)->m_image->format();
    }

    Samples SingleBufferAfterCreateImageBehavior::samples() const {
        return static_cast<SingleBufferImage*>(m_image)->m_image->samples();
    }

    SingleBufferImage::SingleBufferImage(Context* ctx)
        : Image(ctx), m_image(nullptr), m_rtv(nullptr), m_dsv(nullptr), m_srv(nullptr), m_initData{}, m_usage() {
        m_initData.data = nullptr;
        m_initData.size = 0;
        m_behavior = new BeforeCreateImageBehavior(this);
    }

    SingleBufferImage::~SingleBufferImage() {
        removeTextureViewId(this);
        FCT_SAFE_RELEASE(m_rtv);
        FCT_SAFE_RELEASE(m_dsv);
        FCT_SAFE_RELEASE(m_srv);
        FCT_SAFE_RELEASE(m_image);
    }

    void SingleBufferImage::create() {
        create(true);
    }
    
    void SingleBufferImage::create(bool uploadData) {
        FCT_SAFE_RELEASE(m_rtv);
        FCT_SAFE_RELEASE(m_dsv);
        FCT_SAFE_RELEASE(m_srv);
        FCT_SAFE_RELEASE(m_image);
        m_image = m_ctx->createResource<RHI::Image>();
        m_image->width(m_width);
        m_image->height(m_height);
        m_image->format(m_format);
        m_image->samples(m_samples);
        m_image->arrayLayers(m_arrayLayers);
        m_image->isCubeMap(m_isCubeMap);
        m_image->initData(m_initData);
        m_image->usage(m_usage);
        m_image->setRenderbuffer(m_isRenderbuffer);
        m_image->create(uploadData);
        delete m_behavior;
        m_behavior = new SingleBufferAfterCreateImageBehavior(this);
        as(m_usage);
    }

    void SingleBufferImage::uploadAsync(std::vector<uint8_t> data, std::function<void()> callback)
    {
        if (m_image) {
            m_image->uploadAsync(std::move(data), callback);
        }
    }

    void SingleBufferImage::updateData(const void* data, size_t dataSize)
    {
        if (m_image) m_image->updateData(data, dataSize);
    }

    void SingleBufferImage::updateData(const void* data, size_t dataSize, RHI::Fence* fence, std::function<void()>* onCompletion)
    {
        if (m_image) m_image->updateData(data, dataSize, fence, onCompletion);
    }

    void SingleBufferImage::updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch)
    {
        if (m_image) m_image->updateSubRegion(x, y, width, height, arrayLayer, mipLevel, pixelData, rowPitch);
    }

    void SingleBufferImage::create(RHI::Image* image) {
        FCT_SAFE_RELEASE(m_rtv);
        FCT_SAFE_RELEASE(m_dsv);
        FCT_SAFE_RELEASE(m_srv);
        FCT_SAFE_RELEASE(m_image);
        m_image = image;
        if (m_image) {
            m_image->addRef();
            m_width = image->width();
            m_height = image->height();
        }
        delete m_behavior;
        m_behavior = new SingleBufferAfterCreateImageBehavior(this);
        as(m_usage);
    }

    void SingleBufferImage::as(ImageUsages usage) {
        m_usage |= usage;
        if (!m_image || !m_ctx) {
            return;
        }
        if ((m_usage & ImageUsage::RenderTarget) && !m_rtv) {
            m_rtv = m_ctx->createResource<RHI::RenderTargetView>();
            if (m_rtv) {
                m_rtv->image(m_image);
                m_rtv->create();
            }
        }
        if ((m_usage & ImageUsage::DepthStencil) && !m_dsv) {
            m_dsv = m_ctx->createResource<RHI::DepthStencilView>();
            if (m_dsv) {
                m_dsv->image(m_image);
                (void)m_dsv->create();
            }
        }
        if ((m_usage & ImageUsage::Texture) && !m_srv) {
            m_srv = m_ctx->createResource<RHI::TextureView>();
            if (m_srv) {
                m_srv->image(m_image);
                m_srv->create();
                assignNewTextureViewId(this);
            }
        }
    }

    void SingleBufferImage::bind(Context* ctx) {

    }

    void SingleBufferImage::resize(uint32_t width, uint32_t height)
    {
        if (m_width == width && m_height == height)
        {
            return;
        }

        Format currentFormat = m_format;
        Samples currentSamples = m_samples;
        ImageUsages currentUsage = m_usage;

        FCT_SAFE_RELEASE(m_rtv);
        FCT_SAFE_RELEASE(m_dsv);
        FCT_SAFE_RELEASE(m_srv);
        FCT_SAFE_RELEASE(m_image);
        removeTextureViewId(this);

        m_width = width;
        m_height = height;

        m_image = m_ctx->createResource<RHI::Image>();
        m_image->width(m_width);
        m_image->height(m_height);
        m_image->format(currentFormat);
        m_image->samples(currentSamples);
        m_image->usage(currentUsage);
        m_image->setRenderbuffer(m_isRenderbuffer);
        m_image->create();

        as(currentUsage);
    }

    std::vector<Image*> SingleBufferImage::getTargetImages() {
        if (m_rtv) {
            return {this};
        }
        return {};
    }

    RHI::RenderTargetView* SingleBufferImage::currentTargetView()
    {
        return m_rtv;
    }

    RHI::TextureView* SingleBufferImage::currentTextureView()
    {
        return m_srv;
    }

    RHI::DepthStencilView* SingleBufferImage::currentDepthStencilView()
    {
        return m_dsv;
    }

    RHI::Image* SingleBufferImage::currentImage()
    {
        return m_image;
    }

    size_t SingleBufferImage::textureViewHash() const
    {
        return lookupTextureViewId(this, m_srv);
    }

    bool SingleBufferImage::isRenderbuffer() const noexcept
    {
        return m_image ? m_image->isRenderbuffer() : m_isRenderbuffer;
    }

    void SingleBufferImage::setIsRenderbuffer(bool renderbuffer) noexcept
    {
        m_isRenderbuffer = renderbuffer;
        if (m_image) {
            m_image->setRenderbuffer(renderbuffer);
        }
    }
}

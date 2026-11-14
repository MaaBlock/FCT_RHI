//
// Created by Administrator on 2025/3/27.
//
#include "../Type/type.h"
#include "../MutilThreadBase/RefCount.h"
#include "./ImageUsageFlags.h"

#ifndef FCT_RHI_IMAGE_H
#define FCT_RHI_IMAGE_H
namespace FCT
{
    namespace RHI
    {
        class Fence;

        class Image : public RefCount
        {
        public:
            Image()
            {
                m_initData.data = nullptr;
                m_initData.size = 0;
            }
            ~Image() = default;
            Format format() const
            {
                return m_format;
            }
            void format(Format format)
            {
                m_format = format;
            }
            Samples samples() const
            {
                return m_samples;
            }
            void samples(Samples samples)
            {
                m_samples = samples;
            }
            void width(uint32_t width)
            {
                m_width = width;
            }
            void height(uint32_t height)
            {
                m_height = height;
            }
            uint32_t width() const
            {
                return m_width;
            }
            void arrayLayers(uint32_t layers)
            {
                m_arrayLayers = layers;
            }
            uint32_t arrayLayers() const
            {
                return m_arrayLayers;
            }
            void isCubeMap(bool isCube)
            {
                m_isCubeMap = isCube;
            }
            bool isCubeMap() const
            {
                return m_isCubeMap;
            }
            void initData(void* data, size_t size)
            {
                m_initData.data = data;
                m_initData.size = size;
            }
            void initData(ImageInitData data)
            {
                m_initData = data;
            }
            uint32_t height() const
            {
                return m_height;
            }
            virtual void create() = 0;
            virtual void create(bool uploadData) { create(); } // Default implementation for backward compatibility
            
            void usage(ImageUsages usage)
            {
                m_usage = usage;
            }
            virtual void updateData(const void* data, size_t dataSize) = 0;
            virtual void updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion) = 0;
            virtual void updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch = 0) = 0;
            
            virtual void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) {} // Default empty

            virtual bool isRenderbuffer() const noexcept { return false; }
            virtual void setRenderbuffer(bool renderbuffer) noexcept { (void)renderbuffer; }

        protected:
            Format m_format;
            Samples m_samples;
            uint32_t m_width;
            uint32_t m_height;
            uint32_t m_arrayLayers = 1;
            bool m_isCubeMap = false;
            ImageUsages m_usage;
            ImageInitData m_initData;
        };
    }
}
#endif //FCT_RHI_IMAGE_H

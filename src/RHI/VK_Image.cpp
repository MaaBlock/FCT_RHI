//
// Created by Administrator on 2025/3/27.
//

#include "VK_Image.h"
#include "VK_Fence.h"
#include "../ThirdParty.h"
#include "../Context/VK_Context.h"

namespace FCT {
    namespace RHI {
        VK_Image::VK_Image(VK_Context* ctx)
        {
            m_ctx = ctx;
            m_owns = false;
        }

        VK_Image::~VK_Image()
        {
            if (m_owns) {
                m_ctx->getDevice().destroyImage(m_image);
                m_ctx->device().freeMemory(m_memory);
            }
        }

        void VK_Image::create()
        {
            create(true);
        }

        void VK_Image::create(bool uploadData)
        {
            vk::ImageCreateInfo imageInfo;
            imageInfo.setImageType(vk::ImageType::e2D);
            imageInfo.setFormat(ToVkFormat(m_format));
            imageInfo.setExtent(vk::Extent3D(m_width, m_height, 1));
            imageInfo.setMipLevels(1);
            imageInfo.setArrayLayers(m_arrayLayers);
            imageInfo.setSamples(ToVkSampleCount(m_samples));
            imageInfo.setTiling(vk::ImageTiling::eOptimal);
            if (m_isCubeMap) {
                imageInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
            }
            vk::ImageUsageFlags usageFlags;
            if (m_usage & ImageUsage::Texture)
            {
                usageFlags |= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
            }
            if (m_usage & ImageUsage::RenderTarget)
            {
                usageFlags |= vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc |
                              vk::ImageUsageFlagBits::eSampled;
            }
            if (m_usage & ImageUsage::DepthStencil)
            {
                usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            }
            if (m_usage & ImageUsage::Storage)
            {
                usageFlags |= vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc;
            }
            if (!(usageFlags & vk::ImageUsageFlagBits::eTransferDst)) {
                usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
            }
            imageInfo.setUsage(usageFlags);
            imageInfo.setSharingMode(vk::SharingMode::eExclusive);
            imageInfo.setInitialLayout(vk::ImageLayout::eUndefined);

            try {
                m_image = m_ctx->getDevice().createImage(imageInfo);

                vk::MemoryRequirements memRequirements = m_ctx->getDevice().getImageMemoryRequirements(m_image);

                vk::MemoryAllocateInfo allocInfo;
                allocInfo.setAllocationSize(memRequirements.size);
                allocInfo.setMemoryTypeIndex(m_ctx->findMemoryType(
                    memRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eDeviceLocal));

                m_memory = m_ctx->getDevice().allocateMemory(allocInfo);

                m_ctx->getDevice().bindImageMemory(m_image, m_memory, 0);

                if (uploadData && m_initData.data && m_initData.size > 0) {
                    uploadInitialData();
                }

                m_owns = true;
            }
            catch (const std::exception& e) {
                ferr << "Failed to create Vulkan image: " << e.what() << std::endl;
                if (m_image) {
                    m_ctx->getDevice().destroyImage(m_image);
                    m_image = nullptr;
                }

                if (m_memory) {
                    m_ctx->getDevice().freeMemory(m_memory);
                    m_memory = nullptr;
                }

                m_owns = false;
            }
        }

        void VK_Image::uploadInitialData()
        {
            if (m_initData.data && m_initData.size > 0) {
                updateData(m_initData.data, m_initData.size);
            }
        }
        
        void VK_Image::uploadAsync(std::vector<uint8_t> data, std::function<void()> callback)
        {
            vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
            if (m_usage & ImageUsage::DepthStencil) {
                aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            }

            m_ctx->asyncTransferDataToImage(
                m_image,
                m_width,
                m_height,
                1,
                ToVkFormat(m_format),
                1,
                m_arrayLayers,
                aspectMask,
                std::move(data),
                callback
            );
        }


        void VK_Image::updateData(const void* data, size_t dataSize)
        {
            if (!data || dataSize == 0) {
                ferr << "Invalid data for image update" << std::endl;
                return;
            }

            try {
                vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;

                if (m_usage & ImageUsage::DepthStencil) {
                    aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
                } else {
                    aspectMask = vk::ImageAspectFlagBits::eColor;
                }

                vk::ImageLayout targetLayout = (m_usage & ImageUsage::RenderTarget)
                    ? vk::ImageLayout::ePresentSrcKHR
                    : vk::ImageLayout::eShaderReadOnlyOptimal;

                m_ctx->transferDataToImage(
                    m_image,
                    m_width,
                    m_height,
                    1,
                    ToVkFormat(m_format),
                    1,
                    m_arrayLayers,
                    aspectMask,
                    data,
                    dataSize,
                    nullptr,
                    nullptr,
                    targetLayout
                );
            }
            catch (const std::exception& e) {
                ferr << "Failed to update image data: " << e.what() << std::endl;
            }
        }
        void VK_Image::updateData(const void* data, size_t dataSize,Fence* fence,std::function<void()>* onCompletion)
        {
            if (!data || dataSize == 0) {
                ferr << "Invalid data for image update" << std::endl;
                return;
            }

            try {
                vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;

                if (m_usage & ImageUsage::DepthStencil) {
                    aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
                } else {
                    aspectMask = vk::ImageAspectFlagBits::eColor;
                }

                vk::ImageLayout targetLayout = (m_usage & ImageUsage::RenderTarget)
                    ? vk::ImageLayout::ePresentSrcKHR
                    : vk::ImageLayout::eShaderReadOnlyOptimal;

                m_ctx->transferDataToImage(
                    m_image,
                    m_width,
                    m_height,
                    1,
                    ToVkFormat(m_format),
                    1,
                    m_arrayLayers,
                    aspectMask,
                    data,
                    dataSize,
                    static_cast<VK_Fence*>(fence)->fencePtr(),
                    onCompletion,
                    targetLayout
                );
            }
            catch (const std::exception& e) {
                ferr << "Failed to update image data: " << e.what() << std::endl;
            }
        }
        void VK_Image::updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                       uint32_t arrayLayer, uint32_t mipLevel,
                                       const void* pixelData, size_t rowPitch)
        {
            if (!pixelData || width == 0 || height == 0 || !m_image) return;
            size_t pixelSize = FormatSize(m_format);
            if (pixelSize == 0) pixelSize = 4;
            size_t actualPitch = (rowPitch > 0) ? rowPitch : (width * pixelSize);
            size_t copySize = actualPitch * height;

            vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
            if (m_usage & ImageUsage::DepthStencil) {
                aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            }

            vk::BufferCreateInfo stagingBufferInfo;
            stagingBufferInfo.setSize(copySize);
            stagingBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
            stagingBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
            vk::Buffer stagingBuffer = m_ctx->getDevice().createBuffer(stagingBufferInfo);

            vk::MemoryRequirements memReq = m_ctx->getDevice().getBufferMemoryRequirements(stagingBuffer);
            vk::MemoryAllocateInfo allocInfo(memReq.size, m_ctx->findMemoryType(memReq.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
            vk::DeviceMemory stagingMem = m_ctx->getDevice().allocateMemory(allocInfo);
            m_ctx->getDevice().bindBufferMemory(stagingBuffer, stagingMem, 0);

            void* mapped = m_ctx->getDevice().mapMemory(stagingMem, 0, copySize);
            std::memcpy(mapped, pixelData, copySize);
            m_ctx->getDevice().unmapMemory(stagingMem);

            vk::CommandBuffer cmd = m_ctx->beginSingleTimeCommands();

            vk::ImageMemoryBarrier barrier;
            barrier.setOldLayout(vk::ImageLayout::eUndefined)
                   .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                   .setImage(m_image)
                   .setSubresourceRange(vk::ImageSubresourceRange(aspectMask, mipLevel, 1, arrayLayer, 1))
                   .setSrcAccessMask(vk::AccessFlags())
                   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                                vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

            vk::BufferImageCopy region;
            region.setBufferOffset(0)
                  .setBufferRowLength(static_cast<uint32_t>(actualPitch / pixelSize))
                  .setBufferImageHeight(height)
                  .setImageSubresource(vk::ImageSubresourceLayers(aspectMask, mipLevel, arrayLayer, 1))
                  .setImageOffset(vk::Offset3D(static_cast<int32_t>(x), static_cast<int32_t>(y), 0))
                  .setImageExtent(vk::Extent3D(width, height, 1));
            cmd.copyBufferToImage(stagingBuffer, m_image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

            vk::ImageLayout targetLayout = (m_usage & ImageUsage::RenderTarget)
                ? vk::ImageLayout::ePresentSrcKHR
                : vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                   .setNewLayout(targetLayout)
                   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                   .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

            m_ctx->endSingleTimeCommands(cmd);

            m_ctx->getDevice().destroyBuffer(stagingBuffer);
            m_ctx->getDevice().freeMemory(stagingMem);
        }

        void VK_Image::create(vk::Image image)
        {
            m_image = image;
            m_owns = false;
        }

        vk::Image VK_Image::getVkImage()
        {
            return m_image;
        }
    }
}
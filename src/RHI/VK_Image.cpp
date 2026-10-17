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
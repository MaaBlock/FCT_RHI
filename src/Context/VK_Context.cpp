#include "../FCTAPI.h"
#include "./VK_Device.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace FCT
{



    /*Material *VK_Context::createMaterial(VertexShader *vertexShader, PixelShader *pixelShader) {
        return nullptr;
    }*/

    RHI::RasterizationPipeline* VK_Context::createTraditionPipeline()
    {
        return new RHI::VK_TraditionalPipeline(this);
    }

    RHI::ConstBuffer* VK_Context::getEmptyConstBuffer(const ConstLayout& layout)
    {
        size_t layoutHash = layout.getHash();
        auto it = m_emptyConstBufferCache.find(layoutHash);
        if (it != m_emptyConstBufferCache.end())
        {
            return it->second.get();
        }

        auto emptyBuffer = createResource<RHI::ConstBuffer>();
        emptyBuffer->layout(layout);
        emptyBuffer->create();

        RHI::ConstBuffer* bufferPtr = emptyBuffer;
        m_emptyConstBufferCache[layoutHash] = std::unique_ptr<RHI::ConstBuffer>(bufferPtr);
        return bufferPtr;
    }

        Sampler* VK_Context::getEmptySampler()
    {
        if (m_emptySampler)
        {
            return m_emptySampler.get();
        }

        auto sampler = createResource<Sampler>();
        sampler->setFilter(FCT::FilterMode::Linear, FCT::FilterMode::Linear, FCT::FilterMode::Linear);
        sampler->setAddressMode(FCT::AddressMode::ClampToEdge, FCT::AddressMode::ClampToEdge, FCT::AddressMode::ClampToEdge);
        sampler->create();

        m_emptySampler = std::unique_ptr<Sampler>(sampler);
        return m_emptySampler.get();
    }

    VK_Context::~VK_Context()
    {
        if (m_device) {
            if (m_syncStagingBuffer) {
                m_device.destroyBuffer(m_syncStagingBuffer);
                m_syncStagingBuffer = nullptr;
            }
            if (m_syncStagingMemory) {
                m_device.freeMemory(m_syncStagingMemory);
                m_syncStagingMemory = nullptr;
            }
            m_device.freeCommandBuffers(m_commandPool, m_commandBuffers);
            m_device.destroyCommandPool(m_commandPool);
            if (m_transferCommandPool) {
                m_device.destroyCommandPool(m_transferCommandPool);
            }
            m_device.destroy();
        }
    }

    VK_Context::VK_Context(VK_ContextCommon *common) : Context(common->runtime()) {
        m_resourceDevice = new VK_Device(this);
        m_common = common;
        m_phyDevice = common->getPhysicalDevice();

        vk::PhysicalDeviceDescriptorIndexingFeaturesEXT supportedDescriptorIndexingFeatures{};
        vk::PhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.pNext = &supportedDescriptorIndexingFeatures;
        m_phyDevice.getFeatures2(&deviceFeatures2);

        m_bindlessSupported = supportedDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing &&
                              supportedDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount &&
                              supportedDescriptorIndexingFeatures.descriptorBindingPartiallyBound &&
                              supportedDescriptorIndexingFeatures.runtimeDescriptorArray;
    }
    uint32_t VK_Context::getGraphicsQueueFamily() const {
        return m_graphicsQueueFamilyIndex;
    }
    void VK_Context::createTransferCommandPool() {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.setQueueFamilyIndex(m_transferQueueFamilyIndex)
                .setFlags(vk::CommandPoolCreateFlagBits::eTransient |
                          vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_transferCommandPool = m_device.createCommandPool(poolInfo);
    }
    vk::CommandBuffer VK_Context::beginSingleTimeTransferCommands() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        allocInfo.setCommandPool(m_transferCommandPool);
        allocInfo.setCommandBufferCount(1);

        vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void VK_Context::endSingleTimeTransferCommands(vk::CommandBuffer commandBuffer, vk::Fence* outFence) {
        commandBuffer.end();

        vk::FenceCreateInfo fenceInfo;
        vk::Fence fence = m_device.createFence(fenceInfo);

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&commandBuffer);

        m_transferQueue.submit(1, &submitInfo, fence);

        if (outFence) {
            *outFence = fence;
        } else {
            m_device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
            m_device.destroyFence(fence);
        m_device.freeCommandBuffers(m_transferCommandPool, 1, &commandBuffer);
        }

    }

    vk::CommandBuffer VK_Context::beginSingleTimeCommands()
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        allocInfo.setCommandPool(m_commandPool);
        allocInfo.setCommandBufferCount(1);

        vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void VK_Context::endSingleTimeCommands(vk::CommandBuffer commandBuffer)
    {
        commandBuffer.end();

        vk::FenceCreateInfo fenceInfo;
        vk::Fence fence = m_device.createFence(fenceInfo);

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&commandBuffer);

        m_graphicsQueue.submit(1, &submitInfo, fence);
        m_device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
        m_device.destroyFence(fence);

        m_device.freeCommandBuffers(m_commandPool, 1, &commandBuffer);
    }


    void VK_Context::createCommandPoolAndBuffers() {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.setQueueFamilyIndex(m_graphicsQueueFamilyIndex)
                 .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_commandPool = m_device.createCommandPool(poolInfo);
/*
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setCommandPool(m_commandPool)
                  .setLevel(vk::CommandBufferLevel::ePrimary)
                  .setCommandBufferCount(1);

        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);*/
    }

    /* void VK_Context::create(IRenderTarget *target) {
         //target->bind(this);
     }*/

    void VK_Context::transferDataToBuffer(vk::Buffer dstBuffer, size_t size, const void* data)
    {
        ZoneScoped();
        vk::BufferCreateInfo stagingBufferInfo;
        stagingBufferInfo.setSize(size);
        stagingBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        stagingBufferInfo.setSharingMode(vk::SharingMode::eExclusive);

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;

        try {
            stagingBuffer = m_device.createBuffer(stagingBufferInfo);

            vk::MemoryRequirements memRequirements = m_device.getBufferMemoryRequirements(stagingBuffer);

            vk::MemoryAllocateInfo allocInfo;
            allocInfo.setAllocationSize(memRequirements.size);
            allocInfo.setMemoryTypeIndex(findMemoryType(
                memRequirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

            stagingMemory = m_device.allocateMemory(allocInfo);
            m_device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

            void* mappedData = m_device.mapMemory(stagingMemory, 0, size);
            memcpy(mappedData, data, size);
            m_device.unmapMemory(stagingMemory);

            vk::CommandBuffer cmdBuffer = beginSingleTimeCommands();

            vk::BufferCopy copyRegion;
            copyRegion.setSrcOffset(0);
            copyRegion.setDstOffset(0);
            copyRegion.setSize(size);
            cmdBuffer.copyBuffer(stagingBuffer, dstBuffer, 1, &copyRegion);

            endSingleTimeCommands(cmdBuffer);

            m_device.destroyBuffer(stagingBuffer);
            m_device.freeMemory(stagingMemory);
        }
        catch (const std::exception& e) {
            ferr << "Failed to transfer data to buffer: " << e.what() << std::endl;

            if (stagingBuffer) {
                m_device.destroyBuffer(stagingBuffer);
            }

            if (stagingMemory) {
                m_device.freeMemory(stagingMemory);
            }

            throw;
        }
    }
    void VK_Context::transferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height,
                                        vk::Format format, const void* data, size_t dataSize, vk::Fence* outFence, std::function<void()>* cleanUpCallback)
    {
        transferDataToImage(dstImage, width, height, 1, format, 1, 1,
                           vk::ImageAspectFlagBits::eColor, data, dataSize, outFence, cleanUpCallback);
    }

       void VK_Context::transferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height, uint32_t depth,
                                         vk::Format format, uint32_t mipLevels, uint32_t arrayLayers,
                                         vk::ImageAspectFlags aspectMask, const void* data, size_t dataSize,
                                         vk::Fence* outFence, std::function<void()>* cleanUpCallback,
                                         vk::ImageLayout finalLayout)
    {
        ZoneScoped();
        size_t pixelSize = FormatSize(FromVkFormat(format));
        size_t fullImageSize = width * height * depth * pixelSize * arrayLayers;
        vk::BufferCreateInfo stagingBufferInfo;
        stagingBufferInfo.setSize(fullImageSize);
        stagingBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        stagingBufferInfo.setSharingMode(vk::SharingMode::eExclusive);

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;
        bool isCachedStaging = (outFence == nullptr);

        if (finalLayout == vk::ImageLayout::ePresentSrcKHR)
        {
            m_device.waitIdle();
        }

        try
        {
            if (isCachedStaging)
            {
                if (m_syncStagingBufferSize < fullImageSize)
                {
                    if (m_syncStagingBuffer)
                    {
                        m_device.destroyBuffer(m_syncStagingBuffer);
                        m_syncStagingBuffer = nullptr;
                    }
                    if (m_syncStagingMemory)
                    {
                        m_device.freeMemory(m_syncStagingMemory);
                        m_syncStagingMemory = nullptr;
                    }

                    stagingBufferInfo.setSize(fullImageSize);
                    m_syncStagingBuffer = m_device.createBuffer(stagingBufferInfo);

                    vk::MemoryRequirements memRequirements = m_device.getBufferMemoryRequirements(m_syncStagingBuffer);
                    vk::MemoryAllocateInfo allocInfo;
                    allocInfo.setAllocationSize(memRequirements.size);
                    allocInfo.setMemoryTypeIndex(
                        findMemoryType(memRequirements.memoryTypeBits,
                                       vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

                    m_syncStagingMemory = m_device.allocateMemory(allocInfo);
                    m_device.bindBufferMemory(m_syncStagingBuffer, m_syncStagingMemory, 0);
                    m_syncStagingBufferSize = fullImageSize;
                }
                stagingBuffer = m_syncStagingBuffer;
                stagingMemory = m_syncStagingMemory;
            }
            else
            {
                stagingBuffer = m_device.createBuffer(stagingBufferInfo);

                vk::MemoryRequirements memRequirements = m_device.getBufferMemoryRequirements(stagingBuffer);

                vk::MemoryAllocateInfo allocInfo;
                allocInfo.setAllocationSize(memRequirements.size);
                allocInfo.setMemoryTypeIndex(
                    findMemoryType(memRequirements.memoryTypeBits,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

                stagingMemory = m_device.allocateMemory(allocInfo);
                m_device.bindBufferMemory(stagingBuffer, stagingMemory, 0);
            }

            void* mappedData = m_device.mapMemory(stagingMemory, 0, dataSize);
            memcpy(mappedData, data, dataSize);
            m_device.unmapMemory(stagingMemory);
            vk::CommandBuffer cmdBuffer = beginSingleTimeTransferCommands();

            bool needsQueueTransfer = m_graphicsQueueFamilyIndex != m_transferQueueFamilyIndex;

            vk::AccessFlags finalAccessMask;

            if (finalLayout == vk::ImageLayout::eUndefined)
            {
                if (aspectMask & vk::ImageAspectFlagBits::eDepth || aspectMask & vk::ImageAspectFlagBits::eStencil)
                {
                    finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
                    finalAccessMask =
                        vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                }
                else
                {
                    finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    finalAccessMask = vk::AccessFlagBits::eShaderRead;
                }
            }
            else if (finalLayout == vk::ImageLayout::ePresentSrcKHR)
            {
                finalAccessMask = vk::AccessFlagBits::eMemoryRead;
            }
            else
            {
                finalAccessMask = vk::AccessFlagBits::eShaderRead;
            }

            vk::ImageMemoryBarrier barrier;
            barrier.setOldLayout(vk::ImageLayout::eUndefined);
            barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);

            if (needsQueueTransfer)
            {
                barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            }
            else
            {
                barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            }

            barrier.setImage(dstImage);
            barrier.setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, mipLevels, 0, arrayLayers));
            barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

            cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

            vk::BufferImageCopy region;
            region.setBufferOffset(0);
            region.setBufferRowLength(0);
            region.setBufferImageHeight(0);
            region.setImageSubresource(vk::ImageSubresourceLayers(aspectMask, 0, 0, arrayLayers));
            region.setImageOffset(vk::Offset3D(0, 0, 0));
            region.setImageExtent(vk::Extent3D(width, height, depth));

            cmdBuffer.copyBufferToImage(stagingBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &region);

            barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);

            if (needsQueueTransfer)
            {
                barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
                barrier.setSrcQueueFamilyIndex(m_transferQueueFamilyIndex);
                barrier.setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex);
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            }
            else
            {
                barrier.setNewLayout(finalLayout);
                barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                barrier.setDstAccessMask(finalAccessMask);
            }

            cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

            if (outFence)
            {
                vk::Fence fence;
                endSingleTimeTransferCommands(cmdBuffer, &fence);

                if (cleanUpCallback)
                {
                    vk::Buffer capturedBuffer = stagingBuffer;
                    vk::DeviceMemory capturedMemory = stagingMemory;
                    vk::Image capturedImage = dstImage;
                    bool capturedNeedsQueueTransfer = needsQueueTransfer;
                    vk::ImageAspectFlags capturedAspectMask = aspectMask;
                    uint32_t capturedMipLevels = mipLevels;
                    uint32_t capturedArrayLayers = arrayLayers;
                    vk::ImageLayout capturedFinalLayout = finalLayout;
                    vk::AccessFlags capturedFinalAccessMask = finalAccessMask;
                    vk::Fence capturedFence = fence;
                    vk::CommandBuffer capturedCmdBuffer = cmdBuffer;

                    *cleanUpCallback = [this, capturedBuffer, capturedMemory, capturedImage, capturedNeedsQueueTransfer,
                                        capturedAspectMask, capturedMipLevels, capturedArrayLayers, capturedFinalLayout,
                                        capturedFinalAccessMask, capturedFence, capturedCmdBuffer]()
                    {
                        m_device.waitForFences(1, &capturedFence, VK_TRUE, UINT64_MAX);
                        m_device.destroyFence(capturedFence);
                        postLogicTask([this, capturedCmdBuffer]()
                                      { m_device.freeCommandBuffers(m_transferCommandPool, 1, &capturedCmdBuffer); });
                        if (capturedNeedsQueueTransfer)
                        {
                            vk::CommandBuffer graphicsCmdBuffer = beginSingleTimeCommands();

                            vk::ImageMemoryBarrier finalBarrier;
                            finalBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
                            finalBarrier.setNewLayout(capturedFinalLayout);
                            finalBarrier.setSrcQueueFamilyIndex(m_transferQueueFamilyIndex);
                            finalBarrier.setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex);
                            finalBarrier.setImage(capturedImage);
                            finalBarrier.setSubresourceRange(vk::ImageSubresourceRange(
                                capturedAspectMask, 0, capturedMipLevels, 0, capturedArrayLayers));
                            finalBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                            finalBarrier.setDstAccessMask(capturedFinalAccessMask);

                            vk::PipelineStageFlags finalStage;
                            if (capturedAspectMask & vk::ImageAspectFlagBits::eDepth ||
                                capturedAspectMask & vk::ImageAspectFlagBits::eStencil)
                            {
                                finalStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                            }
                            else if (capturedFinalLayout == vk::ImageLayout::ePresentSrcKHR)
                            {
                                finalStage = vk::PipelineStageFlagBits::eBottomOfPipe;
                            }
                            else
                            {
                                finalStage = vk::PipelineStageFlagBits::eAllCommands;
                            }

                            graphicsCmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, finalStage,
                                                              vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1,
                                                              &finalBarrier);

                            endSingleTimeCommands(graphicsCmdBuffer);
                        }

                        if (capturedBuffer)
                        {
                            m_device.destroyBuffer(capturedBuffer);
                        }
                        if (capturedMemory)
                        {
                            m_device.freeMemory(capturedMemory);
                        }
                    };

                    if (outFence)
                    {
                        *outFence = fence;
                    }
                }
                else
                {
                    ferr << "callback is nullptr, resource is leaked in transferDataToImage" << std::endl;
                }
            }
            else
            {
                endSingleTimeTransferCommands(cmdBuffer, nullptr);

                if (needsQueueTransfer)
                {
                    vk::CommandBuffer graphicsCmdBuffer = beginSingleTimeCommands();

                    vk::ImageMemoryBarrier finalBarrier;
                    finalBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
                    finalBarrier.setNewLayout(finalLayout);
                    finalBarrier.setSrcQueueFamilyIndex(m_transferQueueFamilyIndex);
                    finalBarrier.setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex);
                    finalBarrier.setImage(dstImage);
                    finalBarrier.setSubresourceRange(
                        vk::ImageSubresourceRange(aspectMask, 0, mipLevels, 0, arrayLayers));
                    finalBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                    finalBarrier.setDstAccessMask(finalAccessMask);

                    vk::PipelineStageFlags finalStage;
                    if (aspectMask & vk::ImageAspectFlagBits::eDepth || aspectMask & vk::ImageAspectFlagBits::eStencil)
                    {
                        finalStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                    }
                    else if (finalLayout == vk::ImageLayout::ePresentSrcKHR)
                    {
                        finalStage = vk::PipelineStageFlagBits::eBottomOfPipe;
                    }
                    else
                    {
                        finalStage = vk::PipelineStageFlagBits::eAllCommands;
                    }

                    graphicsCmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, finalStage,
                                                      vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &finalBarrier);

                    endSingleTimeCommands(graphicsCmdBuffer);
                }

                if (!isCachedStaging)
                {
                    m_device.destroyBuffer(stagingBuffer);
                    m_device.freeMemory(stagingMemory);
                }
            }
        }
        catch (const std::exception& e)
        {
            ferr << "Failed to transfer data to image: " << e.what() << std::endl;

            if (stagingBuffer && !isCachedStaging)
            {
                m_device.destroyBuffer(stagingBuffer);
            }

            if (stagingMemory && !isCachedStaging)
            {
                m_device.freeMemory(stagingMemory);
            }

            throw;
        }
    }
    void VK_Context::createStageBuffer()
    {
        ZoneScoped();
        if (m_device) {
            m_device.waitIdle();
        }

        if (m_stageMappedPtr) {
            m_device.unmapMemory(m_stageMemory);
            m_stageMappedPtr = nullptr;
        }
        if (m_stageBuffer) {
            m_device.destroyBuffer(m_stageBuffer);
            m_stageBuffer = nullptr;
        }
        if (m_stageMemory) {
            m_device.freeMemory(m_stageMemory);
            m_stageMemory = nullptr;
        }
        
        m_stageHead = 0;
        for(auto& upload : m_inFlightUploads) {
             if(upload.fence) m_device.destroyFence(upload.fence);
             if(upload.onCompletion) upload.onCompletion();
        }
        m_inFlightUploads.clear();

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.setSize(m_maxStageBufferSize);
        bufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        bufferInfo.setSharingMode(vk::SharingMode::eExclusive);

        try {
            m_stageBuffer = m_device.createBuffer(bufferInfo);

            vk::MemoryRequirements memRequirements = m_device.getBufferMemoryRequirements(m_stageBuffer);

            vk::MemoryAllocateInfo allocInfo;
            allocInfo.setAllocationSize(memRequirements.size);
            allocInfo.setMemoryTypeIndex(findMemoryType(
                memRequirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

            m_stageMemory = m_device.allocateMemory(allocInfo);
            m_device.bindBufferMemory(m_stageBuffer, m_stageMemory, 0);

            void* mapped = m_device.mapMemory(m_stageMemory, 0, m_maxStageBufferSize);
            m_stageMappedPtr = static_cast<uint8_t*>(mapped);
        }
        catch (const std::exception& e) {
            ferr << "Failed to create Staging Buffer: " << e.what() << std::endl;
            if (m_stageBuffer) m_device.destroyBuffer(m_stageBuffer);
            if (m_stageMemory) m_device.freeMemory(m_stageMemory);
            m_stageBuffer = nullptr;
            m_stageMemory = nullptr;
            m_stageMappedPtr = nullptr;
        }
    }

    void VK_Context::processInFlightUploads()
    {
        auto it = m_inFlightUploads.begin();
        while (it != m_inFlightUploads.end()) {
            if (m_device.getFenceStatus(it->fence) == vk::Result::eSuccess) {
                ZoneScopedN("CompleteInFlightUpload");
                m_device.destroyFence(it->fence);
                if (it->cmdBuffer) {
                    m_device.freeCommandBuffers(m_asyncTransferCmdPool, 1, &it->cmdBuffer);
                }
                if (it->onCompletion) {
                    it->onCompletion();
                }
                it = m_inFlightUploads.erase(it);
            } else {
                break;
            }
        }
    }

    size_t VK_Context::allocateStagingMemory(size_t size)
    {
        ZoneScoped();
        if (size > m_maxStageBufferSize) {
            return SIZE_MAX;
        }

        processInFlightUploads();

        size_t align = 16;
        size_t alignedHead = (m_stageHead + align - 1) & ~(align - 1);
        
        if (alignedHead + size > m_maxStageBufferSize) {
            alignedHead = 0;
        }

        auto it = m_inFlightUploads.begin();
        while (it != m_inFlightUploads.end()) {
            bool overlap = (std::max(alignedHead, it->offset) < std::min(alignedHead + size, it->offset + it->size));
            
            if (overlap) {
                if (m_device.waitForFences(1, &it->fence, VK_TRUE, UINT64_MAX) == vk::Result::eSuccess) {
                     m_device.destroyFence(it->fence);
                     if (it->cmdBuffer) {
                        m_device.freeCommandBuffers(m_asyncTransferCmdPool, 1, &it->cmdBuffer);
                     }
                     if (it->onCompletion) {
                        it->onCompletion();
                     }
                     it = m_inFlightUploads.erase(it);
                     continue;
                }
            }
            ++it;
        }

        m_stageHead = alignedHead + size;
        return alignedHead;
    }
    void VK_Context::transferThreadLoop()
    {
        tracy::SetThreadName("VK Async Transfer Thread");
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.setQueueFamilyIndex(m_transferQueueFamilyIndex)
                .setFlags(vk::CommandPoolCreateFlagBits::eTransient |
                          vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        m_asyncTransferCmdPool = m_device.createCommandPool(poolInfo);

        createStageBuffer();
        
        while (m_transferRunning) {
             processInFlightUploads();
             TransferTask* task = nullptr;
             if (m_asyncTransferTaskQueue.pop(task)) {
                 if (task) {
                     ZoneScopedN("AsyncTransferTask");
                     task->task();
                     delete task;
                 }
             } else {
                 std::this_thread::yield();
             }
        }

        if (m_stageMappedPtr) m_device.unmapMemory(m_stageMemory);
        if (m_stageBuffer) m_device.destroyBuffer(m_stageBuffer);
        if (m_stageMemory) m_device.freeMemory(m_stageMemory);

        m_device.destroyCommandPool(m_asyncTransferCmdPool);
        
        Context::transferThreadLoop();
    }

    void VK_Context::executeAsyncTransfer(AsyncTransferParams params)
    {
        ZoneScoped();
        size_t dataSize = params.data.size();
        size_t offset = allocateStagingMemory(dataSize);

        if (offset == SIZE_MAX) {
            ferr << "Async Transfer: Data size " << dataSize << " exceeds staging buffer capacity." << std::endl;
            bcpp::this_contract::release();
            return;
        }

        memcpy(m_stageMappedPtr + offset, params.data.data(), dataSize);
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        allocInfo.setCommandPool(m_asyncTransferCmdPool);
        allocInfo.setCommandBufferCount(1);

        vk::CommandBuffer cmdBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmdBuffer.begin(beginInfo);

        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(vk::ImageLayout::eUndefined);
        barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(params.dstImage);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(params.aspectMask, 0, params.mipLevels, 0, params.arrayLayers));
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags(),
            0, nullptr,
            0, nullptr,
            1, &barrier);

        vk::BufferImageCopy region;
        region.setBufferOffset(offset);
        region.setBufferRowLength(0);
        region.setBufferImageHeight(0);
        region.setImageSubresource(vk::ImageSubresourceLayers(params.aspectMask, 0, 0, params.arrayLayers));
        region.setImageOffset(vk::Offset3D(0, 0, 0));
        region.setImageExtent(vk::Extent3D(params.width, params.height, params.depth));

        cmdBuffer.copyBufferToImage(
            m_stageBuffer,
            params.dstImage,
            vk::ImageLayout::eTransferDstOptimal,
            1, &region);

        bool isDepth = (params.aspectMask & vk::ImageAspectFlagBits::eDepth) || (params.aspectMask & vk::ImageAspectFlagBits::eStencil);
        vk::ImageLayout finalLayout = isDepth ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
        vk::AccessFlags finalAccess = isDepth ? (vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite) : vk::AccessFlagBits::eShaderRead;
        bool needsQueueTransfer = m_transferQueueFamilyIndex != m_graphicsQueueFamilyIndex;

        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);

        if (needsQueueTransfer) {
            barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
            barrier.setSrcQueueFamilyIndex(m_transferQueueFamilyIndex);
            barrier.setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex);
            barrier.setDstAccessMask(vk::AccessFlagBits::eNone);
        } else {
            barrier.setNewLayout(finalLayout);
            barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setDstAccessMask(finalAccess);
        }

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            needsQueueTransfer ? vk::PipelineStageFlagBits::eBottomOfPipe : (isDepth ? vk::PipelineStageFlagBits::eEarlyFragmentTests : vk::PipelineStageFlagBits::eFragmentShader),
            vk::DependencyFlags(),
            0, nullptr,
            0, nullptr,
            1, &barrier);

        cmdBuffer.end();

        vk::FenceCreateInfo fenceInfo;
        vk::Fence fence = m_device.createFence(fenceInfo);

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&cmdBuffer);

        m_transferQueue.submit(1, &submitInfo, fence);

        InFlightUpload uploadInfo;
        uploadInfo.offset = offset;
        uploadInfo.size = dataSize;
        uploadInfo.fence = fence;
        uploadInfo.cmdBuffer = cmdBuffer;
        uploadInfo.onCompletion = params.onCompletion;
        m_inFlightUploads.push_back(uploadInfo);
    }

    void VK_Context::asyncTransferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height, uint32_t depth, vk::Format format,
                                      uint32_t mipLevels, uint32_t arrayLayers, vk::ImageAspectFlags aspectMask,
                                      std::vector<uint8_t> data,
                                      std::function<void()> onCompletion)
    {
        ZoneScoped();
        AsyncTransferParams params{
            dstImage, width, height, depth, format, mipLevels, arrayLayers, aspectMask,
            std::move(data), onCompletion
        };
        auto paramsPtr = std::make_shared<AsyncTransferParams>(std::move(params));

        auto transferTask = [this, paramsPtr]() mutable -> void {
            if (paramsPtr) {
                this->executeAsyncTransfer(std::move(*paramsPtr));
            }
        };
        
        TransferTask* task = new TransferTask{ transferTask };
        while (!m_asyncTransferTaskQueue.push(task)) {
             std::this_thread::yield();
        }
    }


    void VK_Context::acquireImageOwnership(Image* image)
    {
        ZoneScoped();
        if (m_transferQueueFamilyIndex == m_graphicsQueueFamilyIndex) {
            return;
        }

        RHI::Image* rhiImage = image->currentImage();
        RHI::VK_Image* vkImageObj = dynamic_cast<RHI::VK_Image*>(rhiImage);
        if (!vkImageObj) {
            ferr << "acquireImageOwnership: Not a VK_Image" << std::endl;
            return;
        }

        vk::Image dstImage = vkImageObj->getVkImage();

        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;

        vk::CommandBuffer cmdBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);

        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        
        barrier.setSrcQueueFamilyIndex(m_transferQueueFamilyIndex);
        barrier.setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex);
        barrier.setImage(dstImage);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, image->arrayLayers()));
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(),
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(cmdBuffer);
    }

    void VK_Context::createPlatform(ContextCreateFlags flag)
    {
        ZoneScoped();
        auto queueFamily = m_phyDevice.getQueueFamilyProperties();

        m_transferQueueFamilyIndex = UINT32_MAX;
        for (size_t i = 0; i < queueFamily.size(); ++i) {
            if (queueFamily[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                m_graphicsQueueFamilyIndex = i;
            }

            if (queueFamily[i].queueFlags & vk::QueueFlagBits::eTransfer) {
                if (!(queueFamily[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    !(queueFamily[i].queueFlags & vk::QueueFlagBits::eCompute)) {
                    m_transferQueueFamilyIndex = i;
                }
                else if (m_transferQueueFamilyIndex == UINT32_MAX) {
                    m_transferQueueFamilyIndex = i;
                }
            }
        }

        if (m_transferQueueFamilyIndex == UINT32_MAX) {
            m_transferQueueFamilyIndex = m_graphicsQueueFamilyIndex;
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {m_graphicsQueueFamilyIndex, m_transferQueueFamilyIndex};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.setQueueFamilyIndex(queueFamily)
                           .setQueueCount(1)
                           .setPQueuePriorities(&queuePriority);
            queueCreateInfos.push_back(queueCreateInfo);
        }

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        
        vk::PhysicalDeviceFeatures supportedFeatures = m_phyDevice.getFeatures();
        vk::PhysicalDeviceFeatures deviceFeatures;
        deviceFeatures.setSamplerAnisotropy(supportedFeatures.samplerAnisotropy);

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setQueueCreateInfos(queueCreateInfos)
                        .setPEnabledExtensionNames(deviceExtensions)
                        .setPEnabledFeatures(&deviceFeatures);

        vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{};
        bool enableBindless = (flag & ContextCreateFlag::enableBindless) && m_bindlessSupported;
        
        if (enableBindless) {
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

            descriptorIndexingFeatures.setPNext(nullptr); 
            descriptorIndexingFeatures.setShaderSampledImageArrayNonUniformIndexing(true);
            descriptorIndexingFeatures.setDescriptorBindingVariableDescriptorCount(true);
            descriptorIndexingFeatures.setDescriptorBindingPartiallyBound(true);
            descriptorIndexingFeatures.setRuntimeDescriptorArray(true);
            
            deviceCreateInfo.setPNext(&descriptorIndexingFeatures);
        }

        m_device = m_phyDevice.createDevice(deviceCreateInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

        m_graphicsQueue = m_device.getQueue(m_graphicsQueueFamilyIndex, 0);
        m_transferQueue = m_device.getQueue(m_transferQueueFamilyIndex, 0);

        createCommandPoolAndBuffers();
        createTransferCommandPool();

        m_bindlessSupported = enableBindless;
        if (!m_bindlessSupported && (flag & ContextCreateFlag::enableBindless)) {
            ferr << "Warning: Bindless requested but not supported by device." << std::endl;
        }

        m_transferRunning = true;
        m_transferThread = std::thread([this] {
            transferThreadLoop();
        });
    }

    uint32_t VK_Context::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memProperties = getPhysicalDevice().getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
                }
        }
    }

    vk::Instance VK_Context::getVkInstance() {
        return m_common->getInstance();
    }

    void VK_Context::beginCommandBuffer(int index) {
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        m_commandBuffers[index].begin(beginInfo);
    }

    void VK_Context::endCommandBuffer(int index) {
        m_commandBuffers[index].end();
    }
    void VK_Context::submitCommandBuffer() {
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(m_commandBuffers[0]);

        m_graphicsQueue.submit(submitInfo);
        m_graphicsQueue.waitIdle();
    }

    bool VK_Context::isIntegrateGpu() {
        if (!m_phyDevice) {
            return false;
        }

        vk::PhysicalDeviceProperties properties = m_phyDevice.getProperties();
        return properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
    }
}

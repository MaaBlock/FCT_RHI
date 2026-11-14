#ifndef VK_CONTEXT_H
#define VK_CONTEXT_H
#include "../ThirdParty.h"
#include "./Context.h"

namespace FCT {
    class VK_ContextCommon;
    
    struct AsyncTransferParams {
        vk::Image dstImage;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        vk::Format format;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        vk::ImageAspectFlags aspectMask;
        std::vector<uint8_t> data;
        std::function<void()> onCompletion;
    };

    class VK_Context : public Context {
    public:
        VK_Context(VK_ContextCommon* common);
        ~VK_Context() override;
        void waitGpu() override;
        void cleanupGpuResources() override;
        void unbindWindow(Window* wnd) override;
        RHI::RasterizationPipeline* createTraditionPipeline() override;
        UniquePtr<RHI::Pipeline> createRhiPipeline(PipelineType type) override;
        bool isIntegrateGpu() override;
        bool isRayTracingSupported() const noexcept override { return m_rayTracingSupported; }
        bool isMeshShaderSupported() const noexcept override { return false; }
        bool isComputeSupported() const noexcept override { return true; }
        bool isBindlessSupported() const noexcept override { return m_bindlessSupported; }
        bool isReadWriteStorageBufferSupported() const noexcept override { return true; }
        bool isIndirectDrawSupported() const noexcept override { return true; }
        bool isBaseInstanceSupported() const noexcept override { return true; }
        uint32_t getUniformBufferOffsetAlignment() const noexcept override;
        uint32_t getMaxUniformBufferRange() const noexcept override;
        void setBindlessSetLayout(vk::DescriptorSetLayout layout) { m_bindlessSetLayout = layout; }
        vk::DescriptorSetLayout getBindlessSetLayout() const { return m_bindlessSetLayout; }
        bool isAccelerationStructureSupported() const noexcept { return m_accelerationStructureSupported; }

        const vk::PhysicalDeviceRayTracingPipelinePropertiesKHR& getRayTracingPipelineProperties() const noexcept
        {
            return m_rtPipelineProperties;
        }
        const vk::PhysicalDeviceAccelerationStructurePropertiesKHR& getAccelerationStructureProperties() const noexcept
        {
            return m_asProperties;
        }
        //void create(IRenderTarget* target) override;
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        void createPlatform(ContextCreateFlags flag) override;
        auto device()
        {
            return m_device;
        }
        auto getDevice() const {
            return m_device;
        }
        auto getPhysicalDevice() const {
            return m_phyDevice;
        }

        vk::Instance getVkInstance();
        void beginCommandBuffer(int index);
        void endCommandBuffer(int index);
        void submitCommandBuffer();

        void clear(Vec4 color,float depth = 1.0,float stencil = 0.0){

        }
        uint32_t getGraphicsQueueFamily() const;
        void createTransferCommandPool();
        vk::CommandBuffer beginSingleTimeTransferCommands();
        void endSingleTimeTransferCommands(vk::CommandBuffer commandBuffer, vk::Fence* outFence);

        vk::Queue getGraphicsQueue()
        {
            return m_graphicsQueue;
        }

        RHI::ConstBuffer* getEmptyConstBuffer(const ConstLayout& layout) override;
        Sampler* getEmptySampler() override;

        vk::CommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::CommandBuffer commandBuffer);
        void transferDataToBuffer(vk::Buffer dstBuffer, size_t size, const void* data);
        void transferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height, vk::Format format,
                                 const void* data,
                                 size_t dataSize, vk::Fence* outFence = nullptr, std::function<void()>* cleanUpCallback = nullptr);
        void transferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height, uint32_t depth, vk::Format format,
                                 uint32_t mipLevels, uint32_t arrayLayers, vk::ImageAspectFlags aspectMask,
                                 const void* data,
                                 size_t dataSize, vk::Fence* outFence = nullptr, std::function<void()>* cleanUpCallback = nullptr,
                                 vk::ImageLayout finalLayout = vk::ImageLayout::eUndefined);

        void asyncTransferDataToImage(vk::Image dstImage, uint32_t width, uint32_t height, uint32_t depth, vk::Format format,
                                      uint32_t mipLevels, uint32_t arrayLayers, vk::ImageAspectFlags aspectMask,
                                      std::vector<uint8_t> data,
                                      std::function<void()> onCompletion);
                                      
        void acquireImageOwnership(Image* image) override;
        bool isBufferDeviceAddressSupported() const noexcept { return m_bufferDeviceAddressSupported; }
        RHI::StorageBuffer* createStorageBuffer(size_t size = 0) override;

    private:
        vk::Buffer m_stageBuffer;
        vk::DeviceMemory m_stageMemory;
        uint8_t* m_stageMappedPtr = nullptr;
        size_t m_stageHead = 0;
#ifdef FCT_ANDROID
        size_t m_maxStageBufferSize = 64 * 1024 * 1024; // 64MB
#else
        size_t m_maxStageBufferSize = 128 * 1024 * 1024; // 128MB
#endif
        struct InFlightUpload {
            size_t offset;
            size_t size;
            vk::Fence fence;
            vk::CommandBuffer cmdBuffer;
            std::function<void()> onCompletion;
        };
        std::deque<InFlightUpload> m_inFlightUploads;

        struct TransferTask {
            std::function<void()> task;
        };
        boost::lockfree::queue<TransferTask*, boost::lockfree::capacity<1024>> m_asyncTransferTaskQueue;
    private:
        void processInFlightUploads();
        void createStageBuffer();
        void transferThreadLoop() override;
        size_t allocateStagingMemory(size_t size);
        void executeAsyncTransfer(AsyncTransferParams params);
    private:
        void createCommandPoolAndBuffers();


        VK_ContextCommon* m_common;
        uint32_t m_graphicsQueueFamilyIndex;
        vk::Device m_device;
        vk::PhysicalDevice m_phyDevice;
        vk::Queue m_graphicsQueue;
        vk::CommandPool m_commandPool;
        std::vector<vk::CommandBuffer> m_commandBuffers;

        uint32_t m_transferQueueFamilyIndex;
        vk::Queue m_transferQueue;
        vk::CommandPool m_transferCommandPool;
        vk::CommandPool m_asyncTransferCmdPool;
        std::unordered_map<size_t, std::unique_ptr<RHI::ConstBuffer>> m_emptyConstBufferCache;
        std::unique_ptr<Sampler> m_emptySampler;
        bool m_bindlessSupported = false;
        bool m_bufferDeviceAddressSupported = false;
        bool m_rayTracingSupported = false;
        bool m_accelerationStructureSupported = false;
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rtPipelineProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_asProperties{};
        vk::Buffer m_syncStagingBuffer;
        vk::DeviceMemory m_syncStagingMemory;
        size_t m_syncStagingBufferSize = 0;
        vk::DescriptorSetLayout m_bindlessSetLayout = nullptr;
    };
}

#endif //VK_CONTEXT_H

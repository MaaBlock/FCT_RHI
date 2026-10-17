#ifndef FCT_D3D12_CONTEXT_H
#define FCT_D3D12_CONTEXT_H

#include "../ThirdParty.h"
#include "./Context.h"
#include "./D3D12_ContextCommon.h"

namespace FCT
{
    namespace RHI
    {
        class D3D12_Image;
        class D3D12_Fence;
        class D3D12_DescriptorPool;
    }

    struct D3D12_CpuDescriptorHeap
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{0};
        uint32_t descriptorSize = 0;
        uint32_t capacity = 0;
        uint32_t nextFreeIndex = 0;
        std::vector<uint32_t> freeList;
        std::mutex mutex;

        void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxDescriptors);
        void destroy();
        uint32_t alloc(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle);
        void free(uint32_t index);
    };

    class D3D12_Context : public Context
    {
    public:
        explicit D3D12_Context(D3D12_ContextCommon* common);
        ~D3D12_Context() override;

        void createPlatform(ContextCreateFlags flag) override;
        bool isIntegrateGpu() override;
        bool isBindlessSupported() const override { return true; }

        RHI::RasterizationPipeline* createTraditionPipeline() override;
        RHI::ConstBuffer* getEmptyConstBuffer(const ConstLayout& layout) override;
        Sampler* getEmptySampler() override;

        ID3D12Device* getDevice() const { return m_device.Get(); }
        IDXGIAdapter1* getAdapter() const { return m_common ? m_common->getAdapter() : nullptr; }
        IDXGIFactory6* getFactory() const { return m_common ? m_common->getFactory() : nullptr; }
        ID3D12CommandQueue* getGraphicsQueue() const { return m_graphicsQueue.Get(); }
        ID3D12CommandQueue* getTransferQueue() const { return m_transferQueue ? m_transferQueue.Get() : m_graphicsQueue.Get(); }

        uint32_t allocRtvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle);
        void freeRtvDescriptor(uint32_t index);

        uint32_t allocDsvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle);
        void freeDsvDescriptor(uint32_t index);

        uint32_t allocStagingCbvSrvUavDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle);
        void freeStagingCbvSrvUavDescriptor(uint32_t index);

        uint32_t allocStagingSamplerDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle);
        void freeStagingSamplerDescriptor(uint32_t index);

        void beginSingleTimeCommands(const std::function<void(ID3D12GraphicsCommandList*)>& commandFunction);

        void transferDataToBuffer(
            ID3D12Resource* dstBuffer,
            size_t size,
            const void* data,
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_GENERIC_READ);

        void transferDataToImage(
            ID3D12Resource* dstImage,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            Format format,
            const void* data,
            size_t dataSize,
            D3D12_RESOURCE_STATES* currentState = nullptr,
            uint32_t mipLevels = 1,
            uint32_t arrayLayers = 1);

        void transferDataToImage(
            ID3D12Resource* dstImage,
            uint32_t width,
            uint32_t height,
            Format format,
            const void* data,
            size_t dataSize,
            D3D12_RESOURCE_STATES* currentState = nullptr);

        void asyncTransferDataToImage(
            ID3D12Resource* dstImage,
            uint32_t width,
            uint32_t height,
            Format format,
            const void* data,
            size_t dataSize,
            D3D12_RESOURCE_STATES* currentState = nullptr,
            std::function<void()> callback = nullptr);

        void waitForGpuIdle();

    private:
        void createDeviceAndQueues();
        void createCpuDescriptorHeaps();

    private:
        D3D12_ContextCommon* m_common;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_transferQueue;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_uploadFence;
        HANDLE m_uploadEvent;
        uint64_t m_uploadFenceValue;
        std::mutex m_uploadMutex;

        D3D12_CpuDescriptorHeap m_rtvHeap;
        D3D12_CpuDescriptorHeap m_dsvHeap;
        D3D12_CpuDescriptorHeap m_stagingCbvSrvUavHeap;
        D3D12_CpuDescriptorHeap m_stagingSamplerHeap;

        std::unordered_map<size_t, RHI::ConstBuffer*> m_emptyConstBuffers;
        Sampler* m_emptySampler;
    };
}

#endif // FCT_D3D12_CONTEXT_H

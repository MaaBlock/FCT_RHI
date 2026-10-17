#include "../FCTAPI.h"
#include "./D3D12_Context.h"
#include "./D3D12_Device.h"
#include "./D3D12_ShaderCompiler.h"
#include "../RHI/D3D12_TraditionalPipeline.h"
#include "../RHI/D3D12_ConstBuffer.h"
#include "../RHI/D3D12_Sampler.h"

namespace FCT
{
    void D3D12_CpuDescriptorHeap::init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxDescriptors)
    {
        destroy();
        capacity = maxDescriptors;
        nextFreeIndex = 0;

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type = type;
        desc.NumDescriptors = maxDescriptors;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
        {
            ferr << "Failed to create D3D12 CPU descriptor heap, hr = " << std::hex << hr << std::dec << std::endl;
            return;
        }
        cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
        descriptorSize = device->GetDescriptorHandleIncrementSize(type);
    }

    void D3D12_CpuDescriptorHeap::destroy()
    {
        std::lock_guard<std::mutex> lock(mutex);
        heap.Reset();
        cpuStart = {0};
        descriptorSize = 0;
        capacity = 0;
        nextFreeIndex = 0;
        freeList.clear();
    }

    uint32_t D3D12_CpuDescriptorHeap::alloc(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle)
    {
        std::lock_guard<std::mutex> lock(mutex);
        uint32_t index = UINT32_MAX;
        if (!freeList.empty())
        {
            index = freeList.back();
            freeList.pop_back();
        }
        else if (nextFreeIndex < capacity)
        {
            index = nextFreeIndex++;
        }
        else
        {
            ferr << "D3D12 CPU descriptor heap exhausted!" << std::endl;
            outHandle = {0};
            return UINT32_MAX;
        }

        outHandle = cpuStart;
        outHandle.ptr += static_cast<SIZE_T>(index) * descriptorSize;
        return index;
    }

    void D3D12_CpuDescriptorHeap::free(uint32_t index)
    {
        if (index == UINT32_MAX)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex);
        freeList.push_back(index);
    }

    D3D12_Context::D3D12_Context(D3D12_ContextCommon* common)
        : Context(common->runtime())
        , m_common(common)
        , m_uploadEvent(nullptr)
        , m_uploadFenceValue(0)
        , m_emptySampler(nullptr)
    {
        FCT_SAFE_DELETE(m_compiler);
        m_compiler = new D3D12_ShaderCompiler();
        m_resourceDevice = new D3D12_Device(this);
    }

    D3D12_Context::~D3D12_Context()
    {
        waitForGpuIdle();

        for (auto& [layout, cb] : m_emptyConstBuffers)
        {
            delete cb;
        }
        m_emptyConstBuffers.clear();

        if (m_emptySampler)
        {
            m_emptySampler->release();
            m_emptySampler = nullptr;
        }

        if (m_uploadEvent)
        {
            CloseHandle(m_uploadEvent);
            m_uploadEvent = nullptr;
        }
        m_uploadFence.Reset();

        m_rtvHeap.destroy();
        m_dsvHeap.destroy();
        m_stagingCbvSrvUavHeap.destroy();
        m_stagingSamplerHeap.destroy();

        m_transferQueue.Reset();
        m_graphicsQueue.Reset();
        m_device.Reset();
    }

    void D3D12_Context::createDeviceAndQueues()
    {
        IDXGIAdapter1* adapter = getAdapter();
        HRESULT hr = D3D12CreateDevice(
            adapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device));
        if (FAILED(hr))
        {
            ferr << "Failed to create D3D12Device, hr = " << std::hex << hr << std::dec << std::endl;
            return;
        }

        D3D12_COMMAND_QUEUE_DESC graphicsQueueDesc{};
        graphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        graphicsQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        graphicsQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        graphicsQueueDesc.NodeMask = 0;

        hr = m_device->CreateCommandQueue(&graphicsQueueDesc, IID_PPV_ARGS(&m_graphicsQueue));
        if (FAILED(hr))
        {
            ferr << "Failed to create D3D12 Graphics CommandQueue, hr = " << std::hex << hr << std::dec << std::endl;
        }

        D3D12_COMMAND_QUEUE_DESC copyQueueDesc{};
        copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        copyQueueDesc.NodeMask = 0;
        m_device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&m_transferQueue));

        m_uploadFenceValue = 0;
        m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_uploadFence));
        m_uploadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    void D3D12_Context::createCpuDescriptorHeaps()
    {
        if (!m_device)
        {
            return;
        }
        m_rtvHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048);
        m_dsvHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024);
        m_stagingCbvSrvUavHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384);
        m_stagingSamplerHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024);
    }

    void D3D12_Context::createPlatform(ContextCreateFlags flag)
    {
        createDeviceAndQueues();
        createCpuDescriptorHeaps();
    }

    bool D3D12_Context::isIntegrateGpu()
    {
        if (!m_device)
        {
            return false;
        }
        D3D12_FEATURE_DATA_ARCHITECTURE arch{};
        arch.NodeIndex = 0;
        if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch))))
        {
            return arch.UMA == TRUE;
        }
        return false;
    }

    RHI::RasterizationPipeline* D3D12_Context::createTraditionPipeline()
    {
        return new RHI::D3D12_TraditionalPipeline(this);
    }

    RHI::ConstBuffer* D3D12_Context::getEmptyConstBuffer(const ConstLayout& layout)
    {
        size_t sizeKey = layout.getTotalSize();
        auto it = m_emptyConstBuffers.find(sizeKey);
        if (it != m_emptyConstBuffers.end() && it->second)
        {
            return it->second;
        }

        auto* rhiCb = new RHI::D3D12_ConstBuffer(this);
        rhiCb->layout(layout);
        rhiCb->create();
        m_emptyConstBuffers[sizeKey] = rhiCb;
        return rhiCb;
    }

    Sampler* D3D12_Context::getEmptySampler()
    {
        if (!m_emptySampler)
        {
            m_emptySampler = new D3D12_Sampler(this);
            m_emptySampler->setLinear();
            m_emptySampler->create();
        }
        return m_emptySampler;
    }

    uint32_t D3D12_Context::allocRtvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle)
    {
        return m_rtvHeap.alloc(outHandle);
    }

    void D3D12_Context::freeRtvDescriptor(uint32_t index)
    {
        m_rtvHeap.free(index);
    }

    uint32_t D3D12_Context::allocDsvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle)
    {
        return m_dsvHeap.alloc(outHandle);
    }

    void D3D12_Context::freeDsvDescriptor(uint32_t index)
    {
        m_dsvHeap.free(index);
    }

    uint32_t D3D12_Context::allocStagingCbvSrvUavDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle)
    {
        return m_stagingCbvSrvUavHeap.alloc(outHandle);
    }

    void D3D12_Context::freeStagingCbvSrvUavDescriptor(uint32_t index)
    {
        m_stagingCbvSrvUavHeap.free(index);
    }

    uint32_t D3D12_Context::allocStagingSamplerDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle)
    {
        return m_stagingSamplerHeap.alloc(outHandle);
    }

    void D3D12_Context::freeStagingSamplerDescriptor(uint32_t index)
    {
        m_stagingSamplerHeap.free(index);
    }

    void D3D12_Context::waitForGpuIdle()
    {
        if (!m_graphicsQueue || !m_uploadFence || !m_uploadEvent)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        const uint64_t fenceVal = ++m_uploadFenceValue;
        m_graphicsQueue->Signal(m_uploadFence.Get(), fenceVal);
        if (m_uploadFence->GetCompletedValue() < fenceVal)
        {
            m_uploadFence->SetEventOnCompletion(fenceVal, m_uploadEvent);
            WaitForSingleObject(m_uploadEvent, INFINITE);
        }
    }

    void D3D12_Context::beginSingleTimeCommands(const std::function<void(ID3D12GraphicsCommandList*)>& commandFunction)
    {
        if (!m_device || !m_graphicsQueue || !commandFunction)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_uploadMutex);

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;

        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
        {
            return;
        }
        if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList))))
        {
            return;
        }

        commandFunction(cmdList.Get());
        cmdList->Close();

        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_graphicsQueue->ExecuteCommandLists(1, ppCommandLists);

        const uint64_t fenceVal = ++m_uploadFenceValue;
        m_graphicsQueue->Signal(m_uploadFence.Get(), fenceVal);
        if (m_uploadFence->GetCompletedValue() < fenceVal)
        {
            m_uploadFence->SetEventOnCompletion(fenceVal, m_uploadEvent);
            WaitForSingleObject(m_uploadEvent, INFINITE);
        }
    }

    void D3D12_Context::transferDataToBuffer(
        ID3D12Resource* dstBuffer,
        size_t size,
        const void* data,
        D3D12_RESOURCE_STATES initialState,
        D3D12_RESOURCE_STATES afterState)
    {
        if (!dstBuffer || !data || size == 0)
        {
            return;
        }

        D3D12_HEAP_PROPERTIES uploadHeapProps{};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC uploadBufferDesc{};
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Alignment = 0;
        uploadBufferDesc.Width = size;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.SampleDesc.Quality = 0;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        if (FAILED(m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer))))
        {
            return;
        }

        void* mapped = nullptr;
        D3D12_RANGE readRange{ 0, 0 };
        if (SUCCEEDED(uploadBuffer->Map(0, &readRange, &mapped)))
        {
            memcpy(mapped, data, size);
            uploadBuffer->Unmap(0, nullptr);
        }

        beginSingleTimeCommands([&](ID3D12GraphicsCommandList* cmdList)
        {
            if (initialState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                D3D12_RESOURCE_BARRIER toCopy{};
                toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                toCopy.Transition.pResource = dstBuffer;
                toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                toCopy.Transition.StateBefore = initialState;
                toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                cmdList->ResourceBarrier(1, &toCopy);
            }

            cmdList->CopyBufferRegion(dstBuffer, 0, uploadBuffer.Get(), 0, size);

            if (afterState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                D3D12_RESOURCE_BARRIER toAfter{};
                toAfter.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                toAfter.Transition.pResource = dstBuffer;
                toAfter.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                toAfter.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                toAfter.Transition.StateAfter = afterState;
                cmdList->ResourceBarrier(1, &toAfter);
            }
        });
    }

    void D3D12_Context::transferDataToImage(
        ID3D12Resource* dstImage,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        Format format,
        const void* data,
        size_t dataSize,
        D3D12_RESOURCE_STATES* currentState,
        uint32_t mipLevels,
        uint32_t arrayLayers)
    {
        if (!dstImage || !data || dataSize == 0 || !m_device)
        {
            return;
        }

        D3D12_RESOURCE_DESC dstDesc = dstImage->GetDesc();
        uint32_t numSubresources = (mipLevels > 0 ? mipLevels : 1) * (arrayLayers > 0 ? arrayLayers : 1);

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
        std::vector<UINT> numRows(numSubresources);
        std::vector<UINT64> rowSizesInBytes(numSubresources);
        UINT64 requiredSize = 0;

        m_device->GetCopyableFootprints(
            &dstDesc,
            0,
            numSubresources,
            0,
            footprints.data(),
            numRows.data(),
            rowSizesInBytes.data(),
            &requiredSize);

        D3D12_HEAP_PROPERTIES uploadHeapProps{};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC uploadBufferDesc{};
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Alignment = 0;
        uploadBufferDesc.Width = requiredSize;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.SampleDesc.Quality = 0;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        if (FAILED(m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer))))
        {
            return;
        }

        uint8_t* mappedPtr = nullptr;
        D3D12_RANGE readRange{ 0, 0 };
        if (SUCCEEDED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr))))
        {
            const uint8_t* srcBytes = static_cast<const uint8_t*>(data);
            size_t srcOffset = 0;

            for (uint32_t s = 0; s < numSubresources; ++s)
            {
                const auto& fp = footprints[s];
                UINT subRows = numRows[s];
                UINT subDepth = fp.Footprint.Depth;
                size_t rowCopyBytes = static_cast<size_t>(rowSizesInBytes[s]);

                uint8_t* dstSubPtr = mappedPtr + fp.Offset;
                size_t dstRowPitch = fp.Footprint.RowPitch;
                size_t dstSlicePitch = dstRowPitch * subRows;

                for (UINT z = 0; z < subDepth; ++z)
                {
                    uint8_t* dstSlicePtr = dstSubPtr + z * dstSlicePitch;
                    for (UINT y = 0; y < subRows; ++y)
                    {
                        if (srcOffset + rowCopyBytes <= dataSize)
                        {
                            memcpy(dstSlicePtr + y * dstRowPitch, srcBytes + srcOffset, rowCopyBytes);
                        }
                        srcOffset += rowCopyBytes;
                    }
                }
            }
            uploadBuffer->Unmap(0, nullptr);
        }

        D3D12_RESOURCE_STATES beforeState = currentState ? *currentState : D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_STATES targetAfterState =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        beginSingleTimeCommands([&](ID3D12GraphicsCommandList* cmdList)
        {
            if (beforeState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                D3D12_RESOURCE_BARRIER toCopy{};
                toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                toCopy.Transition.pResource = dstImage;
                toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                toCopy.Transition.StateBefore = beforeState;
                toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                cmdList->ResourceBarrier(1, &toCopy);
            }

            for (uint32_t s = 0; s < numSubresources; ++s)
            {
                D3D12_TEXTURE_COPY_LOCATION dstLoc{};
                dstLoc.pResource = dstImage;
                dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLoc.SubresourceIndex = s;

                D3D12_TEXTURE_COPY_LOCATION srcLoc{};
                srcLoc.pResource = uploadBuffer.Get();
                srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLoc.PlacedFootprint = footprints[s];

                cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            }

            D3D12_RESOURCE_BARRIER toShaderRead{};
            toShaderRead.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            toShaderRead.Transition.pResource = dstImage;
            toShaderRead.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            toShaderRead.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            toShaderRead.Transition.StateAfter = targetAfterState;
            cmdList->ResourceBarrier(1, &toShaderRead);
        });

        if (currentState)
        {
            *currentState = targetAfterState;
        }
    }

    void D3D12_Context::transferDataToImage(
        ID3D12Resource* dstImage,
        uint32_t width,
        uint32_t height,
        Format format,
        const void* data,
        size_t dataSize,
        D3D12_RESOURCE_STATES* currentState)
    {
        transferDataToImage(dstImage, width, height, 1, format, data, dataSize, currentState, 1, 1);
    }

    void D3D12_Context::asyncTransferDataToImage(
        ID3D12Resource* dstImage,
        uint32_t width,
        uint32_t height,
        Format format,
        const void* data,
        size_t dataSize,
        D3D12_RESOURCE_STATES* currentState,
        std::function<void()> callback)
    {
        transferDataToImage(dstImage, width, height, 1, format, data, dataSize, currentState, 1, 1);
        if (callback)
        {
            postLogicTask(callback);
        }
    }
}

#include "../FCTAPI.h"
#include "./D3D12_Image.h"
#include "./D3D12_Fence.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_Image::D3D12_Image(D3D12_Context* ctx)
            : Image()
            , m_ctx(ctx)
            , m_currentState(D3D12_RESOURCE_STATE_COMMON)
        {
        }

        D3D12_Image::~D3D12_Image()
        {
            m_resource.Reset();
        }

        D3D12_RESOURCE_FLAGS D3D12_Image::buildResourceFlags() const
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if (m_usage & ImageUsage::RenderTarget)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if (m_usage & ImageUsage::DepthStencil)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            return flags;
        }

        DXGI_FORMAT D3D12_Image::resolveResourceFormat() const
        {
            if (IsDepthFormat(m_format) && (m_usage & ImageUsage::Texture))
            {
                return ToDxgiTypelessFormat(m_format);
            }
            return ToDxgiFormat(m_format);
        }

        D3D12_RESOURCE_STATES D3D12_Image::resolveInitialState() const
        {
            if (m_initData.data && m_initData.size > 0)
            {
                return D3D12_RESOURCE_STATE_COMMON;
            }
            if (m_usage & ImageUsage::DepthStencil)
            {
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }
            if (m_usage & ImageUsage::RenderTarget)
            {
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            if (m_usage & ImageUsage::Texture)
            {
                return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            return D3D12_RESOURCE_STATE_COMMON;
        }

        void D3D12_Image::create()
        {
            create(true);
        }

        void D3D12_Image::create(bool uploadData)
        {
            m_resource.Reset();

            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = m_width > 0 ? m_width : 1;
            desc.Height = m_height > 0 ? m_height : 1;
            desc.DepthOrArraySize = static_cast<UINT16>(m_arrayLayers > 0 ? m_arrayLayers : 1);
            desc.MipLevels = 1;
            desc.Format = resolveResourceFormat();
            desc.SampleDesc.Count = ToDxgiSampleCount(m_samples);
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = buildResourceFlags();

            D3D12_CLEAR_VALUE clearValue{};
            D3D12_CLEAR_VALUE* pClearValue = nullptr;
            if (m_usage & ImageUsage::DepthStencil)
            {
                clearValue.Format = ToDxgiDsvFormat(m_format);
                clearValue.DepthStencil.Depth = 1.0f;
                clearValue.DepthStencil.Stencil = 0;
                pClearValue = &clearValue;
            }
            else if (m_usage & ImageUsage::RenderTarget)
            {
                clearValue.Format = ToDxgiFormat(m_format);
                clearValue.Color[0] = 0.0f;
                clearValue.Color[1] = 0.0f;
                clearValue.Color[2] = 0.0f;
                clearValue.Color[3] = 1.0f;
                pClearValue = &clearValue;
            }

            m_currentState = resolveInitialState();

            HRESULT hr = m_ctx->getDevice()->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                m_currentState,
                pClearValue,
                IID_PPV_ARGS(&m_resource));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 Image, hr = " << std::hex << hr << std::dec << std::endl;
                return;
            }

            if (uploadData && m_initData.data && m_initData.size > 0)
            {
                m_ctx->transferDataToImage(
                    m_resource.Get(),
                    m_width,
                    m_height,
                    1,
                    m_format,
                    m_initData.data,
                    m_initData.size,
                    &m_currentState,
                    1,
                    m_arrayLayers);
            }
        }

        void D3D12_Image::create(ID3D12Resource* externalResource, D3D12_RESOURCE_STATES initialState)
        {
            m_resource = externalResource;
            m_currentState = initialState;
            if (externalResource)
            {
                D3D12_RESOURCE_DESC desc = externalResource->GetDesc();
                m_width = static_cast<uint32_t>(desc.Width);
                m_height = static_cast<uint32_t>(desc.Height);
                m_samples = FromDxgiSampleCount(desc.SampleDesc.Count);
                if (m_format == Format::UNDEFINED)
                {
                    m_format = FromDxgiFormat(desc.Format);
                }
            }
        }

        void D3D12_Image::updateData(const void* data, size_t dataSize)
        {
            if (!m_resource || !data || dataSize == 0)
            {
                return;
            }
            m_ctx->transferDataToImage(
                m_resource.Get(),
                m_width,
                m_height,
                1,
                m_format,
                data,
                dataSize,
                &m_currentState,
                1,
                m_arrayLayers);
        }

        void D3D12_Image::updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion)
        {
            if (!m_resource || !data || dataSize == 0)
            {
                if (fence)
                {
                    static_cast<D3D12_Fence*>(fence)->signalOnQueue(m_ctx->getGraphicsQueue());
                }
                if (onCompletion && *onCompletion)
                {
                    (*onCompletion)();
                }
                return;
            }

            m_ctx->transferDataToImage(
                m_resource.Get(),
                m_width,
                m_height,
                1,
                m_format,
                data,
                dataSize,
                &m_currentState,
                1,
                m_arrayLayers);

            if (fence)
            {
                static_cast<D3D12_Fence*>(fence)->signalOnQueue(m_ctx->getGraphicsQueue());
            }
            if (onCompletion && *onCompletion)
            {
                (*onCompletion)();
            }
        }

        void D3D12_Image::uploadAsync(std::vector<uint8_t> data, std::function<void()> callback)
        {
            if (m_resource && !data.empty())
            {
                m_ctx->transferDataToImage(
                    m_resource.Get(),
                    m_width,
                    m_height,
                    1,
                    m_format,
                    data.data(),
                    data.size(),
                    &m_currentState,
                    1,
                    m_arrayLayers);
            }
            if (callback)
            {
                callback();
            }
        }

        void D3D12_Image::transitionState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
        {
            if (!m_resource || !cmdList || m_currentState == newState)
            {
                return;
            }

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = m_resource.Get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = m_currentState;
            barrier.Transition.StateAfter = newState;

            cmdList->ResourceBarrier(1, &barrier);
            m_currentState = newState;
        }
    }
}

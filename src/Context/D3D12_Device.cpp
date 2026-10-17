#include "../FCTAPI.h"
#include "./D3D12_Device.h"
#include "./D3D12_Context.h"
#include "./D3D12_PassResource.h"
#include "./D3D12_FencePool.h"
#include "./D3D12_SemaphorePool.h"
#include "./SingleBufferImage.h"
#include "./MutilBufferImage.h"
#include "./VertexShader.h"
#include "./PixelShader.h"
#include "../RHI/D3D12_VertexShader.h"
#include "../RHI/D3D12_PixelShader.h"
#include "../RHI/D3D12_VertexBuffer.h"
#include "../RHI/D3D12_IndexBuffer.h"
#include "../RHI/D3D12_DescriptorPool.h"
#include "../RHI/D3D12_TraditionalPipeline.h"
#include "../RHI/D3D12_ConstBuffer.h"
#include "../RHI/D3D12_InputLayout.h"
#include "../RHI/D3D12_Image.h"
#include "../RHI/D3D12_Swapchain.h"
#include "../RHI/D3D12_Semaphore.h"
#include "../RHI/D3D12_Fence.h"
#include "../RHI/D3D12_RasterizationState.h"
#include "../RHI/D3D12_BlendState.h"
#include "../RHI/D3D12_Pass.h"
#include "../RHI/D3D12_PassGroup.h"
#include "../RHI/D3D12_CommandPool.h"
#include "../RHI/D3D12_DepthStencilState.h"
#include "../RHI/D3D12_DepthStencilView.h"
#include "../RHI/D3D12_TextureView.h"
#include "../RHI/D3D12_RenderTargetView.h"
#include "../RHI/D3D12_Sampler.h"
#include "../RHI/D3D12_ViewportState.h"

namespace FCT
{
    D3D12_Device::D3D12_Device(D3D12_Context* ctx)
        : m_ctx(ctx)
    {
    }

    VertexShader* D3D12_Device::newVertexShader()
    {
        return new VertexShader(m_ctx);
    }

    PixelShader* D3D12_Device::newPixelShader()
    {
        return new PixelShader(m_ctx);
    }

    RHI::VertexShader* D3D12_Device::newRhiVertexShader()
    {
        return new RHI::D3D12_VertexShader(m_ctx);
    }

    RHI::PixelShader* D3D12_Device::newRhiPixelShader()
    {
        return new RHI::D3D12_PixelShader(m_ctx);
    }

    RHI::ConstBuffer* D3D12_Device::newRhiConstBuffer()
    {
        return new RHI::D3D12_ConstBuffer(m_ctx);
    }

    RHI::InputLayout* D3D12_Device::newRhiInputLayout()
    {
        return new RHI::D3D12_InputLayout(m_ctx);
    }

    SingleBufferImage* D3D12_Device::newSingleBufferImage()
    {
        return new SingleBufferImage(m_ctx);
    }

    MutilBufferImage* D3D12_Device::newMutilBufferImage()
    {
        return new MutilBufferImage(m_ctx);
    }

    RHI::Image* D3D12_Device::newRhiImage()
    {
        return new RHI::D3D12_Image(m_ctx);
    }

    RHI::Swapchain* D3D12_Device::newRhiSwapchain()
    {
        return new RHI::D3D12_Swapchain(m_ctx);
    }

    RHI::PassGroup* D3D12_Device::newRhiPassGroup()
    {
        return new RHI::D3D12_PassGroup(m_ctx);
    }

    RHI::Pass* D3D12_Device::newRhiPass()
    {
        return new RHI::D3D12_Pass(m_ctx);
    }

    RHI::Fence* D3D12_Device::newRhiFence()
    {
        return new RHI::D3D12_Fence(m_ctx);
    }

    RHI::Semaphore* D3D12_Device::newRhiSemaphore()
    {
        return new RHI::D3D12_Semaphore(m_ctx);
    }

    RHI::CommandPool* D3D12_Device::newRhiCommandPool()
    {
        return new RHI::D3D12_CommandPool(m_ctx);
    }

    RHI::VertexBuffer* D3D12_Device::newRhiVertexBuffer()
    {
        return new RHI::D3D12_VertexBuffer(m_ctx);
    }

    RHI::IndexBuffer* D3D12_Device::newRhiIndexBuffer()
    {
        return new RHI::D3D12_IndexBuffer(m_ctx);
    }

    RHI::DescriptorPool* D3D12_Device::newRhiDescriptorPool()
    {
        return new RHI::D3D12_DescriptorPool(m_ctx);
    }

    RHI::RenderTargetView* D3D12_Device::newRhiRenderTargetView()
    {
        return new RHI::D3D12_RenderTargetView(m_ctx);
    }

    RHI::DepthStencilView* D3D12_Device::newRhiDepthStencilView()
    {
        return new RHI::D3D12_DepthStencilView(m_ctx);
    }

    RHI::TextureView* D3D12_Device::newRhiTextureView()
    {
        return new RHI::D3D12_TextureView(m_ctx);
    }

    RasterizationState* D3D12_Device::newRasterizationState()
    {
        return new D3D12_RasterizationState(m_ctx);
    }

    BlendState* D3D12_Device::newBlendState()
    {
        return new D3D12_BlendState(m_ctx);
    }

    Sampler* D3D12_Device::newSampler()
    {
        return new D3D12_Sampler(m_ctx);
    }

    PassResource* D3D12_Device::newPassResource()
    {
        return new D3D12_PassResource(m_ctx);
    }

    SemaphorePool* D3D12_Device::newSemaphorePool()
    {
        return new D3D12_SemaphorePool(m_ctx);
    }

    FencePool* D3D12_Device::newFencePool()
    {
        return new D3D12_FencePool(m_ctx);
    }
}

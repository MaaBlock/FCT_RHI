#include "./WGL2_Device.h"
#include "./WGL2_Context.h"
#include "./VertexShader.h"
#include "./PixelShader.h"
#include "./SingleBufferImage.h"
#include "./MutilBufferImage.h"
#include "./WGL2_PassResource.h"
#include "./WGL2_FencePool.h"
#include "./WGL2_SemaphorePool.h"
#include "../RHI/WGL2_VertexShader.h"
#include "../RHI/WGL2_PixelShader.h"
#include "../RHI/WGL2_ConstBuffer.h"
#include "../RHI/WGL2_InputLayout.h"
#include "../RHI/WGL2_Image.h"
#include "../RHI/WGL2_Swapchain.h"
#include "../RHI/WGL2_PassGroup.h"
#include "../RHI/WGL2_Pass.h"
#include "../RHI/WGL2_Fence.h"
#include "../RHI/WGL2_Semaphore.h"
#include "../RHI/WGL2_CommandPool.h"
#include "../RHI/WGL2_VertexBuffer.h"
#include "../RHI/WGL2_IndexBuffer.h"
#include "../RHI/WGL2_DescriptorPool.h"
#include "../RHI/WGL2_RenderTargetView.h"
#include "../RHI/WGL2_DepthStencilView.h"
#include "../RHI/WGL2_TextureView.h"
#include "../RHI/WGL2_StorageBuffer.h"
#include "../RHI/WGL2_TransformFeedback.h"
#include "../RHI/WGL2_RasterizationState.h"
#include "../RHI/WGL2_DepthStencilState.h"
#include "../RHI/WGL2_BlendState.h"
#include "../RHI/WGL2_Sampler.h"

namespace FCT {

WGL2_Device::WGL2_Device(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

VertexShader* WGL2_Device::newVertexShader()
{
    return FCT_NEW(VertexShader, m_ctx);
}

PixelShader* WGL2_Device::newPixelShader()
{
    return FCT_NEW(PixelShader, m_ctx);
}

RHI::VertexShader* WGL2_Device::newRhiVertexShader()
{
    return FCT_NEW(RHI::WGL2_VertexShader, m_ctx);
}

RHI::PixelShader* WGL2_Device::newRhiPixelShader()
{
    return FCT_NEW(RHI::WGL2_PixelShader, m_ctx);
}

RHI::ConstBuffer* WGL2_Device::newRhiConstBuffer()
{
    return FCT_NEW(RHI::WGL2_ConstBuffer, m_ctx);
}

RHI::InputLayout* WGL2_Device::newRhiInputLayout()
{
    return FCT_NEW(RHI::WGL2_InputLayout, m_ctx);
}

SingleBufferImage* WGL2_Device::newSingleBufferImage()
{
    return FCT_NEW(SingleBufferImage, m_ctx);
}

MutilBufferImage* WGL2_Device::newMutilBufferImage()
{
    return FCT_NEW(MutilBufferImage, m_ctx);
}

RHI::Image* WGL2_Device::newRhiImage()
{
    return FCT_NEW(RHI::WGL2_Image, m_ctx);
}

RHI::Swapchain* WGL2_Device::newRhiSwapchain()
{
    return FCT_NEW(RHI::WGL2_Swapchain, m_ctx);
}

RHI::PassGroup* WGL2_Device::newRhiPassGroup()
{
    return FCT_NEW(RHI::WGL2_PassGroup, m_ctx);
}

RHI::Pass* WGL2_Device::newRhiPass()
{
    return FCT_NEW(RHI::WGL2_Pass, m_ctx);
}

RHI::Fence* WGL2_Device::newRhiFence()
{
    return FCT_NEW(RHI::WGL2_Fence, m_ctx);
}

RHI::Semaphore* WGL2_Device::newRhiSemaphore()
{
    return FCT_NEW(RHI::WGL2_Semaphore, m_ctx);
}

RHI::CommandPool* WGL2_Device::newRhiCommandPool()
{
    return FCT_NEW(RHI::WGL2_CommandPool, m_ctx);
}

RHI::VertexBuffer* WGL2_Device::newRhiVertexBuffer()
{
    return FCT_NEW(RHI::WGL2_VertexBuffer, m_ctx);
}

RHI::IndexBuffer* WGL2_Device::newRhiIndexBuffer()
{
    return FCT_NEW(RHI::WGL2_IndexBuffer, m_ctx);
}

RHI::DescriptorPool* WGL2_Device::newRhiDescriptorPool()
{
    return FCT_NEW(RHI::WGL2_DescriptorPool, m_ctx);
}

RHI::RenderTargetView* WGL2_Device::newRhiRenderTargetView()
{
    return FCT_NEW(RHI::WGL2_RenderTargetView, m_ctx);
}

RHI::DepthStencilView* WGL2_Device::newRhiDepthStencilView()
{
    return FCT_NEW(RHI::WGL2_DepthStencilView, m_ctx);
}

RHI::TextureView* WGL2_Device::newRhiTextureView()
{
    return FCT_NEW(RHI::WGL2_TextureView, m_ctx);
}

RHI::StorageBuffer* WGL2_Device::newRhiStorageBuffer()
{
    return FCT_NEW(RHI::WGL2_StorageBuffer, m_ctx);
}

RHI::StorageBuffer* WGL2_Device::createStorageBuffer(size_t size)
{
    auto* sb = FCT_NEW(RHI::WGL2_StorageBuffer, m_ctx);
    if (size > 0) {
        sb->create(size);
    }
    return sb;
}

RHI::TransformFeedback* WGL2_Device::newRhiTransformFeedback()
{
    return FCT_NEW(RHI::WGL2_TransformFeedback, m_ctx);
}

RasterizationState* WGL2_Device::newRasterizationState()
{
    return FCT_NEW(RHI::WGL2_RasterizationState, m_ctx);
}

DepthStencilState* WGL2_Device::newDepthStencilState()
{
    return FCT_NEW(RHI::WGL2_DepthStencilState, m_ctx);
}

BlendState* WGL2_Device::newBlendState()
{
    return FCT_NEW(RHI::WGL2_BlendState, m_ctx);
}

Sampler* WGL2_Device::newSampler()
{
    return FCT_NEW(RHI::WGL2_Sampler, m_ctx);
}

PassResource* WGL2_Device::newPassResource()
{
    return FCT_NEW(WGL2_PassResource, m_ctx);
}

SemaphorePool* WGL2_Device::newSemaphorePool()
{
    return FCT_NEW(WGL2_SemaphorePool, m_ctx);
}

FencePool* WGL2_Device::newFencePool()
{
    return FCT_NEW(WGL2_FencePool, m_ctx);
}

} // namespace FCT

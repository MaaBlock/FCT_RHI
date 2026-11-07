//
// Created by Administrator on 2025/7/27.
//

#ifndef VK_DEVICE_H
#define VK_DEVICE_H
#include "./Device.h"
namespace FCT {
    class VK_Context;
    class VK_Device : public Device
    {
    protected:
        VK_Context* m_ctx;
    public:
        VK_Device(VK_Context* ctx);
        VertexShader* newVertexShader() override;
        PixelShader* newPixelShader() override;
        RHI::VertexShader* newRhiVertexShader() override;
        RHI::PixelShader* newRhiPixelShader() override;
        RHI::ConstBuffer* newRhiConstBuffer() override;
        RHI::InputLayout* newRhiInputLayout() override;
        SingleBufferImage* newSingleBufferImage() override;
        MutilBufferImage* newMutilBufferImage() override;
        RHI::Image* newRhiImage() override;
        RHI::Swapchain* newRhiSwapchain() override;
        RHI::PassGroup* newRhiPassGroup() override;
        RHI::Pass* newRhiPass() override;
        RHI::Fence* newRhiFence() override;
        RHI::Semaphore* newRhiSemaphore() override;
        RHI::CommandPool* newRhiCommandPool() override;
        RHI::VertexBuffer* newRhiVertexBuffer() override;
        RHI::IndexBuffer* newRhiIndexBuffer() override;
        RHI::DescriptorPool* newRhiDescriptorPool() override;
        RHI::DepthStencilView* newRhiDepthStencilView() override;
        RHI::TextureView* newRhiTextureView() override;
        BlendState* newBlendState() override;
        Sampler* newSampler() override;
        PassResource* newPassResource() override;
        SemaphorePool* newSemaphorePool() override;
        FencePool* newFencePool() override;
        RHI::RenderTargetView* newRhiRenderTargetView() override;
        RasterizationState* newRasterizationState() override;
        DepthStencilState* newDepthStencilState() override;
    };
}
#endif //VK_DEVICE_H

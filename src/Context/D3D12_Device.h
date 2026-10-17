#ifndef FCT_D3D12_DEVICE_H
#define FCT_D3D12_DEVICE_H

#include "./Device.h"

namespace FCT
{
    class D3D12_Context;

    class D3D12_Device : public Device
    {
    public:
        explicit D3D12_Device(D3D12_Context* ctx);
        ~D3D12_Device() override = default;

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
        RHI::RenderTargetView* newRhiRenderTargetView() override;
        RHI::DepthStencilView* newRhiDepthStencilView() override;
        RHI::TextureView* newRhiTextureView() override;
        RasterizationState* newRasterizationState() override;
        BlendState* newBlendState() override;
        Sampler* newSampler() override;
        PassResource* newPassResource() override;
        SemaphorePool* newSemaphorePool() override;
        FencePool* newFencePool() override;

    private:
        D3D12_Context* m_ctx;
    };
}

#endif // FCT_D3D12_DEVICE_H

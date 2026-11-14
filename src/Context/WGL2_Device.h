#pragma once

#include "./Device.h"

namespace FCT {

class WGL2_Context;

class WGL2_Device : public Device {
public:
    explicit WGL2_Device(WGL2_Context* ctx);
    ~WGL2_Device() override = default;

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
    RHI::StorageBuffer* newRhiStorageBuffer() override;
    RHI::StorageBuffer* createStorageBuffer(size_t size = 0) override;
    RHI::TransformFeedback* newRhiTransformFeedback() override;
    RasterizationState* newRasterizationState() override;
    DepthStencilState* newDepthStencilState() override;
    BlendState* newBlendState() override;
    Sampler* newSampler() override;
    PassResource* newPassResource() override;
    SemaphorePool* newSemaphorePool() override;
    FencePool* newFencePool() override;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace FCT

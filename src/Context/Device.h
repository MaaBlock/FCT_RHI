//
// Created by Administrator on 2025/7/27.
//

#ifndef DEVICE_H
#define DEVICE_H
#include <type_traits>

namespace FCT {
    class VertexShader;
    class PixelShader;
    class Image;
    class SingleBufferImage;
    class MutilBufferImage;
    class DescriptorPool;
    class RasterizationState;
    class DepthStencilState;
    class BlendState;
    class Sampler;
    class PassResource;
    class SemaphorePool;
    class FencePool;

    namespace RHI {
        class RasterizationPipeline;
        class VertexShader;
        class PixelShader;
        class ConstBuffer;
        class InputLayout;
        class Image;
        class Swapchain;
        class PassGroup;
        class Pass;
        class Fence;
        class Semaphore;
        class CommandPool;
        class VertexBuffer;
        class IndexBuffer;
        class DescriptorPool;
        class RenderTargetView;
        class DepthStencilView;
        class TextureView;
        class StorageBuffer;
        class TransformFeedback;
        struct TraditionalPipeline
        {

        };
    }

    /**
     * @cond CHINESE
     * @brief 通过这个命名空间即可知道可以创建哪些资源
     * @endcond
     */
    namespace ContextResource
    {
        using VertexShader = FCT::VertexShader;
        using PixelShader = FCT::PixelShader;
        using Image = FCT::Image;
        using SingleBufferImage = FCT::SingleBufferImage;
        using MutilBufferImage = FCT::MutilBufferImage;
    }

    class Device
    {
    public:
        virtual ~Device() = default;
        virtual VertexShader* newVertexShader() = 0;
        virtual PixelShader* newPixelShader() = 0;
        virtual RHI::VertexShader* newRhiVertexShader() = 0;
        virtual RHI::PixelShader* newRhiPixelShader() = 0;
        virtual RHI::ConstBuffer* newRhiConstBuffer() = 0;
        virtual RHI::InputLayout* newRhiInputLayout() = 0;
        virtual SingleBufferImage* newSingleBufferImage() = 0;
        virtual MutilBufferImage* newMutilBufferImage() = 0;
        virtual RHI::Image* newRhiImage() = 0;
        virtual RHI::Swapchain* newRhiSwapchain() = 0;
        virtual RHI::PassGroup* newRhiPassGroup() = 0;
        virtual RHI::Pass* newRhiPass() = 0;
        virtual RHI::Fence* newRhiFence() = 0;
        virtual RHI::Semaphore* newRhiSemaphore() = 0;
        virtual RHI::CommandPool* newRhiCommandPool() = 0;
        virtual RHI::VertexBuffer* newRhiVertexBuffer() = 0;
        virtual RHI::IndexBuffer* newRhiIndexBuffer() = 0;
        virtual RHI::DescriptorPool* newRhiDescriptorPool() = 0;
        virtual RHI::RenderTargetView* newRhiRenderTargetView() = 0;
        virtual RHI::DepthStencilView* newRhiDepthStencilView() = 0;
        virtual RHI::TextureView* newRhiTextureView() = 0;
        virtual RHI::StorageBuffer* newRhiStorageBuffer() { return nullptr; }
        virtual RHI::StorageBuffer* createStorageBuffer(size_t size = 0) { (void)size; return nullptr; }
        virtual RHI::TransformFeedback* newRhiTransformFeedback() { return nullptr; }
        virtual RasterizationState* newRasterizationState() = 0;
        virtual DepthStencilState* newDepthStencilState() = 0;
        virtual BlendState* newBlendState() = 0;
        virtual Sampler* newSampler() = 0;
        virtual PassResource* newPassResource() = 0;
        virtual SemaphorePool* newSemaphorePool() = 0;
        virtual FencePool* newFencePool() = 0;
        template <typename T>
        T* createResource()
        {
            if constexpr (std::is_same_v<T, VertexShader>)
                return newVertexShader();
            else if constexpr (std::is_same_v<T, PixelShader>)
                return newPixelShader();
            else if constexpr (std::is_same_v<T, RHI::VertexShader>)
                return newRhiVertexShader();
            else if constexpr (std::is_same_v<T, RHI::PixelShader>)
                return newRhiPixelShader();
            else if constexpr (std::is_same_v<T, RHI::ConstBuffer>)
                return newRhiConstBuffer();
            else if constexpr (std::is_same_v<T, RHI::InputLayout>)
                return newRhiInputLayout();
            else if constexpr (std::is_same_v<T, Image>)
                return newSingleBufferImage();
            else if constexpr (std::is_same_v<T, SingleBufferImage>)
                return newSingleBufferImage();
            else if constexpr (std::is_same_v<T, MutilBufferImage>)
                return newMutilBufferImage();
            else if constexpr (std::is_same_v<T, RHI::Image>)
                return newRhiImage();
            else if constexpr (std::is_same_v<T, RHI::Swapchain>)
                return newRhiSwapchain();
            else if constexpr (std::is_same_v<T, RHI::PassGroup>)
                return newRhiPassGroup();
            else if constexpr (std::is_same_v<T, RHI::Pass>)
                return newRhiPass();
            else if constexpr (std::is_same_v<T, RHI::Fence>)
                return newRhiFence();
            else if constexpr (std::is_same_v<T, RHI::Semaphore>)
                return newRhiSemaphore();
            else if constexpr (std::is_same_v<T, RHI::CommandPool>)
                return newRhiCommandPool();
            else if constexpr (std::is_same_v<T, RHI::VertexBuffer>)
                return newRhiVertexBuffer();
            else if constexpr (std::is_same_v<T, RHI::IndexBuffer>)
                return newRhiIndexBuffer();
            else if constexpr (std::is_same_v<T, RHI::DescriptorPool>)
                return newRhiDescriptorPool();
            else if constexpr (std::is_same_v<T, RHI::RenderTargetView>)
                return newRhiRenderTargetView();
            else if constexpr (std::is_same_v<T, RHI::DepthStencilView>)
                return newRhiDepthStencilView();
            else if constexpr (std::is_same_v<T, RHI::TextureView>)
                return newRhiTextureView();
            else if constexpr (std::is_same_v<T, RHI::StorageBuffer>)
                return newRhiStorageBuffer();
            else if constexpr (std::is_same_v<T, RHI::TransformFeedback>)
                return newRhiTransformFeedback();
            else if constexpr (std::is_same_v<T, RasterizationState>)
                return newRasterizationState();
            else if constexpr (std::is_same_v<T, DepthStencilState>)
                return newDepthStencilState();
            else if constexpr (std::is_same_v<T, BlendState>)
                return newBlendState();
            else if constexpr (std::is_same_v<T, Sampler>)
                return newSampler();
            else if constexpr (std::is_same_v<T, PassResource>)
                return newPassResource();
            else if constexpr (std::is_same_v<T, SemaphorePool>)
                return newSemaphorePool();
            else if constexpr (std::is_same_v<T, FencePool>)
                return newFencePool();
            return nullptr;
        }
    protected:
    };
}

#endif //DEVICE_H

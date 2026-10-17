//
// Created by Administrator on 2025/2/27.
//

#ifndef TUTORIAL_FCTAPI_H
#define TUTORIAL_FCTAPI_H
#include "ThirdParty.h"
#include "headers.h"
#ifdef FCT_ANDROID
#include "./UI/Android_Window.h"
#include "./UI/Android_UICommon.h"
#endif
#ifdef FCT_USE_VULKAN
#include "./Context/VK_ContextCommon.h"
#include "./Context/VK_Context.h"
#include "./Context/VK_VertexShader.h"
#include "./RHI/VK_Swapchain.h"
#include "./Context/VK_RenderPass.h"
#include "./RHI/VK_VertexShader.h"
#include "./RHI/VK_PixelShader.h"
#include "./RHI/VK_InputLayout.h"
#include "./RHI/VK_ViewportState.h"
#include "./RHI/VK_RasterizationState.h"
#include "./RHI/VK_BlendState.h"
#include "./RHI/VK_RasterizationPipeline.h"
#include "./RHI/VK_TraditionalPipeline.h"
#include "./RHI/VK_PassGroup.h"
#include "./RHI/VK_Image.h"
#include "./RHI/VK_RenderTargetView.h"
#include "./RHI/VK_Pass.h"
#include "./RHI/VK_PixelShader.h"
#include "./RHI/VK_VertexShader.h"
#include "./RHI/VK_TraditionalPipeline.h"
#include "./RHI/VK_CommandPool.h"
#include "./RHI/VK_CommandBuffer.h"
#include "./RHI/VK_Semaphore.h"
#include "./RHI/VK_Fence.h"
#include "./RHI/VK_ShaderBinary.h"
#include "./RHI/VK_VertexBuffer.h"
#include "./RHI/VK_IndexBuffer.h"
#include "./RHI/VK_ConstBuffer.h"
#include "./RHI/VK_DescriptorPool.h"
#include "./RHI/VK_TextureView.h"
#include "./Context/VK_PassResource.h"
#include "./RHI/VK_Sampler.h"
#include "./RHI/VK_DepthStencilView.h"
#include "./RHI/VK_DepthStencilState.h"
//#include "./Context/VK_Image.h"
# include "./Context/VK_SemaphorePool.h"
# include "./Context/VK_FencePool.h"
#endif
#ifdef FCT_USE_D3D12
#include "./Context/D3D12_ContextCommon.h"
#include "./Context/D3D12_Context.h"
#include "./Context/D3D12_Device.h"
#include "./Context/D3D12_ShaderCompiler.h"
#include "./Context/D3D12_PassResource.h"
#include "./Context/D3D12_SemaphorePool.h"
#include "./Context/D3D12_FencePool.h"
#include "./RHI/D3D12_Swapchain.h"
#include "./RHI/D3D12_VertexShader.h"
#include "./RHI/D3D12_PixelShader.h"
#include "./RHI/D3D12_InputLayout.h"
#include "./RHI/D3D12_ViewportState.h"
#include "./RHI/D3D12_RasterizationState.h"
#include "./RHI/D3D12_BlendState.h"
#include "./RHI/D3D12_RasterizationPipeline.h"
#include "./RHI/D3D12_TraditionalPipeline.h"
#include "./RHI/D3D12_PassGroup.h"
#include "./RHI/D3D12_Image.h"
#include "./RHI/D3D12_RenderTargetView.h"
#include "./RHI/D3D12_Pass.h"
#include "./RHI/D3D12_CommandPool.h"
#include "./RHI/D3D12_CommandBuffer.h"
#include "./RHI/D3D12_Semaphore.h"
#include "./RHI/D3D12_Fence.h"
#include "./RHI/D3D12_ShaderBinary.h"
#include "./RHI/D3D12_VertexBuffer.h"
#include "./RHI/D3D12_IndexBuffer.h"
#include "./RHI/D3D12_ConstBuffer.h"
#include "./RHI/D3D12_DescriptorPool.h"
#include "./RHI/D3D12_TextureView.h"
#include "./RHI/D3D12_Sampler.h"
#include "./RHI/D3D12_DepthStencilView.h"
#include "./RHI/D3D12_DepthStencilState.h"
#endif
//#include "./Runtime/FCT_Runtime.h"
#ifdef FCT_ANDROID
#include "./Runtime/Android_Runtime.h"
#endif
#ifdef FCT_USE_GLFW
#include "./UI/GLFW_Window.h"
#include "./UI/GLFW_UICommon.h"
#endif
#ifdef FCT_WIN32
#include "./UI/Win32_UICommon.h"
#include "./UI/Win32_Window.h"
#endif
#ifdef FCT_USE_SHADERC
#include "./Context/Shaderc_ShaderCompiler.h"
#endif
#ifdef FCT_USE_FREEIMAGE
#include "./ImageLoader//FreeImage_ImageLoader.h"
#else
#include "./ImageLoader/Stb_ImageLoader.h"
#endif
#ifdef FCT_USE_ASSIMP
#include "./ModelLoader/Assimp_ModelLoader.h"
#endif

#endif //TUTORIAL_FCTAPI_H

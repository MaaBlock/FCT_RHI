//
// Created by Administrator on 2025/8/2.
//

#ifndef IMAGELAYOUT_H
#define IMAGELAYOUT_H
#include "../Base/Enums.h"
namespace FCT {
    enum class ImageLayout
    {
        undefined,
        general,
        colorAttachmentOptimal,
        depthStencilAttachmentOptimal,
        depthStencilReadOnlyOptimal,
        shaderReadOnlyOptimal,
        transferSrcOptimal,
        transferDstOptimal,
        preinitialized,
        depthReadOnlyStencilAttachmentOptimal,
        depthAttachmentStencilReadOnlyOptimal,
        depthAttachmentOptimal,
        depthReadOnlyOptimal,
        stencilAttachmentOptimal,
        stencilReadOnlyOptimal,
        readOnlyOptimal,
        attachmentOptimal,
        renderingLocalRead,
        presentSrcKHR,
        videoDecodeDstKHR,
        videoDecodeSrcKHR,
        videoDecodeDpbKHR,
        sharedPresentKHR,
        fragmentDensityMapOptimalEXT,
        fragmentShadingRateAttachmentOptimalKHR,
        shadingRateOptimalNV,
        videoEncodeDstKHR,
        videoEncodeSrcKHR,
        videoEncodeDpbKHR,
        attachmentFeedbackLoopOptimalEXT,
        videoEncodeQuantizationMapKHR
    };
#ifdef FCT_USE_VULKAN
     inline vk::ImageLayout ToVkImageLayout(ImageLayout e)
    {
        switch (e)
        {
            case ImageLayout::undefined:
                return vk::ImageLayout::eUndefined;
            case ImageLayout::general:
                return vk::ImageLayout::eGeneral;
            case ImageLayout::colorAttachmentOptimal:
                return vk::ImageLayout::eColorAttachmentOptimal;
            case ImageLayout::depthStencilAttachmentOptimal:
                return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case ImageLayout::depthStencilReadOnlyOptimal:
                return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            case ImageLayout::shaderReadOnlyOptimal:
                return vk::ImageLayout::eShaderReadOnlyOptimal;
            case ImageLayout::transferSrcOptimal:
                return vk::ImageLayout::eTransferSrcOptimal;
            case ImageLayout::transferDstOptimal:
                return vk::ImageLayout::eTransferDstOptimal;
            case ImageLayout::preinitialized:
                return vk::ImageLayout::ePreinitialized;
            case ImageLayout::depthReadOnlyStencilAttachmentOptimal:
                return vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal;
            case ImageLayout::depthAttachmentStencilReadOnlyOptimal:
                return vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
            case ImageLayout::depthAttachmentOptimal:
                return vk::ImageLayout::eDepthAttachmentOptimal;
            case ImageLayout::depthReadOnlyOptimal:
                return vk::ImageLayout::eDepthReadOnlyOptimal;
            case ImageLayout::stencilAttachmentOptimal:
                return vk::ImageLayout::eStencilAttachmentOptimal;
            case ImageLayout::stencilReadOnlyOptimal:
                return vk::ImageLayout::eStencilReadOnlyOptimal;
            case ImageLayout::readOnlyOptimal:
                return vk::ImageLayout::eReadOnlyOptimal;
            case ImageLayout::attachmentOptimal:
                return vk::ImageLayout::eAttachmentOptimal;
            case ImageLayout::renderingLocalRead:
                return vk::ImageLayout::eRenderingLocalRead;
            case ImageLayout::presentSrcKHR:
                return vk::ImageLayout::ePresentSrcKHR;
            case ImageLayout::videoDecodeDstKHR:
                return vk::ImageLayout::eVideoDecodeDstKHR;
            case ImageLayout::videoDecodeSrcKHR:
                return vk::ImageLayout::eVideoDecodeSrcKHR;
            case ImageLayout::videoDecodeDpbKHR:
                return vk::ImageLayout::eVideoDecodeDpbKHR;
            case ImageLayout::sharedPresentKHR:
                return vk::ImageLayout::eSharedPresentKHR;
            case ImageLayout::fragmentDensityMapOptimalEXT:
                return vk::ImageLayout::eFragmentDensityMapOptimalEXT;
            case ImageLayout::fragmentShadingRateAttachmentOptimalKHR:
                return vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;
            case ImageLayout::shadingRateOptimalNV:
                return vk::ImageLayout::eShadingRateOptimalNV;
            case ImageLayout::videoEncodeDstKHR:
                return vk::ImageLayout::eVideoEncodeDstKHR;
            case ImageLayout::videoEncodeSrcKHR:
                return vk::ImageLayout::eVideoEncodeSrcKHR;
            case ImageLayout::videoEncodeDpbKHR:
                return vk::ImageLayout::eVideoEncodeDpbKHR;
            case ImageLayout::attachmentFeedbackLoopOptimalEXT:
                return vk::ImageLayout::eAttachmentFeedbackLoopOptimalEXT;
            case ImageLayout::videoEncodeQuantizationMapKHR:
                return vk::ImageLayout::eVideoEncodeQuantizationMapKHR;
            default:
                return vk::ImageLayout(0);
        }
    }
#endif
#ifdef FCT_USE_D3D12
    inline D3D12_RESOURCE_STATES ToD3D12ResourceState(ImageLayout e)
    {
        switch (e)
        {
            case ImageLayout::undefined:
            case ImageLayout::general:
                return D3D12_RESOURCE_STATE_COMMON;
            case ImageLayout::colorAttachmentOptimal:
            case ImageLayout::attachmentOptimal:
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case ImageLayout::depthStencilAttachmentOptimal:
            case ImageLayout::depthAttachmentOptimal:
            case ImageLayout::stencilAttachmentOptimal:
            case ImageLayout::depthReadOnlyStencilAttachmentOptimal:
            case ImageLayout::depthAttachmentStencilReadOnlyOptimal:
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case ImageLayout::depthStencilReadOnlyOptimal:
            case ImageLayout::depthReadOnlyOptimal:
            case ImageLayout::stencilReadOnlyOptimal:
                return D3D12_RESOURCE_STATE_DEPTH_READ;
            case ImageLayout::shaderReadOnlyOptimal:
                return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            case ImageLayout::readOnlyOptimal:
                return D3D12_RESOURCE_STATE_GENERIC_READ;
            case ImageLayout::transferSrcOptimal:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case ImageLayout::transferDstOptimal:
                return D3D12_RESOURCE_STATE_COPY_DEST;
            case ImageLayout::presentSrcKHR:
                return D3D12_RESOURCE_STATE_PRESENT;
            default:
                return D3D12_RESOURCE_STATE_COMMON;
        }
    }
#endif
}
#endif //IMAGELAYOUT_H

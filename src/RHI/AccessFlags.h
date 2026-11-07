//
// Created by Administrator on 2025/7/30.
//

#ifndef ACCESSFLAGS_H
#define ACCESSFLAGS_H
#include "../ThirdParty.h"
#include "../Base/Flags.h"
namespace FCT {
    enum class AccessFlag : size_t {
        none = 0,
        memoryRead = 1 << 1,
        colorAttachmentWrite = 1 << 2,
        depthStencilAttachmentWrite = 1 << 3,
        shaderRead = 1 << 4,
        shaderWrite = 1 << 5,
        colorAttachmentRead = 1 << 6,
        depthStencilAttachmentRead = 1 << 7,
        transferRead = 1 << 8,
        transferWrite = 1 << 9,
    };
    FCT_DECLARE_FLAGS(AccessFlag);
#ifdef FCT_USE_VULKAN
    inline vk::AccessFlagBits ToVkAccessFlagBits(AccessFlag flag)
    {
        switch (flag) {
            case AccessFlag::none:
                return vk::AccessFlagBits::eNone;
            case AccessFlag::memoryRead:
                return vk::AccessFlagBits::eMemoryRead;
            case AccessFlag::colorAttachmentWrite:
                return vk::AccessFlagBits::eColorAttachmentWrite;
            case AccessFlag::depthStencilAttachmentWrite:
                return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            case AccessFlag::shaderRead:
                return vk::AccessFlagBits::eShaderRead;
            case AccessFlag::shaderWrite:
                return vk::AccessFlagBits::eShaderWrite;
            case AccessFlag::colorAttachmentRead:
                return vk::AccessFlagBits::eColorAttachmentRead;
            case AccessFlag::depthStencilAttachmentRead:
                return vk::AccessFlagBits::eDepthStencilAttachmentRead;
            case AccessFlag::transferRead:
                return vk::AccessFlagBits::eTransferRead;
            case AccessFlag::transferWrite:
                return vk::AccessFlagBits::eTransferWrite;
            default:
                return static_cast<vk::AccessFlagBits>(0);
        }
    }
    /*
    inline vk::AccessFlags ToVkAccessFlags(AccessFlags flags)
    {
        vk::AccessFlags result;

        for (uint32_t i = 0; i < 32; ++i) {
            FCT::AccessFlag singleFlag = static_cast<FCT::AccessFlag>(1u << i);
            if (flags & singleFlag) {
                result |= ToVkAccessFlagBits(singleFlag);
            }
        }

        return result;
    }*/
    FCT_TO_FLAGS(ToVkAccessFlags,ToVkAccessFlagBits,AccessFlag);
#endif
}


#endif //ACCESSFLAGS_H

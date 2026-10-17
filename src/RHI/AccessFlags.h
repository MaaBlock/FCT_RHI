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

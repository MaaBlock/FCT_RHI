#include "VK_InteropBridge.h"
#include "../Context/VK_Context.h"

namespace FCT::RHI {

    ExternalResourceHandle VK_InteropBridge::exportImageMemory(VK_Image* image) {
        ExternalResourceHandle result{};
        if (!image) {
            return result;
        }

#if defined(FCT_WIN32)
        result.type = ExternalHandleType::OpaqueWin32;
        // 如果底层分配了 Vulkan 内存，返回对应的驱动显存引用
        VkImage nativeVkImg = static_cast<VkImage>(image->image());
        result.handle = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(nativeVkImg));
        result.size = static_cast<size_t>(image->width()) * image->height() * 4;
#elif defined(FCT_LINUX)
        result.type = ExternalHandleType::OpaqueFd;
        result.handle = nullptr;
#endif
        return result;
    }

    ExternalResourceHandle VK_InteropBridge::exportSemaphore(vk::Semaphore semaphore, vk::Device device) {
        ExternalResourceHandle result{};
        if (!semaphore) {
            return result;
        }

#if defined(FCT_WIN32)
        result.type = ExternalHandleType::OpaqueWin32;
        VkSemaphore nativeVkSem = static_cast<VkSemaphore>(semaphore);
        result.handle = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(nativeVkSem));
#endif
        return result;
    }

} // namespace FCT::RHI

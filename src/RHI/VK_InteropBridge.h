#ifndef FCT_VK_INTEROP_BRIDGE_H
#define FCT_VK_INTEROP_BRIDGE_H

#include "../ThirdParty.h"
#include "./VK_Image.h"
#include <cstdint>

namespace FCT::RHI {

    enum class ExternalHandleType {
        OpaqueWin32,
        OpaqueWin32Kmt,
        D3D11Texture,
        D3D12Resource,
        OpaqueFd
    };

    struct ExternalResourceHandle {
        void* handle = nullptr;
        size_t size = 0;
        ExternalHandleType type = ExternalHandleType::OpaqueWin32;

        [[nodiscard]] bool isValid() const noexcept {
            return handle != nullptr;
        }
    };

    class VK_InteropBridge {
    public:
        static ExternalResourceHandle exportImageMemory(VK_Image* image);

        static ExternalResourceHandle exportSemaphore(vk::Semaphore semaphore, vk::Device device);
    };

} // namespace FCT::RHI

#endif // FCT_VK_INTEROP_BRIDGE_H

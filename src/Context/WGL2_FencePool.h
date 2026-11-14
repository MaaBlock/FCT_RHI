#pragma once

#include <functional>
#include "../Memory/ObjectPool.h"
#include "./FencePool.h"
#include "../RHI/WGL2_Fence.h"

namespace FCT {

class WGL2_Context;

class WGL2_FencePool : public FencePool {
protected:
    ObjectPool<RHI::WGL2_Fence, WGL2_Context*> m_fencePool;
public:
    explicit WGL2_FencePool(WGL2_Context* ctx) : m_fencePool(ctx) {
        m_fencePool.setDestroyCallback([](RHI::WGL2_Fence* fence) {
            fence->destroy();
        });
    }

    void setDestroyCallback(const std::function<void(RHI::Fence*)>& callback) override {
        m_fencePool.setDestroyCallback([callback](RHI::WGL2_Fence* fence) {
            fence->destroy();
            callback(fence);
        });
    }

    RHI::Fence* alloc() override {
        return m_fencePool.alloc();
    }
};

} // namespace FCT

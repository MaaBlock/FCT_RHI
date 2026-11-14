#pragma once

#include <functional>
#include "../Memory/ObjectPool.h"
#include "./SemaphorePool.h"
#include "../RHI/WGL2_Semaphore.h"

namespace FCT {

class WGL2_Context;

class WGL2_SemaphorePool : public SemaphorePool {
protected:
    ObjectPool<RHI::WGL2_Semaphore, WGL2_Context*> m_semaphorePool;
public:
    explicit WGL2_SemaphorePool(WGL2_Context* ctx) : m_semaphorePool(ctx) {
        m_semaphorePool.setDestroyCallback([](RHI::WGL2_Semaphore* sem) {
            sem->destroy();
        });
    }

    void setDestroyCallback(const std::function<void(RHI::Semaphore*)>& callback) override {
        m_semaphorePool.setDestroyCallback([callback](RHI::WGL2_Semaphore* sem) {
            sem->destroy();
            callback(sem);
        });
    }

    RHI::Semaphore* alloc() override {
        return m_semaphorePool.alloc();
    }
};

} // namespace FCT

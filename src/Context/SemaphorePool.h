//
// Created by Administrator on 2025/5/10.
//

#ifndef FCT_SEMAPHOREPOOL_H
#define FCT_SEMAPHOREPOOL_H

#include <functional>

namespace FCT
{
    namespace RHI
    {
        class Semaphore;
    }

    class SemaphorePool
    {
    protected:

    public:
        virtual ~SemaphorePool() = default;
        virtual RHI::Semaphore* alloc() = 0;
        virtual void setDestroyCallback(const std::function<void(RHI::Semaphore*)>& callback) = 0;
    };
}
#endif //SEMAPHOREPOOL_H

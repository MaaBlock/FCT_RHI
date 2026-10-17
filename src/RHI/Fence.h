//
// Created by Administrator on 2025/4/2.
//
#ifndef FENCE_H
#define FENCE_H
#include "../MutilThreadBase/RefCount.h"
namespace FCT
{
    namespace RHI
    {
        class Fence : public NewRefCount<Fence, true>, public DeletableTrait<Fence>
        {
        public:
            Fence() : NewRefCount(0)
            {
                m_signaled = false;
            }
            virtual ~Fence() {}
            void initSignal(bool signal = true)
            {
                m_signaled = signal;
            }
            virtual void reset() = 0;
            virtual void create() = 0;
            virtual void waitFor() = 0;
            virtual void createSignaled()
            {
                initSignal(true);
                create();
            }
            virtual void destroy() = 0;
            void deleteThis() FCT_REFCOUNT_OVERRIDE {
                if (!this->applyDeleter(this)) {
                    FCT_DELETE(this);
                }
            }
        protected:
            bool m_signaled;
        };
    }
}
#endif //FENCE_H

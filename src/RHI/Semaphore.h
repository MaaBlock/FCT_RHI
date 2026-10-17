//
// Created by Administrator on 2025/4/1.
//
#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include "../MutilThreadBase/RefCount.h"
namespace FCT
{
    namespace RHI
    {
        class Semaphore : public FCT::NewRefCount<Semaphore, true>, public FCT::DeletableTrait<Semaphore>
        {
        public:
            Semaphore() : NewRefCount(0)
            {

            }
            virtual ~Semaphore() = default;
            //实在需要reset可以 调用create,但是不推荐
            /**
             * @cond CHINESE
             * @brief 创建 信号量
             * @note 也可以用于强行reset一个 信号量（如果你知道你在干什么的话）,不推荐这么用
             * @endcond
             */
            virtual void create() = 0;
            /**
             * @cond CHINESE
             * @brief 销毁一个 信号量
             * @endcond
             */
            virtual void destroy() = 0;
            void deleteThis() FCT_REFCOUNT_OVERRIDE {
                if (!this->applyDeleter(this)) {
                    FCT_DELETE(this);
                }
            }
        protected:

        };
    }
}


#endif //SEMAPHORE_H

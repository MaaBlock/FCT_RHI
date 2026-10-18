#ifndef FCT_SWAPCAIN_H
#define FCT_SWAPCAIN_H
#include "../Type/type.h"
#include "Fence.h"
#include "../MutilThreadBase/RefCount.h"
#include "../Context/ImageRenderTarget.h"
#include "../RHI/Semaphore.h"
#include "../Base/IEventSystem.h"
#include "SwapchainEvent.h"
namespace FCT
{
    class Context;
    class MutilBufferImage;

    namespace RHI
    {
        class Image;

        class Swapchain : public RefCount, public IEventSystem<EventSystemConfig::TriggerOnly>
        {
        public:
            Swapchain();
            virtual ~Swapchain() = default;

            /**
             * @cond CHINESE
             * @return 缓存数
             * @endcond
             */
            virtual uint32_t getImageCount() const
            {
                return static_cast<uint32_t>(m_fctImages.size());
            }
            /**
             * @cond CHINESE
             * @return 与getSamples相同
             * @endcond
             */
            virtual Samples getSampleCount() const
            {
                return Samples::sample_1;
            }
            /**
             * @cond CHINESE
             * @brief 传递 surface
             * @param handle 传递了surface
             * @endcond
             */
            void nativeHandle(void* handle)
            {
                m_nativeHandle = handle;
            }
            /**
             * @cond CHINESE
             * @param width 目标宽度
             * @param height 目标高度
             * @endcond
             */
            void size(int width, int height)
            {
                m_dstWidth = width;
                m_dstHeight = height;
            }

            /**
             * @cond CHINESE
             * @return 交换链对应的图像，是一个多缓存图像
             * @endcond
             */
            virtual FCT::Image* image() const;
            /**
             * @cond CHINESE
             * @brief 启用深度模板 缓存
             * @param format 深度模板缓存的格式
             * @endcond
             */
            virtual void enableDepthBuffer(Format format) = 0;
            virtual void create() = 0;
            /**
             * @cond CHINESE
             * @brief 提交present命令
             * @note 其中 包含提交present命令和获取下一个图像俩个 命令，
             * 失败任一一个会触发重建交换 链（如果是因为交换链过时（既窗口改变大小）导致的话）
             * @endcond
             */
            virtual void present() = 0;
            virtual Format getFormat() const = 0;
            /**
             * @cond CHINESE
             * @return 交换链对应的图像的 采样 数
             * @endcond
             */
            virtual Samples getSamples() const
            {
                return Samples::sample_1;
            }
            virtual bool acquireFirstImage() = 0;
            /**
             * @cond CHINESE
             * @brief 获取交换链对应的target
             * @return 交换链对应的target
             * @endcond
             */
            virtual ImageRenderTarget* target() const
            {
                return m_target;
            }
            /**
             * @cond CHINESE
             * @brief 用于渲染命令同步的，获取到 图像，gpu可以执行渲染命令
             * @return gpu获取到图像发出的信号量
             * @endcond
             */
            virtual RHI::Semaphore* getImageAvailableSemaphore() = 0;
            virtual void setPresentFinshSemaphore(RHI::Semaphore* semaphore) = 0;
            /**
             * @cond CHINESE
             * @brief 用于窗口通知需要 recreate
             * @param width 新的width
             * @param height 新的height
             * @endcond
             */
            virtual void needRecreate(int width, int height);
            /**
             * @cond CHINESE
             * @note 主要用来实现max frame in flight，该fence是渲染所必须的
             * @param fence 在提交完present后，获取下一帧图像前等待的fence，
             *      既下一个渲染帧的资源渲染完毕的fence
             * @endcond
             */
            void addRenderFinshFence(RHI::Fence* fence)
            {
                m_renderFinshFences.push_back(fence);
            }
            void clearRenderFinshFences()
            {
                m_renderFinshFences.clear();
            }
            /**
             * @cond CHINESE
             * @brief 渲染完毕以后发出的 信号，是present命令所必须的
             * @param semaphore 渲染完毕以后发出的 信号，是present命令所必须的
             * @endcond
             */
            void addRenderFinshSemaphore(RHI::Semaphore* semaphore)
            {
                m_renderFinshSemaphores.push_back(semaphore);
            }
            void clearRenderFinshSemaphores()
            {
                m_renderFinshSemaphores.clear();
            }
            virtual void sync();
            /**
             * @cond CHINESE
             * @brief 在syncGraph里 的判断是否要recreate节点是否进行了重建交换链， 可持续一帧
             * @return 是否进行了重建交换链
             * @endcond
             */
            bool hasRecreatedInThisFrame() const
            {
                return m_hasRecreatedInThisFrame;
            }

            /**
             * @cond CHINESE
             * @return 创建交换链后得到的实际大小
             * @endcond
             */
            uint32_t width() const
            {
                return m_width;
            }
            /**
             * @cond CHINESE
             * @return 创建交换链后得到的实际大小
             * @endcond
             */
            uint32_t height() const
            {
                return m_height;
            }

            uint32_t getCurrentImageIndex() const
            {
                return m_currentImageIndex;
            }

        protected:
            virtual bool doRecreate() = 0;

            void bindSwapchainImages(FCT::Context* ctx);
            void createDepthBuffer(FCT::Context* ctx, Format format, uint32_t imageCount);
            void updateCurrentImageIndex(uint32_t index);
            void waitAndResetRenderFinishFences();
            void recreateRenderFinishSemaphores();

        protected:
            std::vector<RHI::Fence*> m_renderFinshFences;
            std::vector<RHI::Semaphore*> m_renderFinshSemaphores;
            MutilBufferImage* m_fctImage = nullptr;
            MutilBufferImage* m_depthStencilImage = nullptr;
            std::vector<RHI::Image*> m_fctImages;
            ImageRenderTarget* m_target = nullptr;
            uint32_t m_currentImageIndex = 0;
            bool m_recreatedFlag = false;
            static constexpr uint32_t RECREATE_DEBOUNCE_MS = 50;
            std::chrono::high_resolution_clock::time_point m_lastRecreateRequestTime;
            /**
             * @cond CHINESE
             * @brief 窗口resize的时候传递的大小
             * @endcond
             * @cond ENGLISH
             * @brief The size passed to the window resize callback
             * @endcond
             */
            uint32_t m_pendingWidth = 0;
            uint32_t m_pendingHeight = 0;
            /**
             * @cond CHINESE
             * @brief 交换链对应的surface
             * @endcond
             */
            void* m_nativeHandle;
            /**
             * @cond CHINESE
             * @brief 创建交换链后得到的实际大小
             * @endcond
             * @cond ENGLISH
             * @brief The actual size after creating the swapchain
             * @endcond
             */
            uint32_t m_width;
            /**
             * @cond CHINESE
             * @brief 创建交换链后得到的实际大小
             * @endcond
             * @cond ENGLISH
             * @brief The actual size after creating the swapchain
             * @endcond
             */
            uint32_t m_height;
            /**
             * @cond CHINESE
             * @brief 目标大小
             * @endcond
             * @cond ENGLISH
             * @brief The target size
             * @endcond
             */
            uint32_t m_dstWidth = 0;
            /**
             * @cond CHINESE
             * @brief 目标大小
             * @endcond
             * @cond ENGLISH
             * @brief The target size
             * @endcond
             */
            uint32_t m_dstHeight = 0;
            bool m_hasRecreatedInThisFrame;
        };
    }
}
#endif //FCT_SWAPCAIN_H

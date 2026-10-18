#ifndef FCT_D3D12_SWAPCHAIN_H
#define FCT_D3D12_SWAPCHAIN_H

#include "./Swapcain.h"

namespace FCT
{
    class D3D12_Context;
    class MutilBufferImage;
    class ImageRenderTarget;

    namespace RHI
    {
        class D3D12_Semaphore;

        class D3D12_Swapchain : public Swapchain
        {
        public:
            explicit D3D12_Swapchain(D3D12_Context* ctx);
            ~D3D12_Swapchain() override;

            uint32_t getImageCount() const override { return m_bufferCount; }
            void enableDepthBuffer(Format format) override;
            void create() override;
            void present() override;
            Format getFormat() const override { return m_format; }
            bool acquireFirstImage() override;
            RHI::Semaphore* getImageAvailableSemaphore() override;
            void setPresentFinshSemaphore(RHI::Semaphore* semaphore) override;

            uint32_t currentImageIndex() const { return m_currentImageIndex; }

        protected:
            bool doRecreate() override;

        private:
            void cleanupOldResources();
            void createNativeSwapchain();
            void buildSwapchainBuffers();

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
            D3D12_Semaphore* m_presentFinishSemaphore;
            Format m_format;
            uint32_t m_bufferCount;
        };
    }
}

#endif // FCT_D3D12_SWAPCHAIN_H

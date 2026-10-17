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
            Samples getSampleCount() const override { return Samples::sample_1; }
            FCT::Image* image() const override;
            void enableDepthBuffer(Format format) override;
            void create() override;
            void present() override;
            Format getFormat() const override { return m_format; }
            Samples getSamples() const override { return Samples::sample_1; }
            bool acquireFirstImage() override;
            ImageRenderTarget* target() const override { return m_target; }
            RHI::Semaphore* getImageAvailableSemaphore() override;
            void setPresentFinshSemaphore(RHI::Semaphore* semaphore) override;
            void needRecreate(int width, int height) override;
            void sync() override;

            uint32_t currentImageIndex() const { return m_currentImageIndex; }

        private:
            void cleanupOldResources();
            void createNativeSwapchain();
            void buildSwapchainBuffers();
            bool doRecreate();

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
            MutilBufferImage* m_fctImage;
            MutilBufferImage* m_depthStencilImage;
            std::vector<RHI::Image*> m_fctImages;
            ImageRenderTarget* m_target;
            D3D12_Semaphore* m_presentFinishSemaphore;
            Format m_format;
            uint32_t m_bufferCount;
            uint32_t m_currentImageIndex;
            bool m_needRecreate;
            uint32_t m_pendingWidth;
            uint32_t m_pendingHeight;
        };
    }
}

#endif // FCT_D3D12_SWAPCHAIN_H

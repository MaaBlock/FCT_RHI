#include "../FCTAPI.h"
#include "./D3D12_Swapchain.h"
#include "./D3D12_Image.h"
#include "./D3D12_Semaphore.h"
#include "./D3D12_Fence.h"
#include "../Context/D3D12_Context.h"
#include "../Context/MutilBufferImage.h"
#include "../Context/ImageRenderTarget.h"
#include "../Context/Context.hpp"

namespace FCT
{
    namespace RHI
    {
        D3D12_Swapchain::D3D12_Swapchain(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_presentFinishSemaphore(nullptr)
            , m_format(Format::R8G8B8A8_UNORM)
            , m_bufferCount(3)
        {
        }

        D3D12_Swapchain::~D3D12_Swapchain()
        {
            if (m_ctx)
            {
                m_ctx->waitForGpuIdle();
            }
            cleanupOldResources();
            FCT_SAFE_RELEASE(m_fctImage);
            FCT_SAFE_RELEASE(m_depthStencilImage);
            FCT_SAFE_RELEASE(m_target);
            m_swapchain.Reset();
        }

        void D3D12_Swapchain::cleanupOldResources()
        {
            if (m_fctImage)
            {
                m_fctImage->create({});
            }
            for (auto* img : m_fctImages)
            {
                if (img)
                {
                    static_cast<D3D12_Image*>(img)->create(nullptr);
                    img->release();
                }
            }
            m_fctImages.clear();
        }

        void D3D12_Swapchain::createNativeSwapchain()
        {
            m_bufferCount = (std::max)(2u, m_ctx->maxFrameInFlight());
            m_width = m_dstWidth > 0 ? m_dstWidth : 1;
            m_height = m_dstHeight > 0 ? m_dstHeight : 1;

            if (!m_swapchain)
            {
                DXGI_SWAP_CHAIN_DESC1 desc{};
                desc.Width = m_width;
                desc.Height = m_height;
                desc.Format = ToDxgiFormat(m_format);
                desc.Stereo = FALSE;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                desc.BufferCount = m_bufferCount;
                desc.Scaling = DXGI_SCALING_STRETCH;
                desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                desc.Flags = 0;

                HWND hwnd = static_cast<HWND>(m_nativeHandle);
                Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;
                HRESULT hr = m_ctx->getFactory()->CreateSwapChainForHwnd(
                    m_ctx->getGraphicsQueue(),
                    hwnd,
                    &desc,
                    nullptr,
                    nullptr,
                    &swapchain1);
                if (FAILED(hr))
                {
                    ferr << "Failed to create DXGI SwapChainForHwnd, hr = " << std::hex << hr << std::dec << std::endl;
                    return;
                }

                m_ctx->getFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
                swapchain1.As(&m_swapchain);
            }
            else
            {
                HRESULT hr = m_swapchain->ResizeBuffers(
                    m_bufferCount,
                    m_width,
                    m_height,
                    ToDxgiFormat(m_format),
                    0);
                if (FAILED(hr))
                {
                    ferr << "Failed to resize D3D12 Swapchain buffers, hr = " << std::hex << hr << std::dec << std::endl;
                }
            }
        }

        void D3D12_Swapchain::buildSwapchainBuffers()
        {
            if (!m_swapchain)
            {
                return;
            }

            for (uint32_t i = 0; i < m_bufferCount; ++i)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
                HRESULT hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
                if (FAILED(hr))
                {
                    ferr << "Failed to get D3D12 Swapchain buffer " << i << std::endl;
                    continue;
                }

                auto* d3dImg = FCT_NEW(D3D12_Image, m_ctx);
                d3dImg->samples(getSamples());
                d3dImg->format(getFormat());
                d3dImg->width(m_width);
                d3dImg->height(m_height);
                d3dImg->usage(ImageUsage::RenderTarget);
                d3dImg->create(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);
                m_fctImages.push_back(d3dImg);
            }

            bindSwapchainImages(m_ctx);
        }

        void D3D12_Swapchain::create()
        {
            if (m_pendingWidth > 0 && m_pendingHeight > 0)
            {
                m_dstWidth = m_pendingWidth;
                m_dstHeight = m_pendingHeight;
            }
            cleanupOldResources();
            createNativeSwapchain();
            buildSwapchainBuffers();
        }

        bool D3D12_Swapchain::acquireFirstImage()
        {
            if (!m_swapchain)
            {
                return false;
            }
            uint32_t nextIdx = m_swapchain->GetCurrentBackBufferIndex();
            if (m_presentFinishSemaphore)
            {
                m_presentFinishSemaphore->create();
                m_presentFinishSemaphore->signalOnQueue(m_ctx->getGraphicsQueue());
            }
            updateCurrentImageIndex(nextIdx);
            return true;
        }

        void D3D12_Swapchain::present()
        {
            if (!m_swapchain)
            {
                return;
            }

            ID3D12CommandQueue* queue = m_ctx->getGraphicsQueue();
            {
                std::lock_guard<std::mutex> lock(m_ctx->uploadMutex());
                for (Semaphore* sem : m_renderFinshSemaphores)
                {
                    if (sem)
                    {
                        static_cast<D3D12_Semaphore*>(sem)->waitOnQueue(queue);
                    }
                }

                m_swapchain->Present(0, 0);
            }

            waitAndResetRenderFinishFences();

            uint32_t nextIdx = m_swapchain->GetCurrentBackBufferIndex();
            if (m_presentFinishSemaphore)
            {
                std::lock_guard<std::mutex> lock(m_ctx->uploadMutex());
                m_presentFinishSemaphore->signalOnQueue(queue);
            }

            updateCurrentImageIndex(nextIdx);
        }

        bool D3D12_Swapchain::doRecreate()
        {
            ZoneScoped();
            m_ctx->waitForGpuIdle();
            recreateRenderFinishSemaphores();
            create();
            return acquireFirstImage();
        }

        RHI::Semaphore* D3D12_Swapchain::getImageAvailableSemaphore()
        {
            return m_presentFinishSemaphore;
        }

        void D3D12_Swapchain::setPresentFinshSemaphore(RHI::Semaphore* semaphore)
        {
            m_presentFinishSemaphore = static_cast<D3D12_Semaphore*>(semaphore);
        }

        void D3D12_Swapchain::enableDepthBuffer(Format format)
        {
            createDepthBuffer(m_ctx, format, m_bufferCount);
        }
    }
}

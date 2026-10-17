#include "../FCTAPI.h"
#include "./D3D12_RenderTargetView.h"
#include "./D3D12_Image.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_RenderTargetView::D3D12_RenderTargetView(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_cpuHandle{0}
            , m_descriptorIndex(UINT32_MAX)
        {
        }

        D3D12_RenderTargetView::~D3D12_RenderTargetView()
        {
            releaseDescriptor();
        }

        void D3D12_RenderTargetView::releaseDescriptor()
        {
            if (m_descriptorIndex != UINT32_MAX && m_ctx)
            {
                m_ctx->freeRtvDescriptor(m_descriptorIndex);
                m_descriptorIndex = UINT32_MAX;
                m_cpuHandle = {0};
            }
        }

        void D3D12_RenderTargetView::create()
        {
            releaseDescriptor();
            if (!m_image)
            {
                return;
            }

            auto* d3dImage = static_cast<D3D12_Image*>(m_image);
            ID3D12Resource* resource = d3dImage->getResource();
            if (!resource)
            {
                return;
            }

            m_descriptorIndex = m_ctx->allocRtvDescriptor(m_cpuHandle);

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = ToDxgiFormat(m_image->format());
            if (m_image->samples() > Samples::sample_1)
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
                rtvDesc.Texture2D.PlaneSlice = 0;
            }

            m_ctx->getDevice()->CreateRenderTargetView(resource, &rtvDesc, m_cpuHandle);
        }
    }
}

#include "../FCTAPI.h"
#include "./D3D12_DepthStencilView.h"
#include "./D3D12_Image.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_DepthStencilView::D3D12_DepthStencilView(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_cpuHandle{0}
            , m_descriptorIndex(UINT32_MAX)
        {
        }

        D3D12_DepthStencilView::~D3D12_DepthStencilView()
        {
            releaseDescriptor();
        }

        void D3D12_DepthStencilView::releaseDescriptor()
        {
            if (m_descriptorIndex != UINT32_MAX && m_ctx)
            {
                m_ctx->freeDsvDescriptor(m_descriptorIndex);
                m_descriptorIndex = UINT32_MAX;
                m_cpuHandle = {0};
            }
        }

        bool D3D12_DepthStencilView::create()
        {
            releaseDescriptor();
            if (!m_image || !IsDepthFormat(m_image->format()))
            {
                return false;
            }

            auto* d3dImage = static_cast<D3D12_Image*>(m_image);
            ID3D12Resource* resource = d3dImage->getResource();
            if (!resource)
            {
                return false;
            }

            m_descriptorIndex = m_ctx->allocDsvDescriptor(m_cpuHandle);

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format = ToDxgiDsvFormat(m_image->format());
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            if (m_image->samples() > Samples::sample_1)
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
            }

            m_ctx->getDevice()->CreateDepthStencilView(resource, &dsvDesc, m_cpuHandle);
            return true;
        }
    }
}

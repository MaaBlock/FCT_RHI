#include "../FCTAPI.h"
#include "./D3D12_TextureView.h"
#include "./D3D12_Image.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_TextureView::D3D12_TextureView(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_cpuHandle{0}
            , m_descriptorIndex(UINT32_MAX)
        {
        }

        D3D12_TextureView::~D3D12_TextureView()
        {
            releaseDescriptor();
        }

        void D3D12_TextureView::releaseDescriptor()
        {
            if (m_descriptorIndex != UINT32_MAX && m_ctx)
            {
                m_ctx->freeStagingCbvSrvUavDescriptor(m_descriptorIndex);
                m_descriptorIndex = UINT32_MAX;
                m_cpuHandle = {0};
            }
        }

        void D3D12_TextureView::create()
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

            m_descriptorIndex = m_ctx->allocStagingCbvSrvUavDescriptor(m_cpuHandle);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = ToDxgiSrvFormat(m_image->format());
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (m_image->isCubeMap())
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = 1;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
            else if (m_image->arrayLayers() > 1)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = 1;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = m_image->arrayLayers();
                srvDesc.Texture2DArray.PlaneSlice = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            }
            else if (m_image->samples() > Samples::sample_1)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            }

            m_ctx->getDevice()->CreateShaderResourceView(resource, &srvDesc, m_cpuHandle);
        }
    }
}

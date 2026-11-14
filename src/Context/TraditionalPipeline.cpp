#include "TraditionalPipeline.h"
#include "Context.h"

namespace FCT
{
    TraditionalPipeline::TraditionalPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline)
        : Pipeline(ctx, PipelineType::Traditional, std::move(rhiPipeline))
    {
    }

    TraditionalPipeline::~TraditionalPipeline()
    {
        FCT_SAFE_RELEASE(m_vertexShader);
        FCT_SAFE_RELEASE(m_pixelShader);
        FCT_SAFE_RELEASE(m_blendState);
        FCT_SAFE_RELEASE(m_rasterizationState);
        FCT_SAFE_RELEASE(m_depthStencilState);
        FCT_SAFE_RELEASE(m_viewportState);
    }

    void TraditionalPipeline::setShaders(VertexShader* vs, PixelShader* ps)
    {
        setVertexShader(vs);
        setPixelShader(ps);
    }

    void TraditionalPipeline::setVertexShader(VertexShader* vs)
    {
        FCT_SAFE_RELEASE(m_vertexShader);
        m_vertexShader = vs;
        if (m_vertexShader) m_vertexShader->addRef();
    }

    void TraditionalPipeline::setPixelShader(PixelShader* ps)
    {
        FCT_SAFE_RELEASE(m_pixelShader);
        m_pixelShader = ps;
        if (m_pixelShader) m_pixelShader->addRef();
    }

    void TraditionalPipeline::setVertexLayout(const VertexLayout& layout, uint32_t slot)
    {
        m_vertexLayouts[slot] = layout;
    }

    void TraditionalPipeline::setPixelLayout(const PixelLayout& layout)
    {
        m_pixelLayout = layout;
    }

    void TraditionalPipeline::setResourceLayout(const ResourceLayout& layout)
    {
        m_resourceLayout = layout;
    }

    void TraditionalPipeline::setBlendState(BlendState* bs)
    {
        FCT_SAFE_RELEASE(m_blendState);
        m_blendState = bs;
        if (m_blendState) m_blendState->addRef();
    }

    void TraditionalPipeline::setRasterizationState(RasterizationState* rs)
    {
        FCT_SAFE_RELEASE(m_rasterizationState);
        m_rasterizationState = rs;
        if (m_rasterizationState) m_rasterizationState->addRef();
    }

    void TraditionalPipeline::setDepthStencilState(DepthStencilState* ds)
    {
        FCT_SAFE_RELEASE(m_depthStencilState);
        m_depthStencilState = ds;
        if (m_depthStencilState) m_depthStencilState->addRef();
    }

    void TraditionalPipeline::setViewportState(ViewportState* vs)
    {
        FCT_SAFE_RELEASE(m_viewportState);
        m_viewportState = vs;
        if (m_viewportState) m_viewportState->addRef();
    }

    void TraditionalPipeline::addResources(IPipelineResource* resource)
    {
        if (!resource) return;
        if (auto* rhiRaster = getRhiPipelineAs<RHI::RasterizationPipeline>())
        {
            rhiRaster->addResources(resource);
        }
    }

    void TraditionalPipeline::bindPass(RHI::Pass* pass)
    {
        m_pass = pass;
    }

    Status TraditionalPipeline::build()
    {
        auto* rhiRaster = getRhiPipelineAs<RHI::RasterizationPipeline>();
        if (!rhiRaster)
        {
            return FailedPreconditionError("TraditionalPipeline: 底层 RHI 光栅化管线实例为空");
        }

        for (const auto& [slot, layout] : m_vertexLayouts)
        {
            rhiRaster->setVertexLayout(layout, slot);
        }
        rhiRaster->setPixelLayout(m_pixelLayout);
        rhiRaster->setResourceLayout(m_resourceLayout);

        if (m_vertexShader) rhiRaster->addResources(m_vertexShader);
        if (m_pixelShader) rhiRaster->addResources(m_pixelShader);
        if (m_blendState) rhiRaster->addResources(m_blendState);
        if (m_rasterizationState) rhiRaster->addResources(m_rasterizationState);
        if (m_depthStencilState) rhiRaster->addResources(m_depthStencilState);
        if (m_viewportState) rhiRaster->addResources(m_viewportState);

        if (m_pass)
        {
            rhiRaster->bindPass(m_pass);
        }

        return rhiRaster->create();
    }
}

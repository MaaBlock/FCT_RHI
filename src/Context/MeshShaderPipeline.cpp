#include "MeshShaderPipeline.h"
#include "Context.h"

namespace FCT
{
    MeshShaderPipeline::MeshShaderPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline)
        : Pipeline(ctx, PipelineType::MeshShader, std::move(rhiPipeline))
    {
    }

    MeshShaderPipeline::~MeshShaderPipeline()
    {
        FCT_SAFE_RELEASE(m_pixelShader);
        FCT_SAFE_RELEASE(m_blendState);
        FCT_SAFE_RELEASE(m_rasterizationState);
        FCT_SAFE_RELEASE(m_depthStencilState);
    }

    void MeshShaderPipeline::setPixelShader(PixelShader* ps)
    {
        FCT_SAFE_RELEASE(m_pixelShader);
        m_pixelShader = ps;
        if (m_pixelShader) m_pixelShader->addRef();
    }

    void MeshShaderPipeline::setPixelLayout(const PixelLayout& layout)
    {
        m_pixelLayout = layout;
    }

    void MeshShaderPipeline::setResourceLayout(const ResourceLayout& layout)
    {
        m_resourceLayout = layout;
    }

    void MeshShaderPipeline::setBlendState(BlendState* bs)
    {
        FCT_SAFE_RELEASE(m_blendState);
        m_blendState = bs;
        if (m_blendState) m_blendState->addRef();
    }

    void MeshShaderPipeline::setRasterizationState(RasterizationState* rs)
    {
        FCT_SAFE_RELEASE(m_rasterizationState);
        m_rasterizationState = rs;
        if (m_rasterizationState) m_rasterizationState->addRef();
    }

    void MeshShaderPipeline::setDepthStencilState(DepthStencilState* ds)
    {
        FCT_SAFE_RELEASE(m_depthStencilState);
        m_depthStencilState = ds;
        if (m_depthStencilState) m_depthStencilState->addRef();
    }

    void MeshShaderPipeline::bindPass(RHI::Pass* pass)
    {
        m_pass = pass;
    }

    Status MeshShaderPipeline::build()
    {
        auto* rhiRaster = getRhiPipelineAs<RHI::RasterizationPipeline>();
        if (!rhiRaster)
        {
            return FailedPreconditionError("MeshShaderPipeline: 底层 RHI 光栅化管线实例为空");
        }

        rhiRaster->setPixelLayout(m_pixelLayout);
        rhiRaster->setResourceLayout(m_resourceLayout);

        if (m_pixelShader) rhiRaster->addResources(m_pixelShader);
        if (m_blendState) rhiRaster->addResources(m_blendState);
        if (m_rasterizationState) rhiRaster->addResources(m_rasterizationState);
        if (m_depthStencilState) rhiRaster->addResources(m_depthStencilState);

        if (m_pass)
        {
            rhiRaster->bindPass(m_pass);
        }

        return rhiRaster->create();
    }

    void MeshShaderPipeline::dispatchMesh(
        RHI::CommandBuffer* cmdBuf,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ)
    {
        auto* rhiRaster = getRhiPipelineAs<RHI::RasterizationPipeline>();
        if (rhiRaster)
        {
            rhiRaster->dispatchMesh(cmdBuf, groupCountX, groupCountY, groupCountZ);
        }
    }
}

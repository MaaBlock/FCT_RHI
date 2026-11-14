#pragma once
#ifndef FCT_CONTEXT_MESHSHADERPIPELINE_H
#define FCT_CONTEXT_MESHSHADERPIPELINE_H

#include "Pipeline.h"
#include "PixelShader.h"
#include "../RHI/BlendState.h"
#include "../RHI/RasterizationState.h"
#include "../RHI/DepthStencilState.h"
#include "../RHI/RasterizationPipeline.h"
#include <vector>
#include <string>

namespace FCT
{
    /**
     * @brief 高层 Mesh Shader 渲染管线
     * 采用放大着色器（Task）与网格着色器（Mesh）直接生成图元，绕过固定 IA 装配单元
     */
    class MeshShaderPipeline : public Pipeline
    {
    public:
        MeshShaderPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline);
        ~MeshShaderPipeline() override;

        void setPixelShader(PixelShader* ps);
        void setPixelLayout(const PixelLayout& layout);
        void setResourceLayout(const ResourceLayout& layout);
        void setBlendState(BlendState* bs);
        void setRasterizationState(RasterizationState* rs);
        void setDepthStencilState(DepthStencilState* ds);
        void bindPass(RHI::Pass* pass);

        Status build() override;

        void dispatchMesh(
            RHI::CommandBuffer* cmdBuf,
            uint32_t groupCountX,
            uint32_t groupCountY = 1,
            uint32_t groupCountZ = 1);

    private:
        PixelShader* m_pixelShader = nullptr;
        BlendState* m_blendState = nullptr;
        RasterizationState* m_rasterizationState = nullptr;
        DepthStencilState* m_depthStencilState = nullptr;
        RHI::Pass* m_pass = nullptr;

        PixelLayout m_pixelLayout;
        ResourceLayout m_resourceLayout;
    };
} // namespace FCT

#endif // FCT_CONTEXT_MESHSHADERPIPELINE_H

#pragma once
#ifndef FCT_CONTEXT_TRADITIONALPIPELINE_H
#define FCT_CONTEXT_TRADITIONALPIPELINE_H

#include "Pipeline.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "../RHI/BlendState.h"
#include "../RHI/RasterizationState.h"
#include "../RHI/DepthStencilState.h"
#include "../RHI/ViewportState.h"
#include "../RHI/RasterizationPipeline.h"

namespace FCT
{
    /**
     * @brief 高层传统光栅化管线
     * 封装高阶顶点输入布局、像素布局、材质资源反射与管线状态，桥接至 RHI::RasterizationPipeline
     */
    class TraditionalPipeline : public Pipeline
    {
    public:
        TraditionalPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline);
        ~TraditionalPipeline() override;

        void setShaders(VertexShader* vs, PixelShader* ps);
        void setVertexShader(VertexShader* vs);
        void setPixelShader(PixelShader* ps);
        VertexShader* getVertexShader() const noexcept { return m_vertexShader; }
        PixelShader* getPixelShader() const noexcept { return m_pixelShader; }

        void setVertexLayout(const VertexLayout& layout, uint32_t slot = 0);
        void setPixelLayout(const PixelLayout& layout);
        void setResourceLayout(const ResourceLayout& layout);

        void vertexLayout(const VertexLayout& layout, uint32_t slot = 0) { setVertexLayout(layout, slot); }
        void pixelLayout(const PixelLayout& layout) { setPixelLayout(layout); }
        void resourceLayout(const ResourceLayout& layout) { setResourceLayout(layout); }

        void setBlendState(BlendState* bs);
        void setRasterizationState(RasterizationState* rs);
        void setDepthStencilState(DepthStencilState* ds);
        void setViewportState(ViewportState* vs);

        void addResources(IPipelineResource* resource);
        void bindPass(RHI::Pass* pass);

        Status build() override;

    private:
        VertexShader* m_vertexShader = nullptr;
        PixelShader* m_pixelShader = nullptr;
        BlendState* m_blendState = nullptr;
        RasterizationState* m_rasterizationState = nullptr;
        DepthStencilState* m_depthStencilState = nullptr;
        ViewportState* m_viewportState = nullptr;
        RHI::Pass* m_pass = nullptr;

        std::map<uint32_t, VertexLayout> m_vertexLayouts;
        PixelLayout m_pixelLayout;
        ResourceLayout m_resourceLayout;
    };
} // namespace FCT

#endif // FCT_CONTEXT_TRADITIONALPIPELINE_H

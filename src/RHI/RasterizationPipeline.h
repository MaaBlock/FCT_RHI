#pragma once
#ifndef RASTERIZATION_PIPELINE_H
#define RASTERIZATION_PIPELINE_H

#include "../MutilThreadBase/RefCount.h"
#include "./IPipelineResource.h"
#include "./Pipeline.h"
#include "./CommandBuffer.h"
#include "../Context/ResourceLayout.h"
#include "../Context/Vertex.h"
#include <map>

namespace FCT
{
    class BlendState;
    class RasterizationState;
    class DepthStencilState;
    class ViewportState;

    namespace RHI
    {
        class InputLayout;
        class Pass;

        /**
         * @brief 底层 RHI 光栅化管线纯虚契约基类
         * 覆盖 Traditional 与 MeshShader 光栅化阶段配置
         */
        class RasterizationPipeline : public Pipeline
        {
        public:
            RasterizationPipeline() = default;
            ~RasterizationPipeline() override = default;

            virtual void addResources(IPipelineResource* resource) = 0;
            virtual Status create() override = 0;
            virtual void bindPass(FCT::RHI::Pass* pass) = 0;

            // 布局配置接口
            virtual void setVertexLayout(const VertexLayout& layout, uint32_t slot = 0)
            {
                m_vertexLayouts[slot] = layout;
            }
            virtual void setPixelLayout(const PixelLayout& layout)
            {
                m_pixelLayout = layout;
            }
            virtual void setResourceLayout(const ResourceLayout& layout)
            {
                m_resourceLayout = layout;
            }

            // 兼容历史命名接口
            void vertexLayout(VertexLayout layout, uint32_t slot = 0) { setVertexLayout(layout, slot); }
            void pixelLayout(const PixelLayout& layout) { setPixelLayout(layout); }
            void resourceLayout(const ResourceLayout& layout) { setResourceLayout(layout); }

            // 固定功能状态配置接口
            virtual void setBlendState(BlendState* state) {}
            virtual void setRasterizationState(RasterizationState* state) {}
            virtual void setDepthStencilState(DepthStencilState* state) {}
            virtual void setViewportState(ViewportState* state) {}
            virtual void setInputLayout(InputLayout* layout) {}

            // 网格着色器管线任务分发虚接口 (由具体 MeshShader 管线派生类实现)
            virtual void dispatchMesh(
                CommandBuffer* cmdBuf,
                uint32_t groupCountX,
                uint32_t groupCountY = 1,
                uint32_t groupCountZ = 1) {}

        protected:
            std::map<uint32_t, VertexLayout> m_vertexLayouts;
            PixelLayout m_pixelLayout;
            ResourceLayout m_resourceLayout;
        };
    }
}
#endif // RASTERIZATION_PIPELINE_H

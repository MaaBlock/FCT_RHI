#pragma once
#ifndef VK_TRADITIONALPIPELINE_H
#define VK_TRADITIONALPIPELINE_H

#include "./VK_RasterizationPipeline.h"
#include "./VertexShader.h"
#include "./PixelShader.h"
#include "../Base/Status.h"

namespace FCT
{
    class VK_Context;
    namespace RHI
    {
        class VK_TraditionalPipeline : public VK_RasterizationPipeline
        {
        public:
            explicit VK_TraditionalPipeline(VK_Context* ctx);
            ~VK_TraditionalPipeline() override;

            PipelineType getType() const noexcept override { return PipelineType::Traditional; }
            void addResources(IPipelineResource* resource) override;
            Status create() override;

        private:
            void generateDefaultResources();
            Status createDescriptorSetLayouts();
            FCT::VertexShader* m_vertexShader = nullptr;
            FCT::PixelShader* m_pixelShader = nullptr;
            vk::DescriptorSetLayout m_emptyLayout = nullptr;
        };
    }
}
#endif // VK_TRADITIONALPIPELINE_H

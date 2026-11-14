#pragma once
#ifndef VK_RASTERIZATIONPIPELINE_H
#define VK_RASTERIZATIONPIPELINE_H

#include "./RasterizationPipeline.h"
#include "./VK_BlendState.h"
#include "./VK_InputLayout.h"
#include "./VK_ViewportState.h"
#include "./VK_RasterizationState.h"
#include "./VK_DepthStencilState.h"
#include "VK_PassGroup.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <map>

namespace FCT
{
    class VK_Context;

    namespace RHI
    {
        class VK_Pass;

        /**
         * @brief Vulkan 光栅化管线基底驱动实现
         * 管理光栅化状态、动态状态、RenderPass 匹配与 PSO 构建
         */
        class VK_RasterizationPipeline : public RasterizationPipeline
        {
        public:
            explicit VK_RasterizationPipeline(VK_Context* ctx);
            ~VK_RasterizationPipeline() override;

            void addResources(IPipelineResource* resource) override;
            Status create() override;
            void bindPass(Pass* pass) override;
            void bind(CommandBuffer* cmdBuf) override;

            uint64_t getNativeHandle() const noexcept override
            {
                return reinterpret_cast<uint64_t>(static_cast<VkPipeline>(m_pipeline));
            }

            uint64_t getNativeLayoutHandle() const noexcept override
            {
                return reinterpret_cast<uint64_t>(static_cast<VkPipelineLayout>(m_pipelineLayout));
            }

            vk::Pipeline pipeline() const noexcept
            {
                return m_pipeline;
            }

            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts() const noexcept
            {
                return m_descriptorSetLayoutsArr;
            }

            vk::PipelineLayout pipelineLayout() const noexcept
            {
                return m_pipelineLayout;
            }

            void setBlendState(BlendState* state) override;
            void setRasterizationState(RasterizationState* state) override;
            void setDepthStencilState(DepthStencilState* state) override;
            void setViewportState(ViewportState* state) override;
            void setInputLayout(InputLayout* layout) override;

        protected:
            void generateDefaultResources();
            Status setupCommonGraphicsState();

            VK_Context* m_ctx = nullptr;
            VK_BlendState* m_blendState = nullptr;
            VK_InputLayout* m_inputLayout = nullptr;
            VK_ViewportState* m_viewportState = nullptr;
            VK_RasterizationState* m_rasterizationState = nullptr;
            VK_DepthStencilState* m_depthStencilState = nullptr;
            VK_Pass* m_pass = nullptr;

            std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
            vk::GraphicsPipelineCreateInfo m_createInfo{};
            vk::Pipeline m_pipeline = nullptr;
            vk::PipelineCache m_pipelineCache = nullptr;
            vk::PipelineDynamicStateCreateInfo m_dynamicState{};
            vk::PipelineVertexInputStateCreateInfo m_vertexInputState{};
            vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyState{};
            std::vector<vk::DescriptorSetLayout> m_descriptorSetLayoutsArr;
            std::map<uint32_t, vk::DescriptorSetLayout> m_descriptorSetLayouts;
            vk::PipelineLayout m_pipelineLayout = nullptr;
            std::vector<vk::DynamicState> m_dynamicStates;
            std::vector<vk::PipelineColorBlendAttachmentState> m_resolvedColorBlendAttachments;
            vk::PipelineColorBlendStateCreateInfo m_resolvedColorBlendState{};
        };
    }
}

#endif // VK_RASTERIZATIONPIPELINE_H

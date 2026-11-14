#pragma once
#ifndef VK_MESHSHADERPIPELINE_H
#define VK_MESHSHADERPIPELINE_H

#include "./VK_RasterizationPipeline.h"
#include <vector>

namespace FCT
{
    namespace RHI
    {
        /**
         * @brief 基于 VK_EXT_mesh_shader 的 Vulkan 网格着色器管线驱动实现
         * 负责 Task/Mesh/Pixel 着色阶段装配与网格分发
         */
        class VK_MeshShaderPipeline : public VK_RasterizationPipeline
        {
        public:
            explicit VK_MeshShaderPipeline(VK_Context* ctx);
            ~VK_MeshShaderPipeline() override;

            PipelineType getType() const noexcept override { return PipelineType::MeshShader; }
            Status create() override;

            void setTaskShader(const vk::PipelineShaderStageCreateInfo& stage);
            void setMeshShader(const vk::PipelineShaderStageCreateInfo& stage);
            void setPixelShader(const vk::PipelineShaderStageCreateInfo& stage);
            void setShaderStages(const std::vector<vk::PipelineShaderStageCreateInfo>& stages);

            /**
             * @brief 触发网格任务分发绘制 (vkCmdDrawMeshTasksEXT)
             */
            void dispatchMesh(
                CommandBuffer* cmdBuf,
                uint32_t groupCountX,
                uint32_t groupCountY = 1,
                uint32_t groupCountZ = 1) override;

        private:
            vk::PipelineVertexInputStateCreateInfo m_emptyVertexInput{};
            bool m_hasMeshShader = false;
        };
    }
}

#endif // VK_MESHSHADERPIPELINE_H

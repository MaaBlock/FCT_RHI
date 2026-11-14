#pragma once
#ifndef FCT_RHI_VK_RAYTRACINGSHADER_H
#define FCT_RHI_VK_RAYTRACINGSHADER_H

#include "../ThirdParty.h"
#include "./RayTracingShader.h"

namespace FCT
{
    class VK_Context;

    namespace RHI
    {
        /**
         * @brief 基于 Vulkan 的硬件光线追踪着色模块封装
         */
        class VK_RayTracingShader : public RayTracingShader
        {
        public:
            VK_RayTracingShader(VK_Context* ctx, ShaderStage stage);
            ~VK_RayTracingShader() override;

            PipelineResourceType getType() const override;
            ShaderStage getStage() const noexcept override { return m_stage; }
            Status create() override;

            vk::PipelineShaderStageCreateInfo getStageInfo() const noexcept { return m_stageInfo; }
            vk::ShaderModule getModule() const noexcept { return m_module; }

        private:
            VK_Context* m_ctx = nullptr;
            ShaderStage m_stage;
            vk::ShaderModule m_module = nullptr;
            vk::PipelineShaderStageCreateInfo m_stageInfo{};
            std::string m_entryPointStored = "main";
        };
    }
}

#endif // FCT_RHI_VK_RAYTRACINGSHADER_H

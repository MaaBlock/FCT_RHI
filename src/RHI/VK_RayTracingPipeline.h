#pragma once
#ifndef VK_RAYTRACINGPIPELINE_H
#define VK_RAYTRACINGPIPELINE_H

#include "./RayTracingPipeline.h"
#include <vulkan/vulkan.hpp>
#include <vector>

namespace FCT
{
    class VK_Context;

    namespace RHI
    {
        class StorageBuffer;

        /**
         * @brief 基于 VK_KHR_ray_tracing_pipeline 的 Vulkan 硬件光追管线实现
         */
        class VK_RayTracingPipeline : public RayTracingPipeline
        {
        public:
            explicit VK_RayTracingPipeline(VK_Context* ctx);
            ~VK_RayTracingPipeline() override;

            Status create() override;
            void bind(CommandBuffer* cmdBuf) override;

            Status buildSBT() override;
            const StridedDeviceAddressRegions& getSBTRegions() const noexcept override
            {
                return m_sbtRegions;
            }

            void traceRays(
                CommandBuffer* cmdBuf,
                uint32_t width,
                uint32_t height,
                uint32_t depth = 1) override;

            void setMaxRecursionDepth(uint32_t depth) override
            {
                m_maxRecursionDepth = depth;
            }

            uint64_t getNativeHandle() const noexcept override
            {
                return reinterpret_cast<uint64_t>(static_cast<VkPipeline>(m_pipeline));
            }
            uint64_t getNativeLayoutHandle() const noexcept override
            {
                return reinterpret_cast<uint64_t>(static_cast<VkPipelineLayout>(m_pipelineLayout));
            }

            vk::Pipeline pipeline() const noexcept { return m_pipeline; }
            vk::PipelineLayout pipelineLayout() const noexcept { return m_pipelineLayout; }

            void setPipelineLayout(vk::PipelineLayout layout)
            {
                m_pipelineLayout = layout;
                m_externalLayout = true;
            }
            Status setLayouts(const std::vector<vk::DescriptorSetLayout>& layouts, const std::vector<vk::PushConstantRange>& pushConstants = {});

            // 着色器组配置接口
            void addRayGenShaderStage(const vk::PipelineShaderStageCreateInfo& stage);
            void addMissShaderStage(const vk::PipelineShaderStageCreateInfo& stage);
            void addCallableShaderStage(const vk::PipelineShaderStageCreateInfo& stage);
            void addHitGroup(
                uint32_t closestHitStageIndex,
                uint32_t anyHitStageIndex = VK_SHADER_UNUSED_KHR,
                uint32_t intersectionStageIndex = VK_SHADER_UNUSED_KHR);
            void addHitGroupStages(
                const vk::PipelineShaderStageCreateInfo* closestHit,
                const vk::PipelineShaderStageCreateInfo* anyHit = nullptr,
                const vk::PipelineShaderStageCreateInfo* intersection = nullptr);

        private:
            Status createRayTracingPipelineLayout();

            VK_Context* m_ctx = nullptr;
            vk::Pipeline m_pipeline = nullptr;
            vk::PipelineLayout m_pipelineLayout = nullptr;
            bool m_externalLayout = false;

            std::vector<vk::PipelineShaderStageCreateInfo> m_stages;
            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;

            // 各着色器组在 m_shaderGroups 中的显式索引映射表，杜绝句柄提取时的顺序耦合
            std::vector<uint32_t> m_rayGenGroupIndices;
            std::vector<uint32_t> m_missGroupIndices;
            std::vector<uint32_t> m_hitGroupIndices;
            std::vector<uint32_t> m_callableGroupIndices;

            uint32_t m_rayGenGroupCount = 0;
            uint32_t m_missGroupCount = 0;
            uint32_t m_hitGroupCount = 0;
            uint32_t m_callableGroupCount = 0;
            uint32_t m_maxRecursionDepth = 1;

            StorageBuffer* m_sbtBuffer = nullptr;
            StridedDeviceAddressRegions m_sbtRegions{};
        };
    }
}

#endif // VK_RAYTRACINGPIPELINE_H

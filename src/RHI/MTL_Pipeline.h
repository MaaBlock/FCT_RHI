#pragma once
#ifndef FCT_RHI_MTL_PIPELINE_H
#define FCT_RHI_MTL_PIPELINE_H

#include "./RasterizationPipeline.h"
#include "./RayTracingPipeline.h"
#include <string>

namespace FCT::RHI
{
    /**
     * @brief Metal 统一光栅化管线基底驱动契约
     */
    class MTL_RasterizationPipeline : public RasterizationPipeline
    {
    public:
        MTL_RasterizationPipeline() = default;
        ~MTL_RasterizationPipeline() override = default;

        void addResources(IPipelineResource* resource) override {}
        void bindPass(Pass* pass) override {}
        void bind(CommandBuffer* cmdBuf) override {}
    };

    /**
     * @brief Metal 传统光栅化管线驱动契约
     */
    class MTL_TraditionalPipeline : public MTL_RasterizationPipeline
    {
    public:
        MTL_TraditionalPipeline() = default;
        ~MTL_TraditionalPipeline() override = default;

        PipelineType getType() const noexcept override { return PipelineType::Traditional; }

        Status create() override
        {
            return UnimplementedError("MTL_TraditionalPipeline: Metal 后端正在对接中");
        }
    };

    /**
     * @brief 基于 Metal 3 的网格着色器管线驱动契约
     */
    class MTL_MeshShaderPipeline : public MTL_RasterizationPipeline
    {
    public:
        MTL_MeshShaderPipeline() = default;
        ~MTL_MeshShaderPipeline() override = default;

        PipelineType getType() const noexcept override { return PipelineType::MeshShader; }

        Status create() override
        {
            return UnimplementedError("MTL_MeshShaderPipeline: Metal 3 网格管线正在对接中");
        }
    };

    /**
     * @brief 基于 Metal 3 硬件光线追踪管线契约
     */
    class MTL_RayTracingPipeline : public RayTracingPipeline
    {
    public:
        MTL_RayTracingPipeline() = default;
        ~MTL_RayTracingPipeline() override = default;

        Status create() override
        {
            return UnimplementedError("MTL_RayTracingPipeline: Metal 光追后端正在对接中");
        }

        void bind(CommandBuffer* cmdBuf) override {}

        Status buildSBT() override
        {
            return UnimplementedError("MTL_RayTracingPipeline: Metal SBT 正在对接中");
        }

        const StridedDeviceAddressRegions& getSBTRegions() const noexcept override
        {
            return m_sbtRegions;
        }

        void traceRays(CommandBuffer* cmdBuf, uint32_t width, uint32_t height, uint32_t depth = 1) override {}

        void setMaxRecursionDepth(uint32_t depth) override
        {
            m_maxRecursionDepth = depth;
        }

    private:
        StridedDeviceAddressRegions m_sbtRegions{};
        uint32_t m_maxRecursionDepth = 1;
    };
}

#endif // FCT_RHI_MTL_PIPELINE_H

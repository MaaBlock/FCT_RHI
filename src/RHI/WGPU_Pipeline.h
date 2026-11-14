#pragma once
#ifndef FCT_RHI_WGPU_PIPELINE_H
#define FCT_RHI_WGPU_PIPELINE_H

#include "./RasterizationPipeline.h"
#include "./RayTracingPipeline.h"

namespace FCT::RHI
{
    /**
     * @brief WebGPU 统一光栅化管线基底驱动契约
     */
    class WGPU_RasterizationPipeline : public RasterizationPipeline
    {
    public:
        WGPU_RasterizationPipeline() = default;
        ~WGPU_RasterizationPipeline() override = default;

        void addResources(IPipelineResource* resource) override {}
        void bindPass(Pass* pass) override {}
        void bind(CommandBuffer* cmdBuf) override {}
    };

    /**
     * @brief WebGPU 传统光栅化管线驱动契约
     */
    class WGPU_TraditionalPipeline : public WGPU_RasterizationPipeline
    {
    public:
        WGPU_TraditionalPipeline() = default;
        ~WGPU_TraditionalPipeline() override = default;

        PipelineType getType() const noexcept override { return PipelineType::Traditional; }

        Status create() override
        {
            return UnimplementedError("WGPU_TraditionalPipeline: WebGPU 后端正在对接中");
        }
    };

    /**
     * @brief WebGPU 网格着色器管线（规范暂未支持，优雅返回 Unimplemented）
     */
    class WGPU_MeshShaderPipeline : public WGPU_RasterizationPipeline
    {
    public:
        WGPU_MeshShaderPipeline() = default;
        ~WGPU_MeshShaderPipeline() override = default;

        PipelineType getType() const noexcept override { return PipelineType::MeshShader; }

        Status create() override
        {
            return UnimplementedError("WGPU_MeshShaderPipeline: WebGPU 1.0 标准暂不支持 Mesh Shader");
        }
    };

    /**
     * @brief WebGPU 硬件光线追踪管线（规范暂未支持，优雅返回 Unimplemented）
     */
    class WGPU_RayTracingPipeline : public RayTracingPipeline
    {
    public:
        WGPU_RayTracingPipeline() = default;
        ~WGPU_RayTracingPipeline() override = default;

        Status create() override
        {
            return UnimplementedError("WGPU_RayTracingPipeline: WebGPU 1.0 标准暂不支持硬件光线追踪");
        }

        void bind(CommandBuffer* cmdBuf) override {}

        Status buildSBT() override
        {
            return UnimplementedError("WGPU_RayTracingPipeline: WebGPU 暂不支持 SBT 构建");
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

#endif // FCT_RHI_WGPU_PIPELINE_H

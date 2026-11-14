#pragma once
#ifndef FCT_RHI_RAYTRACINGPIPELINE_H
#define FCT_RHI_RAYTRACINGPIPELINE_H

#include "Pipeline.h"
#include <vector>
#include <cstdint>

namespace FCT::RHI
{
    /**
     * @brief 着色器绑定表 (SBT) 单个区域的跨度与设备地址描述
     * 100% 对齐 VkStridedDeviceAddressRegionKHR，跨平台统一呈现
     */
    struct StridedDeviceAddressRegion
    {
        uint64_t deviceAddress = 0;
        uint64_t stride = 0;
        uint64_t size = 0;
    };

    /**
     * @brief 光追着色器绑定表的四个核心阶段区域集合
     */
    struct StridedDeviceAddressRegions
    {
        StridedDeviceAddressRegion rayGen{};    // 光线生成着色器区域 (stride == size)
        StridedDeviceAddressRegion miss{};      // 未命中着色器区域
        StridedDeviceAddressRegion hitGroup{};  // 命中组区域 (ClosestHit + AnyHit + Intersection)
        StridedDeviceAddressRegion callable{};  // 可调用着色器区域
    };

    /**
     * @brief 底层 RHI 硬件光线追踪管线纯虚契约基类
     */
    class RayTracingPipeline : public Pipeline
    {
    public:
        RayTracingPipeline() = default;
        ~RayTracingPipeline() override = default;

        PipelineType getType() const noexcept override { return PipelineType::RayTracing; }

        /**
         * @brief 构建着色器绑定表 (SBT) 显存缓冲区并计算对齐偏移
         * @return 运行期状态
         */
        virtual Status buildSBT() = 0;

        /**
         * @brief 获取构建完成的 SBT 跨度地址区域描述
         */
        virtual const StridedDeviceAddressRegions& getSBTRegions() const noexcept = 0;

        /**
         * @brief 录制并分发光线追踪调用指令 (如 vkCmdTraceRaysKHR)
         * @param cmdBuf 目标命令缓冲区
         * @param width 光线分发网格宽度 (X 维度)
         * @param height 光线分发网格高度 (Y 维度)
         * @param depth 光线分发网格深度 (Z 维度，默认为 1)
         */
        virtual void traceRays(
            CommandBuffer* cmdBuf,
            uint32_t width,
            uint32_t height,
            uint32_t depth = 1) = 0;

        /**
         * @brief 设置最大光线递归深度
         */
        virtual void setMaxRecursionDepth(uint32_t depth) = 0;
    };
}

#endif // FCT_RHI_RAYTRACINGPIPELINE_H

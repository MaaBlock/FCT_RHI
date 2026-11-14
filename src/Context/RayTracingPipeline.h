#pragma once
#ifndef FCT_CONTEXT_RAYTRACINGPIPELINE_H
#define FCT_CONTEXT_RAYTRACINGPIPELINE_H

#include "Pipeline.h"
#include "../RHI/RayTracingPipeline.h"
#include <vector>
#include <string>

namespace FCT
{
    class RayGenShader;
    class MissShader;
    class ClosestHitShader;
    class AnyHitShader;
    class IntersectionShader;
    class CallableShader;

    /**
     * @brief 光追 HitGroup 逻辑描述
     */
    struct RayTracingHitGroup
    {
        std::string name;
        uint32_t closestHitStageIndex = UINT32_MAX;
        uint32_t anyHitStageIndex = UINT32_MAX;
        uint32_t intersectionStageIndex = UINT32_MAX;
        bool isProcedural = false;
    };

    struct RayTracingHitGroupShaders
    {
        ClosestHitShader* closestHit = nullptr;
        AnyHitShader* anyHit = nullptr;
        IntersectionShader* intersection = nullptr;
    };

    /**
     * @brief 高层硬件光线追踪管线抽象类（基于 VK_KHR_ray_tracing_pipeline）
     */
    class RayTracingPipeline : public Pipeline
    {
    public:
        RayTracingPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline);
        ~RayTracingPipeline() override;

        void addRayGenShader(RayGenShader* shader);
        void addMissShader(MissShader* shader);
        void addCallableShader(CallableShader* shader);
        void addHitGroup(ClosestHitShader* closestHit, AnyHitShader* anyHit = nullptr, IntersectionShader* intersection = nullptr);
        void addHitGroup(const RayTracingHitGroup& hitGroup);
        void setMaxRecursionDepth(uint32_t depth);

        Status build() override;

        const RHI::StridedDeviceAddressRegions& getSBTRegions() const;

        void traceRays(
            RHI::CommandBuffer* cmdBuf,
            uint32_t width,
            uint32_t height,
            uint32_t depth = 1);

    private:
        std::vector<RayGenShader*> m_rayGenShaders;
        std::vector<MissShader*> m_missShaders;
        std::vector<CallableShader*> m_callableShaders;
        std::vector<RayTracingHitGroupShaders> m_hitGroupShaders;
        std::vector<RayTracingHitGroup> m_hitGroups;
        uint32_t m_maxRecursionDepth = 1;
    };
} // namespace FCT

#endif // FCT_CONTEXT_RAYTRACINGPIPELINE_H

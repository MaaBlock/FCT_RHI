#include "RayTracingPipeline.h"
#include "RayTracingShader.h"
#include "Context.h"
#include "../RHI/VK_RayTracingPipeline.h"
#include "../RHI/VK_RayTracingShader.h"

namespace FCT
{
    RayTracingPipeline::RayTracingPipeline(Context* ctx, UniquePtr<RHI::Pipeline> rhiPipeline)
        : Pipeline(ctx, PipelineType::RayTracing, std::move(rhiPipeline))
    {
    }

    RayTracingPipeline::~RayTracingPipeline() = default;

    void RayTracingPipeline::addRayGenShader(RayGenShader* shader)
    {
        if (shader)
        {
            m_rayGenShaders.push_back(shader);
        }
    }

    void RayTracingPipeline::addMissShader(MissShader* shader)
    {
        if (shader)
        {
            m_missShaders.push_back(shader);
        }
    }

    void RayTracingPipeline::addCallableShader(CallableShader* shader)
    {
        if (shader)
        {
            m_callableShaders.push_back(shader);
        }
    }

    void RayTracingPipeline::addHitGroup(ClosestHitShader* closestHit, AnyHitShader* anyHit, IntersectionShader* intersection)
    {
        m_hitGroupShaders.push_back(RayTracingHitGroupShaders{ closestHit, anyHit, intersection });
    }

    void RayTracingPipeline::addHitGroup(const RayTracingHitGroup& hitGroup)
    {
        m_hitGroups.push_back(hitGroup);
    }

    void RayTracingPipeline::setMaxRecursionDepth(uint32_t depth)
    {
        m_maxRecursionDepth = depth;
    }

    Status RayTracingPipeline::build()
    {
        auto* rhiRt = getRhiPipelineAs<RHI::RayTracingPipeline>();
        if (!rhiRt)
        {
            return FailedPreconditionError("RayTracingPipeline: 底层 RHI 光追管线实例为空");
        }

        if (auto* vkRt = dynamic_cast<RHI::VK_RayTracingPipeline*>(rhiRt))
        {
            for (auto* rg : m_rayGenShaders)
            {
                FCT_CHECK_STATUS(rg->create());
                auto* vkShader = static_cast<RHI::VK_RayTracingShader*>(rg->getRhiShader());
                vkRt->addRayGenShaderStage(vkShader->getStageInfo());
            }

            for (auto* ms : m_missShaders)
            {
                FCT_CHECK_STATUS(ms->create());
                auto* vkShader = static_cast<RHI::VK_RayTracingShader*>(ms->getRhiShader());
                vkRt->addMissShaderStage(vkShader->getStageInfo());
            }

            // 规范 Vulkan 光追阶段注册顺序: RayGen -> Miss -> HitGroup -> Callable
            for (const auto& hg : m_hitGroupShaders)
            {
                vk::PipelineShaderStageCreateInfo cInfo{}, aInfo{}, iInfo{};
                const vk::PipelineShaderStageCreateInfo* pChit = nullptr;
                const vk::PipelineShaderStageCreateInfo* pAhit = nullptr;
                const vk::PipelineShaderStageCreateInfo* pInt = nullptr;

                if (hg.closestHit)
                {
                    FCT_CHECK_STATUS(hg.closestHit->create());
                    cInfo = static_cast<RHI::VK_RayTracingShader*>(hg.closestHit->getRhiShader())->getStageInfo();
                    pChit = &cInfo;
                }
                if (hg.anyHit)
                {
                    FCT_CHECK_STATUS(hg.anyHit->create());
                    aInfo = static_cast<RHI::VK_RayTracingShader*>(hg.anyHit->getRhiShader())->getStageInfo();
                    pAhit = &aInfo;
                }
                if (hg.intersection)
                {
                    FCT_CHECK_STATUS(hg.intersection->create());
                    iInfo = static_cast<RHI::VK_RayTracingShader*>(hg.intersection->getRhiShader())->getStageInfo();
                    pInt = &iInfo;
                }

                vkRt->addHitGroupStages(pChit, pAhit, pInt);
            }

            for (const auto& rawHg : m_hitGroups)
            {
                vkRt->addHitGroup(rawHg.closestHitStageIndex, rawHg.anyHitStageIndex, rawHg.intersectionStageIndex);
            }

            for (auto* call : m_callableShaders)
            {
                FCT_CHECK_STATUS(call->create());
                auto* vkShader = static_cast<RHI::VK_RayTracingShader*>(call->getRhiShader());
                vkRt->addCallableShaderStage(vkShader->getStageInfo());
            }
        }

        rhiRt->setMaxRecursionDepth(m_maxRecursionDepth);
        return rhiRt->create();
    }

    const RHI::StridedDeviceAddressRegions& RayTracingPipeline::getSBTRegions() const
    {
        auto* rhiRt = getRhiPipelineAs<RHI::RayTracingPipeline>();
        static const RHI::StridedDeviceAddressRegions emptyRegions{};
        return rhiRt ? rhiRt->getSBTRegions() : emptyRegions;
    }

    void RayTracingPipeline::traceRays(
        RHI::CommandBuffer* cmdBuf,
        uint32_t width,
        uint32_t height,
        uint32_t depth)
    {
        auto* rhiRt = getRhiPipelineAs<RHI::RayTracingPipeline>();
        if (rhiRt)
        {
            rhiRt->traceRays(cmdBuf, width, height, depth);
        }
    }
}

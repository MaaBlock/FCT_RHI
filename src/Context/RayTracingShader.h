#pragma once
#ifndef FCT_CONTEXT_RAYTRACINGSHADER_H
#define FCT_CONTEXT_RAYTRACINGSHADER_H

#include "../ThirdParty.h"
#include "./Context.h"
#include "./ShaderStage.h"
#include "./ShaderGenerate.h"
#include "../MutilThreadBase/UniquePtr.h"
#include "../RHI/IPipelineResource.h"
#include "../RHI/RayTracingShader.h"

namespace FCT
{
    /**
     * @brief 高层光线追踪着色器抽象基类
     */
    class RayTracingShader : public IPipelineResource
    {
    public:
        RayTracingShader(Context* ctx, ShaderStage stage, ShaderKind kind);
        ~RayTracingShader() override;

        ShaderStage getStage() const noexcept { return m_stage; }
        ShaderKind getKind() const noexcept { return m_kind; }

        void code(std::string source) { m_userSource = std::move(source); }
        void setBinaryCode(std::vector<char> code) { m_binaryCode = std::move(code); }
        void setEntryPoint(std::string_view entryPoint) { m_entryPoint = entryPoint; }
        const std::string& getEntryPoint() const noexcept { return m_entryPoint; }
        const std::vector<char>& getBinaryCode() const noexcept { return m_binaryCode; }

        Status compile();
        Status create();

        RHI::RayTracingShader* getRhiShader() const noexcept { return m_rhiShader; }

    protected:
        Context* m_ctx = nullptr;
        ShaderStage m_stage;
        ShaderKind m_kind;
        std::string m_userSource;
        std::string m_entryPoint = "main";
        std::vector<char> m_binaryCode;
        RHI::RayTracingShader* m_rhiShader = nullptr;
    };

    class RayGenShader : public RayTracingShader
    {
    public:
        explicit RayGenShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::RayGen, ShaderKind::RayGenShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::RayGenShader; }
    };

    class MissShader : public RayTracingShader
    {
    public:
        explicit MissShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::Miss, ShaderKind::MissShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::MissShader; }
    };

    class ClosestHitShader : public RayTracingShader
    {
    public:
        explicit ClosestHitShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::ClosestHit, ShaderKind::ClosestHitShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::ClosestHitShader; }
    };

    class AnyHitShader : public RayTracingShader
    {
    public:
        explicit AnyHitShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::AnyHit, ShaderKind::AnyHitShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::AnyHitShader; }
    };

    class IntersectionShader : public RayTracingShader
    {
    public:
        explicit IntersectionShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::Intersection, ShaderKind::IntersectionShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::IntersectionShader; }
    };

    class CallableShader : public RayTracingShader
    {
    public:
        explicit CallableShader(Context* ctx)
            : RayTracingShader(ctx, ShaderStage::Callable, ShaderKind::CallableShader) {}
        PipelineResourceType getType() const override { return PipelineResourceType::CallableShader; }
    };
}

#endif // FCT_CONTEXT_RAYTRACINGSHADER_H

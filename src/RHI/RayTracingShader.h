#pragma once
#ifndef FCT_RHI_RAYTRACINGSHADER_H
#define FCT_RHI_RAYTRACINGSHADER_H

#include <string>
#include <vector>
#include <string_view>
#include "../ThirdParty.h"
#include "./IPipelineResource.h"
#include "../Context/ShaderStage.h"

namespace FCT::RHI
{
    /**
     * @brief 底层 RHI 光线追踪着色器抽象基类
     */
    class RayTracingShader : public IPipelineResource
    {
    public:
        virtual ~RayTracingShader() = default;
        virtual ShaderStage getStage() const noexcept = 0;

        virtual void code(const std::vector<char>& code) { m_code = code; }
        virtual void code(std::vector<char>&& code) { m_code = std::move(code); }
        virtual const std::vector<char>& code() const noexcept { return m_code; }

        virtual void setEntryPoint(std::string_view entryPoint) { m_entryPoint = entryPoint; }
        virtual const std::string& getEntryPoint() const noexcept { return m_entryPoint; }

        virtual Status create() = 0;

    protected:
        std::vector<char> m_code;
        std::string m_entryPoint = "main";
    };

    class RayGenShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::RayGenShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::RayGen; }
    };

    class MissShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::MissShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::Miss; }
    };

    class ClosestHitShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::ClosestHitShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::ClosestHit; }
    };

    class AnyHitShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::AnyHitShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::AnyHit; }
    };

    class IntersectionShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::IntersectionShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::Intersection; }
    };

    class CallableShader : public RayTracingShader
    {
    public:
        PipelineResourceType getType() const override { return PipelineResourceType::CallableShader; }
        ShaderStage getStage() const noexcept override { return ShaderStage::Callable; }
    };
}

#endif // FCT_RHI_RAYTRACINGSHADER_H

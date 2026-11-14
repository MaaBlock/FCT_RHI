#include "./VK_RayTracingShader.h"
#include "../Context/VK_Context.h"

namespace FCT::RHI
{
    VK_RayTracingShader::VK_RayTracingShader(VK_Context* ctx, ShaderStage stage)
        : m_ctx(ctx), m_stage(stage)
    {
        assert(m_ctx != nullptr && "VK_Context 不能为空");
    }

    VK_RayTracingShader::~VK_RayTracingShader()
    {
        if (m_module && m_ctx)
        {
            m_ctx->getDevice().destroyShaderModule(m_module);
            m_module = nullptr;
        }
    }

    PipelineResourceType VK_RayTracingShader::getType() const
    {
        switch (m_stage)
        {
        case ShaderStage::RayGen:
            return PipelineResourceType::RayGenShader;
        case ShaderStage::Miss:
            return PipelineResourceType::MissShader;
        case ShaderStage::ClosestHit:
            return PipelineResourceType::ClosestHitShader;
        case ShaderStage::AnyHit:
            return PipelineResourceType::AnyHitShader;
        case ShaderStage::Intersection:
            return PipelineResourceType::IntersectionShader;
        case ShaderStage::Callable:
            return PipelineResourceType::CallableShader;
        default:
            return PipelineResourceType::RayGenShader;
        }
    }

    Status VK_RayTracingShader::create()
    {
        if (m_code.empty())
        {
            return InvalidArgumentError("VK_RayTracingShader: 着色器字节码为空");
        }

        if (m_code.size() % sizeof(uint32_t) != 0 || m_code.size() < sizeof(uint32_t) * 5)
        {
            return InvalidArgumentError("VK_RayTracingShader: SPIR-V 字节码长度非法或未 4 字节对齐");
        }

        const uint32_t magic = *reinterpret_cast<const uint32_t*>(m_code.data());
        if (magic != 0x07230203)
        {
            return InvalidArgumentError("VK_RayTracingShader: 非法的 SPIR-V 魔数");
        }

        if (m_module)
        {
            m_ctx->getDevice().destroyShaderModule(m_module);
            m_module = nullptr;
        }

        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = m_code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(m_code.data());

        vk::Result res = m_ctx->getDevice().createShaderModule(&createInfo, nullptr, &m_module);
        if (res != vk::Result::eSuccess)
        {
            return InternalError(StrCat("VK_RayTracingShader: 创建 vk::ShaderModule 失败: ", vk::to_string(res)));
        }

        vk::ShaderStageFlagBits vkStage{};
        switch (m_stage)
        {
        case ShaderStage::RayGen:
            vkStage = vk::ShaderStageFlagBits::eRaygenKHR;
            break;
        case ShaderStage::Miss:
            vkStage = vk::ShaderStageFlagBits::eMissKHR;
            break;
        case ShaderStage::ClosestHit:
            vkStage = vk::ShaderStageFlagBits::eClosestHitKHR;
            break;
        case ShaderStage::AnyHit:
            vkStage = vk::ShaderStageFlagBits::eAnyHitKHR;
            break;
        case ShaderStage::Intersection:
            vkStage = vk::ShaderStageFlagBits::eIntersectionKHR;
            break;
        case ShaderStage::Callable:
            vkStage = vk::ShaderStageFlagBits::eCallableKHR;
            break;
        default:
            vkStage = vk::ShaderStageFlagBits::eRaygenKHR;
            break;
        }

        m_entryPointStored = m_entryPoint.empty() ? "main" : m_entryPoint;

        m_stageInfo = vk::PipelineShaderStageCreateInfo{};
        m_stageInfo.stage = vkStage;
        m_stageInfo.module = m_module;
        m_stageInfo.pName = m_entryPointStored.c_str();

        return OkStatus();
    }
}

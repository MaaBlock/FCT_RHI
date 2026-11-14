#include "./RayTracingShader.h"
#include "./VK_Context.h"
#include "./Shaderc_ShaderCompiler.h"
#include "../RHI/VK_RayTracingShader.h"

namespace FCT
{
    RayTracingShader::RayTracingShader(Context* ctx, ShaderStage stage, ShaderKind kind)
        : m_ctx(ctx), m_stage(stage), m_kind(kind)
    {
    }

    RayTracingShader::~RayTracingShader()
    {
        if (m_rhiShader)
        {
            m_rhiShader->release();
            m_rhiShader = nullptr;
        }
    }

    Status RayTracingShader::compile()
    {
        assert(m_ctx != nullptr && "Context 不能为空");
        if (m_userSource.empty())
        {
            return InvalidArgumentError("RayTracingShader: 着色器源码为空，无法编译");
        }

        Shaderc_ShaderCompiler compiler;
        m_binaryCode = compiler.compile(m_userSource, m_entryPoint, m_kind);
        if (m_binaryCode.empty())
        {
            return InternalError("RayTracingShader: 着色器编译失败，未生成 SPIR-V 字节码");
        }

        return OkStatus();
    }

    Status RayTracingShader::create()
    {
        if (m_binaryCode.empty())
        {
            FCT_CHECK_STATUS(compile());
        }

        if (!m_rhiShader)
        {
            auto* vkCtx = dynamic_cast<VK_Context*>(m_ctx);
            if (!vkCtx)
            {
                return UnimplementedError("RayTracingShader: 当前仅支持 Vulkan 后端");
            }
            m_rhiShader = new RHI::VK_RayTracingShader(vkCtx, m_stage);
        }

        m_rhiShader->code(m_binaryCode);
        m_rhiShader->setEntryPoint(m_entryPoint);
        return m_rhiShader->create();
    }
}

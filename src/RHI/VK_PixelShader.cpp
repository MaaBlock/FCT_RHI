#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
#include "./VK_PixelShader.h"
namespace FCT
{
    namespace RHI
    {
        VK_PixelShader::VK_PixelShader(VK_Context* ctx)
            : m_ctx(ctx), m_module(VK_NULL_HANDLE)
        {

        }

        VK_PixelShader::~VK_PixelShader()
        {
            if (m_module != VK_NULL_HANDLE) {
                m_ctx->getDevice().destroyShaderModule(m_module);
            }
        }

        void VK_PixelShader::create()
        {
            createShaderModule();
            createPipelineStage();
        }

        void VK_PixelShader::createShaderModule()
        {
            if (m_code.empty()) {
                ferr << "[VK_PixelShader] 着色器字节码为空，跳过创建 VkShaderModule" << std::endl;
                return;
            }
            vk::ShaderModuleCreateInfo createInfo{};
            createInfo.codeSize = m_code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(m_code.data());
            m_module = m_ctx->getDevice().createShaderModule(createInfo);
        }

        void VK_PixelShader::createPipelineStage()
        {
            m_stageInfo.stage = vk::ShaderStageFlagBits::eFragment;
            m_stageInfo.module = m_module;
            m_stageInfo.pName = "FCTEntry";
        }
    }
}
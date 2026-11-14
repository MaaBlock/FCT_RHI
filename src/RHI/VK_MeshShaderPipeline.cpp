#include "./VK_MeshShaderPipeline.h"
#include "./VK_CommandBuffer.h"
#include "./VK_PassGroup.h"
#include "../Context/VK_Context.h"

namespace FCT::RHI
{
    VK_MeshShaderPipeline::VK_MeshShaderPipeline(VK_Context* ctx)
        : VK_RasterizationPipeline(ctx)
    {
    }

    VK_MeshShaderPipeline::~VK_MeshShaderPipeline()
    {
        if (m_pipelineLayout && m_ctx)
        {
            m_ctx->getDevice().destroyPipelineLayout(m_pipelineLayout);
            m_pipelineLayout = nullptr;
        }
        for (auto& pair : m_descriptorSetLayouts)
        {
            if (pair.second && m_ctx)
            {
                m_ctx->getDevice().destroyDescriptorSetLayout(pair.second);
            }
        }
        m_descriptorSetLayouts.clear();
    }

    void VK_MeshShaderPipeline::setTaskShader(const vk::PipelineShaderStageCreateInfo& stage)
    {
        m_shaderStages.push_back(stage);
    }

    void VK_MeshShaderPipeline::setMeshShader(const vk::PipelineShaderStageCreateInfo& stage)
    {
        m_shaderStages.push_back(stage);
        m_hasMeshShader = true;
    }

    void VK_MeshShaderPipeline::setPixelShader(const vk::PipelineShaderStageCreateInfo& stage)
    {
        m_shaderStages.push_back(stage);
    }

    void VK_MeshShaderPipeline::setShaderStages(const std::vector<vk::PipelineShaderStageCreateInfo>& stages)
    {
        m_shaderStages = stages;
        for (const auto& s : stages)
        {
            if (s.stage == vk::ShaderStageFlagBits::eMeshEXT)
            {
                m_hasMeshShader = true;
            }
        }
    }

    Status VK_MeshShaderPipeline::create()
    {
        ZoneScopedN("VK_MeshShaderPipeline::create");

        if (m_shaderStages.empty())
        {
            return InvalidArgumentError("VK_MeshShaderPipeline: 必须提供着色阶段");
        }
        for (const auto& stage : m_shaderStages)
        {
            if (!stage.module)
            {
                return FailedPreconditionError("VK_MeshShaderPipeline: 存在空的 VkShaderModule，中止创建网格图形管线");
            }
        }

        generateDefaultResources();
        FCT_CHECK_STATUS(setupCommonGraphicsState());

        // Mesh shader 管线绕过传统 IA 阶段
        m_createInfo.pVertexInputState = &m_emptyVertexInput;
        m_createInfo.pInputAssemblyState = nullptr;

        m_createInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
        m_createInfo.pStages = m_shaderStages.data();

        if (!m_pipelineLayout)
        {
            vk::PipelineLayoutCreateInfo layoutInfo{};
            auto res = m_ctx->getDevice().createPipelineLayout(&layoutInfo, nullptr, &m_pipelineLayout);
            if (res != vk::Result::eSuccess)
            {
                return InternalError(StrCat("VK_MeshShaderPipeline: 创建 PipelineLayout 失败: ", vk::to_string(res)));
            }
            m_createInfo.layout = m_pipelineLayout;
        }

        auto result = m_ctx->getDevice().createGraphicsPipeline(m_pipelineCache, m_createInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return InternalError(StrCat("VK_MeshShaderPipeline: 创建网格图形管线失败, Vulkan 错误: ", vk::to_string(result.result)));
        }

        m_pipeline = result.value;
        return OkStatus();
    }

    void VK_MeshShaderPipeline::dispatchMesh(
        CommandBuffer* cmdBuf,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ)
    {
        assert(cmdBuf != nullptr && "CommandBuffer 不能为空");
        if (!m_pipeline) return;

        auto* vkCmd = static_cast<VK_CommandBuffer*>(cmdBuf);
        vkCmd->commandBuffer().drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ);
    }
}

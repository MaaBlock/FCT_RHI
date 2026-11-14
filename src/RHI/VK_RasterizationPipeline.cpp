#include "./VK_Pass.h"
#include "./VK_PassGroup.h"
#include "./VK_RasterizationPipeline.h"
#include "./VK_CommandBuffer.h"
#include "../Context/VK_Context.h"

namespace FCT::RHI
{
    VK_RasterizationPipeline::VK_RasterizationPipeline(VK_Context* ctx) : m_createInfo{}
    {
        m_ctx = ctx;
        m_blendState = nullptr;
        m_rasterizationState = nullptr;
        m_viewportState = nullptr;
        m_inputLayout = nullptr;
        m_depthStencilState = nullptr;
        m_pass = nullptr;
    }

    VK_RasterizationPipeline::~VK_RasterizationPipeline()
    {
        if (m_pipeline && m_ctx)
        {
            m_ctx->getDevice().destroyPipeline(m_pipeline);
            m_pipeline = nullptr;
        }
        FCT_SAFE_RELEASE(m_blendState);
        FCT_SAFE_RELEASE(m_rasterizationState);
        FCT_SAFE_RELEASE(m_viewportState);
        FCT_SAFE_RELEASE(m_inputLayout);
        FCT_SAFE_RELEASE(m_depthStencilState);
    }

    void VK_RasterizationPipeline::setBlendState(BlendState* state)
    {
        FCT_SAFE_RELEASE(m_blendState);
        m_blendState = static_cast<VK_BlendState*>(state);
        if (m_blendState) m_blendState->addRef();
    }

    void VK_RasterizationPipeline::setRasterizationState(RasterizationState* state)
    {
        FCT_SAFE_RELEASE(m_rasterizationState);
        m_rasterizationState = static_cast<VK_RasterizationState*>(state);
        if (m_rasterizationState) m_rasterizationState->addRef();
    }

    void VK_RasterizationPipeline::setDepthStencilState(DepthStencilState* state)
    {
        FCT_SAFE_RELEASE(m_depthStencilState);
        m_depthStencilState = static_cast<VK_DepthStencilState*>(state);
        if (m_depthStencilState) m_depthStencilState->addRef();
    }

    void VK_RasterizationPipeline::setViewportState(ViewportState* state)
    {
        FCT_SAFE_RELEASE(m_viewportState);
        m_viewportState = static_cast<VK_ViewportState*>(state);
        if (m_viewportState) m_viewportState->addRef();
    }

    void VK_RasterizationPipeline::setInputLayout(InputLayout* layout)
    {
        FCT_SAFE_RELEASE(m_inputLayout);
        m_inputLayout = static_cast<VK_InputLayout*>(layout);
        if (m_inputLayout) m_inputLayout->addRef();
    }

    void VK_RasterizationPipeline::addResources(IPipelineResource* resource)
    {
        if (!resource)
            return;
        switch (resource->getType())
        {
        case PipelineResourceType::BlendState:
            setBlendState(static_cast<BlendState*>(resource));
            break;
        case PipelineResourceType::RasterizationState:
            setRasterizationState(static_cast<RasterizationState*>(resource));
            break;
        case PipelineResourceType::ViewportState:
            setViewportState(static_cast<ViewportState*>(resource));
            break;
        case PipelineResourceType::InputLayout:
            setInputLayout(static_cast<InputLayout*>(resource));
            break;
        case PipelineResourceType::DepthStencilState:
            setDepthStencilState(static_cast<DepthStencilState*>(resource));
            break;
        default:
            ferr << "Unsupported pipeline resource type: " << (unsigned int)(resource->getType()) << std::endl;
            break;
        }
    }

    void VK_RasterizationPipeline::bind(CommandBuffer* cmdBuf)
    {
        assert(cmdBuf != nullptr && "CommandBuffer 不能为空");
        if (m_pipeline)
        {
            auto* vkCmd = static_cast<VK_CommandBuffer*>(cmdBuf);
            vkCmd->commandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        }
    }

    Status VK_RasterizationPipeline::setupCommonGraphicsState()
    {
        if (!m_pass)
        {
            return FailedPreconditionError("VK_RasterizationPipeline: pass 必须在 create() 前通过 bindPass 绑定");
        }

        m_dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        m_dynamicState = vk::PipelineDynamicStateCreateInfo();
        m_dynamicState.setDynamicStates(m_dynamicStates);
        m_createInfo.pDynamicState = &m_dynamicState;
        m_createInfo.renderPass = static_cast<VK_PassGroup*>(m_pass->passGroup())->getRenderPass();
        m_createInfo.subpass = m_pass->index();
        m_createInfo.layout = m_pipelineLayout;
        m_createInfo.pRasterizationState = &m_rasterizationState->rasterizationStateCreateInfo();
        m_createInfo.pMultisampleState = &m_rasterizationState->multisampleStateCreateInfo();
        m_createInfo.pViewportState = &m_viewportState->viewportStateCreateInfo();
        m_createInfo.pDepthStencilState = &m_depthStencilState->getCreateInfo();

        // 对齐当前 Subpass 实际颜色附件槽位数，并对纯整数 MRT 强制关闭混合以符合 Vulkan 规范
        const vk::PipelineColorBlendStateCreateInfo& srcBlend = m_blendState->colorBlendStateCreateInfo();
        const uint32_t subpassColorCount = m_pass->getDescription().colorAttachmentCount;
        m_resolvedColorBlendAttachments.clear();
        m_resolvedColorBlendAttachments.reserve(subpassColorCount);

        const auto isIntegerColorFormat = [](Format fmt) noexcept -> bool {
            return (fmt >= Format::R8_SINT && fmt <= Format::R32G32B32A32_UINT);
        };

        auto& passTargets = m_pass->renderTargets();
        for (uint32_t slot = 0; slot < subpassColorCount; ++slot)
        {
            vk::PipelineColorBlendAttachmentState att{};
            if (slot < srcBlend.attachmentCount && srcBlend.pAttachments != nullptr)
            {
                att = srcBlend.pAttachments[slot];
            }
            else
            {
                att.blendEnable = VK_FALSE;
                att.srcColorBlendFactor = vk::BlendFactor::eOne;
                att.dstColorBlendFactor = vk::BlendFactor::eZero;
                att.colorBlendOp = vk::BlendOp::eAdd;
                att.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                att.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                att.alphaBlendOp = vk::BlendOp::eAdd;
                att.colorWriteMask =
                    vk::ColorComponentFlagBits::eR |
                    vk::ColorComponentFlagBits::eG |
                    vk::ColorComponentFlagBits::eB |
                    vk::ColorComponentFlagBits::eA;
            }

            auto itTarget = passTargets.find(slot);
            if (itTarget != passTargets.end() && itTarget->second != nullptr &&
                isIntegerColorFormat(itTarget->second->format()))
            {
                att.blendEnable = VK_FALSE;
            }
            m_resolvedColorBlendAttachments.push_back(att);
        }

        m_resolvedColorBlendState = srcBlend;
        m_resolvedColorBlendState.setAttachmentCount(subpassColorCount);
        m_resolvedColorBlendState.setPAttachments(
            subpassColorCount > 0 ? m_resolvedColorBlendAttachments.data() : nullptr);
        m_createInfo.pColorBlendState = &m_resolvedColorBlendState;

        return OkStatus();
    }

    Status VK_RasterizationPipeline::create()
    {
        if (m_shaderStages.empty())
        {
            return FailedPreconditionError("VK_RasterizationPipeline: 未配置任何有效着色阶段");
        }
        for (const auto& stage : m_shaderStages)
        {
            if (!stage.module)
            {
                return FailedPreconditionError("VK_RasterizationPipeline: 存在空的 VkShaderModule，中止创建图形管线");
            }
        }

        generateDefaultResources();
        FCT_CHECK_STATUS(setupCommonGraphicsState());

        m_createInfo.pVertexInputState = m_inputLayout->pInputStateInfo();
        m_inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo{};
        m_inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
        m_inputAssemblyState.primitiveRestartEnable = VK_FALSE;
        m_createInfo.pInputAssemblyState = &m_inputAssemblyState;

        m_createInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
        m_createInfo.pStages = m_shaderStages.data();

        auto result = m_ctx->getDevice().createGraphicsPipeline(m_pipelineCache, m_createInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return InternalError(StrCat("VK_RasterizationPipeline: 创建图形管线失败, Vulkan 错误: ", vk::to_string(result.result)));
        }
        m_pipeline = result.value;
        return OkStatus();
    }

    void VK_RasterizationPipeline::bindPass(Pass* pass)
    {
        m_pass = static_cast<VK_Pass*>(pass);
    }

    void VK_RasterizationPipeline::generateDefaultResources()
    {
        if (!m_blendState)
        {
            m_blendState = new VK_BlendState(m_ctx);
            if (m_pass)
            {
                if (!m_pass->renderTargets().empty())
                {
                    m_blendState->targets(static_cast<int>(m_pass->renderTargets().rbegin()->first + 1));
                }
                else
                {
                    m_blendState->targets(0);
                }
            }
            m_blendState->create();
        }
        if (!m_rasterizationState)
        {
            m_rasterizationState = new VK_RasterizationState(m_ctx);
            m_rasterizationState->create();
        }
        if (!m_viewportState)
        {
            m_viewportState = new VK_ViewportState(m_ctx);
            m_viewportState->create();
        }
        if (!m_inputLayout)
        {
            m_inputLayout = new VK_InputLayout(m_ctx);
            m_inputLayout->create();
        }
        if (!m_depthStencilState)
        {
            m_depthStencilState = new VK_DepthStencilState(m_ctx);
            m_depthStencilState->create();
        }
    }
}

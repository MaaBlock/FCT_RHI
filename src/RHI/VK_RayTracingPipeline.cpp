#include "./VK_RayTracingPipeline.h"
#include "./VK_CommandBuffer.h"
#include "./VK_StorageBuffer.h"
#include "../Context/VK_Context.h"
#include <cstring>
#include <algorithm>

namespace FCT::RHI
{
    template <typename T>
    static inline constexpr T alignUp(T value, T alignment) noexcept
    {
        return (alignment > 0) ? ((value + alignment - 1) & ~(alignment - 1)) : value;
    }

    VK_RayTracingPipeline::VK_RayTracingPipeline(VK_Context* ctx)
        : m_ctx(ctx)
    {
        assert(m_ctx != nullptr && "VK_Context 不能为空");
    }

    VK_RayTracingPipeline::~VK_RayTracingPipeline()
    {
        if (m_pipeline && m_ctx)
        {
            m_ctx->getDevice().destroyPipeline(m_pipeline);
            m_pipeline = nullptr;
        }
        if (m_pipelineLayout && m_ctx && !m_externalLayout)
        {
            m_ctx->getDevice().destroyPipelineLayout(m_pipelineLayout);
            m_pipelineLayout = nullptr;
        }
        if (m_sbtBuffer)
        {
            m_sbtBuffer->release();
            m_sbtBuffer = nullptr;
        }
    }

    void VK_RayTracingPipeline::addRayGenShaderStage(const vk::PipelineShaderStageCreateInfo& stage)
    {
        uint32_t stageIdx = static_cast<uint32_t>(m_stages.size());
        m_stages.push_back(stage);

        vk::RayTracingShaderGroupCreateInfoKHR group{};
        group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        group.generalShader = stageIdx;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        uint32_t groupIndex = static_cast<uint32_t>(m_shaderGroups.size());
        m_shaderGroups.push_back(group);
        m_rayGenGroupIndices.push_back(groupIndex);
        m_rayGenGroupCount = static_cast<uint32_t>(m_rayGenGroupIndices.size());
    }

    void VK_RayTracingPipeline::addMissShaderStage(const vk::PipelineShaderStageCreateInfo& stage)
    {
        uint32_t stageIdx = static_cast<uint32_t>(m_stages.size());
        m_stages.push_back(stage);

        vk::RayTracingShaderGroupCreateInfoKHR group{};
        group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        group.generalShader = stageIdx;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        uint32_t groupIndex = static_cast<uint32_t>(m_shaderGroups.size());
        m_shaderGroups.push_back(group);
        m_missGroupIndices.push_back(groupIndex);
        m_missGroupCount = static_cast<uint32_t>(m_missGroupIndices.size());
    }

    void VK_RayTracingPipeline::addCallableShaderStage(const vk::PipelineShaderStageCreateInfo& stage)
    {
        uint32_t stageIdx = static_cast<uint32_t>(m_stages.size());
        m_stages.push_back(stage);

        vk::RayTracingShaderGroupCreateInfoKHR group{};
        group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
        group.generalShader = stageIdx;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        uint32_t groupIndex = static_cast<uint32_t>(m_shaderGroups.size());
        m_shaderGroups.push_back(group);
        m_callableGroupIndices.push_back(groupIndex);
        m_callableGroupCount = static_cast<uint32_t>(m_callableGroupIndices.size());
    }

    void VK_RayTracingPipeline::addHitGroup(
        uint32_t closestHitStageIndex,
        uint32_t anyHitStageIndex,
        uint32_t intersectionStageIndex)
    {
        vk::RayTracingShaderGroupCreateInfoKHR group{};
        group.type = (intersectionStageIndex != VK_SHADER_UNUSED_KHR)
            ? vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup
            : vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = closestHitStageIndex;
        group.anyHitShader = anyHitStageIndex;
        group.intersectionShader = intersectionStageIndex;

        uint32_t groupIndex = static_cast<uint32_t>(m_shaderGroups.size());
        m_shaderGroups.push_back(group);
        m_hitGroupIndices.push_back(groupIndex);
        m_hitGroupCount = static_cast<uint32_t>(m_hitGroupIndices.size());
    }

    void VK_RayTracingPipeline::addHitGroupStages(
        const vk::PipelineShaderStageCreateInfo* closestHit,
        const vk::PipelineShaderStageCreateInfo* anyHit,
        const vk::PipelineShaderStageCreateInfo* intersection)
    {
        uint32_t chitIdx = VK_SHADER_UNUSED_KHR;
        uint32_t ahitIdx = VK_SHADER_UNUSED_KHR;
        uint32_t intIdx = VK_SHADER_UNUSED_KHR;

        if (closestHit)
        {
            chitIdx = static_cast<uint32_t>(m_stages.size());
            m_stages.push_back(*closestHit);
        }
        if (anyHit)
        {
            ahitIdx = static_cast<uint32_t>(m_stages.size());
            m_stages.push_back(*anyHit);
        }
        if (intersection)
        {
            intIdx = static_cast<uint32_t>(m_stages.size());
            m_stages.push_back(*intersection);
        }

        addHitGroup(chitIdx, ahitIdx, intIdx);
    }

    Status VK_RayTracingPipeline::setLayouts(
        const std::vector<vk::DescriptorSetLayout>& layouts,
        const std::vector<vk::PushConstantRange>& pushConstants)
    {
        if (m_pipelineLayout && !m_externalLayout)
        {
            m_ctx->getDevice().destroyPipelineLayout(m_pipelineLayout);
            m_pipelineLayout = nullptr;
        }

        vk::PipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        layoutInfo.pSetLayouts = layouts.empty() ? nullptr : layouts.data();
        layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
        layoutInfo.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

        vk::Result res = m_ctx->getDevice().createPipelineLayout(&layoutInfo, nullptr, &m_pipelineLayout);
        if (res != vk::Result::eSuccess)
        {
            return InternalError(StrCat("VK_RayTracingPipeline: 创建自定义光追管线布局失败: ", vk::to_string(res)));
        }
        m_externalLayout = false;
        return OkStatus();
    }

    Status VK_RayTracingPipeline::createRayTracingPipelineLayout()
    {
        if (m_pipelineLayout)
        {
            return OkStatus();
        }

        vk::PipelineLayoutCreateInfo layoutInfo{};
        vk::Result res = m_ctx->getDevice().createPipelineLayout(&layoutInfo, nullptr, &m_pipelineLayout);
        if (res != vk::Result::eSuccess)
        {
            return InternalError(StrCat("VK_RayTracingPipeline: 创建光追管线布局失败: ", vk::to_string(res)));
        }
        m_externalLayout = false;
        return OkStatus();
    }

    Status VK_RayTracingPipeline::create()
    {
        ZoneScopedN("VK_RayTracingPipeline::create");

        if (m_stages.empty() || m_shaderGroups.empty())
        {
            return InvalidArgumentError("VK_RayTracingPipeline: 缺少着色阶段或着色组");
        }

        if (m_rayGenGroupIndices.empty())
        {
            return InvalidArgumentError("VK_RayTracingPipeline: 必须包含至少一个 RayGen 着色组");
        }

        FCT_CHECK_STATUS(createRayTracingPipelineLayout());

        VkRayTracingPipelineCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        createInfo.stageCount = static_cast<uint32_t>(m_stages.size());
        createInfo.pStages = reinterpret_cast<const VkPipelineShaderStageCreateInfo*>(m_stages.data());
        createInfo.groupCount = static_cast<uint32_t>(m_shaderGroups.size());
        createInfo.pGroups = reinterpret_cast<const VkRayTracingShaderGroupCreateInfoKHR*>(m_shaderGroups.data());
        createInfo.maxPipelineRayRecursionDepth = m_maxRecursionDepth;
        createInfo.layout = static_cast<VkPipelineLayout>(m_pipelineLayout);

        if (!VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateRayTracingPipelinesKHR)
        {
            return UnimplementedError("VK_RayTracingPipeline: 设备未启用 VK_KHR_ray_tracing_pipeline 扩展");
        }

        VkPipeline rawPipeline = VK_NULL_HANDLE;
        VkResult res = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateRayTracingPipelinesKHR(
            static_cast<VkDevice>(m_ctx->getDevice()),
            VK_NULL_HANDLE,
            VK_NULL_HANDLE,
            1,
            &createInfo,
            nullptr,
            &rawPipeline);

        if (res != VK_SUCCESS)
        {
            return InternalError(StrCat("VK_RayTracingPipeline: 创建光追管线失败, 错误码: ", static_cast<int>(res)));
        }

        m_pipeline = rawPipeline;

        // 管线创建成功后立即构建 SBT
        FCT_CHECK_STATUS(buildSBT());
        return OkStatus();
    }

    Status VK_RayTracingPipeline::buildSBT()
    {
        ZoneScopedN("VK_RayTracingPipeline::buildSBT");
        if (!m_pipeline)
        {
            return FailedPreconditionError("VK_RayTracingPipeline: 管线尚未创建，无法构建 SBT");
        }

        // 默认光追硬件对齐限制 (RTX / DXR 标准: handle 32, baseAlign 64)
        uint32_t handleSize = 32;
        uint32_t handleAlignment = 32;
        uint32_t baseAlignment = 64;

        // 使用 Vulkan C API 非抛异常机制安全查询物理设备光线追踪管线属性 (VkPhysicalDeviceRayTracingPipelinePropertiesKHR)
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
        rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 props2{};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props2.pNext = &rtProps;

        auto pfnGetProps2 = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties2;
        if (!pfnGetProps2)
        {
            pfnGetProps2 = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties2KHR;
        }

        if (pfnGetProps2)
        {
            pfnGetProps2(static_cast<VkPhysicalDevice>(m_ctx->getPhysicalDevice()), &props2);
            if (rtProps.shaderGroupHandleSize > 0)
            {
                handleSize = rtProps.shaderGroupHandleSize;
                handleAlignment = rtProps.shaderGroupHandleAlignment;
                baseAlignment = rtProps.shaderGroupBaseAlignment;
            }
        }

        const uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);
        const uint32_t totalGroupCount = static_cast<uint32_t>(m_shaderGroups.size());
        const uint32_t sbtDataSize = totalGroupCount * handleSize;
        std::vector<uint8_t> handles(sbtDataSize);

        if (!VULKAN_HPP_DEFAULT_DISPATCHER.vkGetRayTracingShaderGroupHandlesKHR)
        {
            return UnimplementedError("VK_RayTracingPipeline: 缺少 vkGetRayTracingShaderGroupHandlesKHR 驱动支持");
        }

        VkResult getHandlesResult = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetRayTracingShaderGroupHandlesKHR(
            static_cast<VkDevice>(m_ctx->getDevice()),
            static_cast<VkPipeline>(m_pipeline),
            0,
            totalGroupCount,
            sbtDataSize,
            handles.data());

        if (getHandlesResult != VK_SUCCESS)
        {
            return InternalError(StrCat("VK_RayTracingPipeline: 提取着色组句柄失败, 错误码: ", static_cast<int>(getHandlesResult)));
        }

        const uint32_t rayGenSize = alignUp(m_rayGenGroupCount * handleSizeAligned, baseAlignment);
        const uint32_t missSize = alignUp(m_missGroupCount * handleSizeAligned, baseAlignment);
        const uint32_t hitSize = alignUp(m_hitGroupCount * handleSizeAligned, baseAlignment);
        const uint32_t callableSize = alignUp(m_callableGroupCount * handleSizeAligned, baseAlignment);

        const uint32_t rayGenOffset = 0;
        const uint32_t missOffset = rayGenOffset + rayGenSize;
        const uint32_t hitOffset = missOffset + missSize;
        const uint32_t callableOffset = hitOffset + hitSize;
        const uint32_t totalBufferSize = callableOffset + callableSize;

        if (m_sbtBuffer)
        {
            m_sbtBuffer->release();
            m_sbtBuffer = nullptr;
        }
        m_sbtBuffer = m_ctx->createStorageBuffer(totalBufferSize);
        if (!m_sbtBuffer)
        {
            return InternalError("VK_RayTracingPipeline: 分配 SBT GPU 缓冲区失败");
        }

        uint8_t* mappedData = static_cast<uint8_t*>(m_sbtBuffer->map());
        if (!mappedData)
        {
            return InternalError("VK_RayTracingPipeline: 映射 SBT 缓冲区失败");
        }

        std::memset(mappedData, 0, totalBufferSize);

        // 使用显式记录的 groupIndex 精确查找句柄，杜绝阶段注册顺序导致的句柄倒置风险
        for (size_t i = 0; i < m_rayGenGroupIndices.size(); ++i)
        {
            uint32_t groupIdx = m_rayGenGroupIndices[i];
            std::memcpy(mappedData + rayGenOffset + i * handleSizeAligned,
                        handles.data() + groupIdx * handleSize, handleSize);
        }
        for (size_t i = 0; i < m_missGroupIndices.size(); ++i)
        {
            uint32_t groupIdx = m_missGroupIndices[i];
            std::memcpy(mappedData + missOffset + i * handleSizeAligned,
                        handles.data() + groupIdx * handleSize, handleSize);
        }
        for (size_t i = 0; i < m_hitGroupIndices.size(); ++i)
        {
            uint32_t groupIdx = m_hitGroupIndices[i];
            std::memcpy(mappedData + hitOffset + i * handleSizeAligned,
                        handles.data() + groupIdx * handleSize, handleSize);
        }
        for (size_t i = 0; i < m_callableGroupIndices.size(); ++i)
        {
            uint32_t groupIdx = m_callableGroupIndices[i];
            std::memcpy(mappedData + callableOffset + i * handleSizeAligned,
                        handles.data() + groupIdx * handleSize, handleSize);
        }
        m_sbtBuffer->unmap();

        uint64_t sbtDeviceAddress = m_sbtBuffer->getDeviceAddress();

        if (m_rayGenGroupCount > 0)
        {
            m_sbtRegions.rayGen.deviceAddress = sbtDeviceAddress + rayGenOffset;
            m_sbtRegions.rayGen.stride = handleSizeAligned;
            m_sbtRegions.rayGen.size = handleSizeAligned;
        }
        else
        {
            m_sbtRegions.rayGen = {};
        }

        if (m_missGroupCount > 0)
        {
            m_sbtRegions.miss.deviceAddress = sbtDeviceAddress + missOffset;
            m_sbtRegions.miss.stride = handleSizeAligned;
            m_sbtRegions.miss.size = m_missGroupCount * handleSizeAligned;
        }
        else
        {
            m_sbtRegions.miss = {};
        }

        if (m_hitGroupCount > 0)
        {
            m_sbtRegions.hitGroup.deviceAddress = sbtDeviceAddress + hitOffset;
            m_sbtRegions.hitGroup.stride = handleSizeAligned;
            m_sbtRegions.hitGroup.size = m_hitGroupCount * handleSizeAligned;
        }
        else
        {
            m_sbtRegions.hitGroup = {};
        }

        if (m_callableGroupCount > 0)
        {
            m_sbtRegions.callable.deviceAddress = sbtDeviceAddress + callableOffset;
            m_sbtRegions.callable.stride = handleSizeAligned;
            m_sbtRegions.callable.size = m_callableGroupCount * handleSizeAligned;
        }
        else
        {
            m_sbtRegions.callable = {};
        }

        return OkStatus();
    }

    void VK_RayTracingPipeline::bind(CommandBuffer* cmdBuf)
    {
        assert(cmdBuf != nullptr && "CommandBuffer 不能为空");
        if (!m_pipeline) return;

        auto* vkCmd = static_cast<VK_CommandBuffer*>(cmdBuf);
        vkCmd->commandBuffer().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);
    }

    void VK_RayTracingPipeline::traceRays(
        CommandBuffer* cmdBuf,
        uint32_t width,
        uint32_t height,
        uint32_t depth)
    {
        assert(cmdBuf != nullptr && "CommandBuffer 不能为空");
        if (!m_pipeline) return;

        cmdBuf->traceRays(m_sbtRegions, width, height, depth);
    }
}

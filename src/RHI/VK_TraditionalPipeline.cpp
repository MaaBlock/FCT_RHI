//
// Created by Administrator on 2025/3/24.
//
#include "../Context/VK_Context.h"
#include "VK_TraditionalPipeline.h"
#include "../Context/VertexShader.h"
#include "../Context/PixelShader.h"
#include "./VK_VertexShader.h"
#include "./VK_PixelShader.h"
#include "./VK_Pass.h"
#include "../Context/Context.hpp"
namespace FCT
{
    namespace RHI
    {


        VK_TraditionalPipeline::VK_TraditionalPipeline(VK_Context* ctx) : VK_RasterizationPipeline(ctx)
        {
            m_pixelShader = nullptr;
            m_vertexShader = nullptr;
        }

        VK_TraditionalPipeline::~VK_TraditionalPipeline()
        {
            if (m_vertexShader)
            {
                m_vertexShader->release();
                m_vertexShader = nullptr;
            }
            if (m_pixelShader)
            {
                m_pixelShader->release();
                m_pixelShader = nullptr;
            }

            if (m_pipelineLayout)
            {
                m_ctx->getDevice().destroyPipelineLayout(m_pipelineLayout);
            }

            for(auto& pair : m_descriptorSetLayouts)
            {
                if (m_ctx && pair.second == m_ctx->getBindlessSetLayout()) {
                    continue;
                }
                m_ctx->getDevice().destroyDescriptorSetLayout(pair.second);
            }
            m_descriptorSetLayouts.clear();

            if (m_emptyLayout)
            {
                m_ctx->getDevice().destroyDescriptorSetLayout(m_emptyLayout);
            }
        }

        void VK_TraditionalPipeline::addResources(IPipelineResource* resource)
        {
            if (!resource)
                return;
            switch (resource->getType())
            {
            case PipelineResourceType::VertexShader:
                m_vertexShader = static_cast<FCT::VertexShader*>(resource);
                m_vertexShader->addRef();
                /*
                m_vertexShader = static_cast<VK_VertexShader*>(static_cast<FCT::VertexShader*>(resource)->vertexShader());
                m_vertexShader->addRef();
                */
                break;
            case PipelineResourceType::PixelShader:
                m_pixelShader = static_cast<FCT::PixelShader*>(resource);
                m_pixelShader->addRef();
                /*
                m_pixelShader = static_cast<VK_PixelShader*>(static_cast<FCT::PixelShader*>(resource)->pixelShader());
                m_pixelShader->addRef();*/
                break;
            default:
                VK_RasterizationPipeline::addResources(resource);
                break;
            }
        }

        Status VK_TraditionalPipeline::create()
        {
            ZoneScoped();
            generateDefaultResources();
            FCT_CHECK_STATUS(createDescriptorSetLayouts());

            m_shaderStages.clear();
            if (m_vertexShader && m_vertexShader->vertexShader())
            {
                const auto stageInfo = static_cast<VK_VertexShader*>(m_vertexShader->vertexShader())->getStageInfo();
                if (!stageInfo.module)
                {
                    return FailedPreconditionError("VK_TraditionalPipeline: 顶点着色器模块为空，中止创建管线");
                }
                m_shaderStages.push_back(stageInfo);
            }
            if (m_pixelShader && m_pixelShader->pixelShader())
            {
                const auto stageInfo = static_cast<VK_PixelShader*>(m_pixelShader->pixelShader())->getStageInfo();
                if (!stageInfo.module)
                {
                    return FailedPreconditionError("VK_TraditionalPipeline: 像素着色器模块为空，中止创建管线");
                }
                m_shaderStages.push_back(stageInfo);
            }

            return VK_RasterizationPipeline::create();
        }
        void VK_TraditionalPipeline::generateDefaultResources()
        {
            ZoneScoped();
            FCT::VertexShader* vs = nullptr;
            if (!m_vertexShader)
            {
                m_vertexShader = m_ctx->createResource<FCT::VertexShader>();
                for (auto pair : m_vertexLayouts)
                {
                    m_vertexShader->addLayout(pair.first, pair.second);
                }
                m_vertexShader->resourceLayout(m_resourceLayout);
                m_vertexShader->pixelLayout(m_pixelLayout);
                m_vertexShader->create();
            }
            if (!m_pixelShader && (!m_pass || !m_pass->renderTargets().empty()))
            {
                m_pixelShader = m_ctx->createResource<FCT::PixelShader>();
                m_pixelShader->pixelLayout(m_pixelLayout);
                m_pixelShader->resourceLayout(m_resourceLayout);
                m_pixelShader->create();
            }
            if (!m_inputLayout)
            {
                m_inputLayout = static_cast<VK_InputLayout*>(m_vertexShader->createBindedInputLayout());
            }
        }
        Status VK_TraditionalPipeline::createDescriptorSetLayouts()
        {
            ZoneScoped();
            std::map<uint32_t, std::set<uint32_t>> usedBindings;
            std::map<uint32_t, std::map<std::string, uint32_t>> resourceBindings; // 记录每个资源名称对应的binding
            std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> setBindings;

            if (m_vertexShader)
            {
                auto shaderBinary = m_vertexShader->binaryCode();

                auto constBufferLocations = shaderBinary.constBufferLocations();
                auto constBufferLayouts = shaderBinary.constBufferLayout();

                for (const auto& [name, location] : constBufferLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = constBufferLayouts.find(name);
                    if (layoutIt == constBufferLayouts.end()) {
                        continue;
                    }

                    const ConstLayout& layout = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(layout.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eUniformBuffer,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }

                auto textureLocations = shaderBinary.textureLocations();
                auto textureLayouts = shaderBinary.textureLayout();

                for (const auto& [name, location] : textureLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (usedBindings[set].count(binding)) {
                        fout << "Warning: Binding collision for resource '" << name << "' in set " << set << ". Original binding " << binding << " is already in use. Remapping to a new binding." << std::endl;
                        binding = 0;
                        while (usedBindings[set].count(binding)) {
                            binding++;
                        }
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = textureLayouts.find(name);
                    if (layoutIt == textureLayouts.end()) {
                        continue;
                    }

                    const TextureElement& texture = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(texture.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eSampledImage,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }

                auto samplerLocations = shaderBinary.samplerLocations();
                auto samplerLayouts = shaderBinary.samplerLayout();

                for (const auto& [name, location] : samplerLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (usedBindings[set].count(binding)) {
                        fout << "Warning: Binding collision for resource '" << name << "' in set " << set << ". Original binding " << binding << " is already in use. Remapping to a new binding." << std::endl;
                        binding = 0;
                        while (usedBindings[set].count(binding)) {
                            binding++;
                        }
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = samplerLayouts.find(name);
                    if (layoutIt == samplerLayouts.end()) {
                        continue;
                    }

                    const SamplerElement& sampler = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(sampler.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eSampler,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }
            }

            if (m_pixelShader)
            {
                auto shaderBinary = m_pixelShader->binaryCode();

                auto constBufferLocations = shaderBinary.constBufferLocations();
                auto constBufferLayouts = shaderBinary.constBufferLayout();

                for (const auto& [name, location] : constBufferLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (usedBindings[set].count(binding)) {
                        fout << "Warning: Binding collision for resource '" << name << "' in set " << set << ". Original binding " << binding << " is already in use. Remapping to a new binding." << std::endl;
                        binding = 0;
                        while (usedBindings[set].count(binding)) {
                            binding++;
                        }
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = constBufferLayouts.find(name);
                    if (layoutIt == constBufferLayouts.end()) {
                        continue;
                    }

                    const ConstLayout& layout = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(layout.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eUniformBuffer,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }

                auto textureLocations = shaderBinary.textureLocations();
                auto textureLayouts = shaderBinary.textureLayout();

                for (const auto& [name, location] : textureLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (usedBindings[set].count(binding)) {
                        fout << "Warning: Binding collision for resource '" << name << "' in set " << set << ". Original binding " << binding << " is already in use. Remapping to a new binding." << std::endl;
                        binding = 0;
                        while (usedBindings[set].count(binding)) {
                            binding++;
                        }
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = textureLayouts.find(name);
                    if (layoutIt == textureLayouts.end()) {
                        continue;
                    }

                    const TextureElement& texture = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(texture.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eSampledImage,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }

                auto samplerLocations = shaderBinary.samplerLocations();
                auto samplerLayouts = shaderBinary.samplerLayout();

                for (const auto& [name, location] : samplerLocations)
                {
                    uint32_t set = location.first;
                    uint32_t binding = location.second;

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (resourceBindings[set].find(name) != resourceBindings[set].end()) {
                        continue;
                    }

                    if (usedBindings[set].count(binding)) {
                        fout << "Warning: Binding collision for resource '" << name << "' in set " << set << ". Original binding " << binding << " is already in use. Remapping to a new binding." << std::endl;
                        binding = 0;
                        while (usedBindings[set].count(binding)) {
                            binding++;
                        }
                    }

                    resourceBindings[set][name] = binding;
                    usedBindings[set].insert(binding);

                    auto layoutIt = samplerLayouts.find(name);
                    if (layoutIt == samplerLayouts.end()) {
                        continue;
                    }

                    const SamplerElement& sampler = layoutIt->second;
                    vk::ShaderStageFlags stageFlags = ConvertToVkShaderStageFlags(sampler.getShaderStages());

                    vk::DescriptorSetLayoutBinding layoutBinding(
                        binding,
                        vk::DescriptorType::eSampler,
                        1,
                        stageFlags,
                        nullptr
                    );

                    setBindings[set].push_back(layoutBinding);
                }
            }

            bool isBindless = false;
            if (m_ctx && m_ctx->isBindlessSupported() && m_ctx->getBindlessSetLayout()) {
                if (m_pixelShader && (m_pixelShader->userSource().find("BindlessCommon.hlsli") != std::string::npos ||
                                      m_pixelShader->source().find("BindlessCommon.hlsli") != std::string::npos)) {
                    isBindless = true;
                }
                if (m_vertexShader && (m_vertexShader->userSource().find("BindlessCommon.hlsli") != std::string::npos ||
                                       m_vertexShader->source().find("BindlessCommon.hlsli") != std::string::npos)) {
                    isBindless = true;
                }
            }

            m_descriptorSetLayouts.clear();
            for (const auto& [set, bindings] : setBindings) {
                if (set == 0 && isBindless) {
                    continue;
                }
                vk::DescriptorSetLayoutCreateInfo layoutInfo(
                    vk::DescriptorSetLayoutCreateFlags(),
                    static_cast<uint32_t>(bindings.size()),
                    bindings.data()
                );

                vk::DescriptorSetLayout layout;
                vk::Result res = m_ctx->getDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &layout);
                if (res != vk::Result::eSuccess) {
                    for (auto& pair : m_descriptorSetLayouts) {
                        if (pair.second != m_ctx->getBindlessSetLayout()) {
                            m_ctx->getDevice().destroyDescriptorSetLayout(pair.second);
                        }
                    }
                    m_descriptorSetLayouts.clear();
                    return InternalError(StrCat("VK_TraditionalPipeline: 创建描述符集布局失败 (Set ", set, "), Vulkan 错误: ", vk::to_string(res)));
                }
                m_descriptorSetLayouts[set] = layout;
            }

            if (isBindless) {
                m_descriptorSetLayouts[0] = m_ctx->getBindlessSetLayout();
            }

            m_descriptorSetLayoutsArr.clear();
            if (!m_descriptorSetLayouts.empty()) {
                uint32_t maxSet = 0;
                for (const auto& [set, layout] : m_descriptorSetLayouts) {
                    maxSet = std::max(maxSet, set);
                }

                vk::DescriptorSetLayoutCreateInfo emptyLayoutInfo(
                    vk::DescriptorSetLayoutCreateFlags(),
                    0,
                    nullptr
                );
                vk::DescriptorSetLayout emptyLayout;
                vk::Result res = m_ctx->getDevice().createDescriptorSetLayout(&emptyLayoutInfo, nullptr, &emptyLayout);
                if (res != vk::Result::eSuccess) {
                    for (auto& pair : m_descriptorSetLayouts) {
                        if (pair.second != m_ctx->getBindlessSetLayout()) {
                            m_ctx->getDevice().destroyDescriptorSetLayout(pair.second);
                        }
                    }
                    m_descriptorSetLayouts.clear();
                    return InternalError(StrCat("VK_TraditionalPipeline: 创建空描述符集布局失败, Vulkan 错误: ", vk::to_string(res)));
                }

                m_descriptorSetLayoutsArr.resize(maxSet + 1, emptyLayout);

                for (const auto& [set, layout] : m_descriptorSetLayouts) {
                    m_descriptorSetLayoutsArr[set] = layout;
                }

                m_emptyLayout = emptyLayout;
            }

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
                vk::PipelineLayoutCreateFlags(),
                static_cast<uint32_t>(m_descriptorSetLayoutsArr.size()),
                m_descriptorSetLayoutsArr.empty() ? nullptr : m_descriptorSetLayoutsArr.data(),
                0,
                nullptr
            );

            vk::Result res = m_ctx->getDevice().createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_pipelineLayout);
            if (res != vk::Result::eSuccess) {
                for (auto& pair : m_descriptorSetLayouts) {
                    if (pair.second != m_ctx->getBindlessSetLayout()) {
                        m_ctx->getDevice().destroyDescriptorSetLayout(pair.second);
                    }
                }
                m_descriptorSetLayouts.clear();
                if (m_emptyLayout) {
                    m_ctx->getDevice().destroyDescriptorSetLayout(m_emptyLayout);
                    m_emptyLayout = nullptr;
                }
                return InternalError(StrCat("VK_TraditionalPipeline: 创建管线布局失败, Vulkan 错误: ", vk::to_string(res)));
            }

            return OkStatus();
        }
    }
}

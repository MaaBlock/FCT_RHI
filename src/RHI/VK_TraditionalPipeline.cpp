//
// Created by Administrator on 2025/3/24.
//
#include "../Context/VK_Context.h"
#include "VK_TraditionalPipeline.h"
#include "../Context/VertexShader.h"
#include "../Context/PixelShader.h"
#include "./VK_VertexShader.h"
#include "./VK_PixelShader.h"
#include "../Context/Context.hpp"
namespace FCT
{
    namespace RHI
    {
        PipelineType VK_TraditionalPipeline::getType() const
        {
            return PipelineType::Traditional;
        }

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

        void VK_TraditionalPipeline::create()
        {
            ZoneScoped();
            generateDefaultResources();
            createDescriptorSetLayouts();
            m_shaderStages.push_back(static_cast<VK_VertexShader*>(m_vertexShader->vertexShader())->getStageInfo());
            m_shaderStages.push_back(static_cast<VK_PixelShader*>(m_pixelShader->pixelShader())->getStageInfo());
            VK_RasterizationPipeline::create();
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
            if (!m_pixelShader)
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
        void VK_TraditionalPipeline::createDescriptorSetLayouts()
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

            m_descriptorSetLayouts.clear();
            for (const auto& [set, bindings] : setBindings) {
                vk::DescriptorSetLayoutCreateInfo layoutInfo(
                    vk::DescriptorSetLayoutCreateFlags(),
                    static_cast<uint32_t>(bindings.size()),
                    bindings.data()
                );

                try {
                    m_descriptorSetLayouts[set] = m_ctx->getDevice().createDescriptorSetLayout(layoutInfo);
                } catch (const vk::SystemError& e) {
                    throw std::runtime_error("Failed to create descriptor set layout: " + std::string(e.what()));
                }
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
                try {
                    emptyLayout = m_ctx->getDevice().createDescriptorSetLayout(emptyLayoutInfo);
                } catch (const vk::SystemError& e) {
                    throw std::runtime_error("Failed to create empty descriptor set layout: " + std::string(e.what()));
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

            try {
                m_pipelineLayout = m_ctx->getDevice().createPipelineLayout(pipelineLayoutInfo);
            } catch (const vk::SystemError& e) {
                throw std::runtime_error("Failed to create pipeline layout: " + std::string(e.what()));
            }
        }
    }
}

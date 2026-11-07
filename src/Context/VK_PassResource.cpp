//
// Created by Administrator on 2025/4/14.
//
#include "../FCTAPI.h"
namespace FCT
{
    VK_PassResource::VK_PassResource(VK_Context* ctx)
    {
        m_ctx = ctx;
    }

    VK_PassResource::~VK_PassResource()
    {
        for (auto& set : m_descriptorSets)
        {
            m_ctx->device().
                freeDescriptorSets(
                     static_cast<RHI::VK_DescriptorPool*>(m_ctx->getDescriptorPool())->getPool(),set);
        }
        for (auto& layout : m_descriptorSetLayouts)
        {
            m_ctx->device().destroyDescriptorSetLayout(layout);
        }
    }


    void VK_PassResource::addConstBuffer(RHI::ConstBuffer* buffer)
    {
        m_constBuffers.push_back(buffer);
        markAllDescriptorSetsDirty();
        markAllDescriptorSetsNeedRecreate();
    }

    void VK_PassResource::addSampler(Sampler* sampler, SamplerElement element)
    {
        m_samplers.emplace_back(sampler, element);
        markAllDescriptorSetsDirty();
        markAllDescriptorSetsNeedRecreate();
    }

    void VK_PassResource::addTexture(Image* texture, TextureElement element)
    {
        m_textures[element] = texture;
        m_textureViewHashes[element] = texture ? texture->textureViewHash() : 0;
        markAllDescriptorSetsDirty();
        markAllDescriptorSetsNeedRecreate();
    }

    void VK_PassResource::setTexture(Image* texture, TextureElement element)
    {
        if (texture) {
            if (m_textures.find(element)!= m_textures.end())
            {
            } else
            {
                markAllDescriptorSetsNeedRecreate();
            }
            m_textures[element] = texture;
            m_textureViewHashes[element] = texture->textureViewHash();
            markDescriptorSetDirty(m_ctx->currentFrameIndex());
        }
    }

    void VK_PassResource::markAllDescriptorSetsDirty()
    {
        uint32_t maxFrames = m_ctx->maxFrameInFlight();
        m_dirtyFlags.resize(maxFrames, true);
    }

    void VK_PassResource::markDescriptorSetDirty(uint32_t frameIdx)
    {
        if (frameIdx < m_dirtyFlags.size()) {
            m_dirtyFlags[frameIdx] = true;
        }
    }
bool VK_PassResource::createDescriptorSetsAndLayouts(uint32_t frameIdx,
                                                    std::vector<vk::DescriptorSetLayout>& outLayouts,
                                                    std::vector<vk::DescriptorSet>& outDescriptorSets)
    {
        vk::Device device = m_ctx->getDevice();
        auto descriptorPool =
            static_cast<RHI::VK_DescriptorPool*>(m_ctx->getDescriptorPool());

        for (auto& layout : outLayouts) {
            if (layout) {
                device.destroyDescriptorSetLayout(layout);
                layout = nullptr;
            }
        }

        if (!outDescriptorSets.empty()) {
            try {
                device.freeDescriptorSets(descriptorPool->getPool(), outDescriptorSets);
                outDescriptorSets.clear();
            } catch (const vk::SystemError& e) {
            }
        }

        std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> setBindings;

        for (auto* constBuffer : m_constBuffers) {
            if (!constBuffer) continue;
            auto* vkConstBuffer = static_cast<RHI::VK_ConstBuffer*>(constBuffer);
            auto uniformLayout = vkConstBuffer->layout();
            auto [setIndex, binding] = m_ctx->getGenerator()->getLayoutBinding(uniformLayout);

            vk::DescriptorSetLayoutBinding layoutBinding;
            layoutBinding.setBinding(binding);
            layoutBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
            layoutBinding.setDescriptorCount(1);
            layoutBinding.setStageFlags(ConvertToVkShaderStageFlags(uniformLayout.getShaderStages()));

            setBindings[setIndex].push_back(layoutBinding);
        }

        for (auto& [element, texture] : m_textures) {
            auto [setIndex, binding] = m_ctx->getGenerator()->getTextureBinding(element);

            vk::DescriptorSetLayoutBinding layoutBinding;
            layoutBinding.setBinding(binding);
            layoutBinding.setDescriptorType(vk::DescriptorType::eSampledImage);
            layoutBinding.setDescriptorCount(1);
            layoutBinding.setStageFlags(ConvertToVkShaderStageFlags(element.getShaderStages()));

            setBindings[setIndex].push_back(layoutBinding);
        }

        for (auto& [sampler, element] : m_samplers) {
            auto [setIndex, binding] = m_ctx->getGenerator()->getSamplerBinding(element);

            vk::DescriptorSetLayoutBinding layoutBinding;
            layoutBinding.setBinding(binding);
            layoutBinding.setDescriptorType(vk::DescriptorType::eSampler);
            layoutBinding.setDescriptorCount(1);
            layoutBinding.setStageFlags(ConvertToVkShaderStageFlags(element.getShaderStages()));

            setBindings[setIndex].push_back(layoutBinding);
        }

        if (setBindings.empty()) {
            outLayouts.clear();
            outDescriptorSets.clear();
            return false;
        }

        uint32_t maxSetIndex = setBindings.rbegin()->first;

        vk::DescriptorSetLayoutCreateInfo emptyLayoutInfo(
            vk::DescriptorSetLayoutCreateFlags(),
            0,
            nullptr
        );
        vk::DescriptorSetLayout emptyLayout;
        try {
            emptyLayout = device.createDescriptorSetLayout(emptyLayoutInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error("Failed to create empty descriptor set layout: " + std::string(e.what()));
        }

        outLayouts.resize(maxSetIndex + 1, emptyLayout);

        for (const auto& [setIndex, bindings] : setBindings) {
            vk::DescriptorSetLayoutCreateInfo layoutInfo;
            layoutInfo.setBindingCount(static_cast<uint32_t>(bindings.size()));
            layoutInfo.setPBindings(bindings.data());

            try {
                if (outLayouts[setIndex] != emptyLayout) {
                    device.destroyDescriptorSetLayout(outLayouts[setIndex]);
                }
                outLayouts[setIndex] = device.createDescriptorSetLayout(layoutInfo);
            } catch (const vk::SystemError& e) {
                bool emptyStillUsed = false;
                for (auto& layout : outLayouts) {
                    if (layout == emptyLayout) {
                        emptyStillUsed = true;
                    } else if (layout) {
                        device.destroyDescriptorSetLayout(layout);
                    }
                    layout = nullptr;
                }
                if (emptyLayout) {
                    device.destroyDescriptorSetLayout(emptyLayout);
                }
                throw std::runtime_error("Failed to create descriptor set layout: " + std::string(e.what()));
            }
        }

        bool emptyLayoutUsed = false;
        for (const auto& layout : outLayouts) {
            if (layout == emptyLayout) {
                emptyLayoutUsed = true;
                break;
            }
        }
        if (!emptyLayoutUsed && emptyLayout) {
            device.destroyDescriptorSetLayout(emptyLayout);
        }

        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool(descriptorPool->getPool());
        allocInfo.setDescriptorSetCount(static_cast<uint32_t>(outLayouts.size()));
        allocInfo.setPSetLayouts(outLayouts.data());
        try {
            outDescriptorSets = device.allocateDescriptorSets(allocInfo);
        } catch (const vk::SystemError& e) {
            for (auto& layout : outLayouts) {
                if (layout) {
                    device.destroyDescriptorSetLayout(layout);
                    layout = nullptr;
                }
            }
            throw std::runtime_error("Failed to allocate descriptor sets for frame " +
                                    std::to_string(frameIdx) + ": " + std::string(e.what()));
        }

        return true;
    }
    void VK_PassResource::create()
    {
        if (m_constBuffers.empty() && m_samplers.empty() && m_textures.empty()) {
            return;
        }

        uint32_t maxFrames = m_ctx->maxFrameInFlight();

        m_descriptorSets.resize(maxFrames);
        m_dirtyFlags.resize(maxFrames, true);
        m_needRecreate.assign(maxFrames, false);

        for (uint32_t frameIdx = 0; frameIdx < maxFrames; ++frameIdx) {
            if (createDescriptorSetsAndLayouts(frameIdx, m_descriptorSetLayouts, m_descriptorSets[frameIdx])) {

                m_dirtyFlags[frameIdx] = true;
                updateDescriptorSetsIfNeeded(frameIdx);
            }
        }
    }

   void VK_PassResource::updateDescriptorSetsIfNeeded(uint32_t frameIdx) {


        if (frameIdx >= m_dirtyFlags.size() || !m_dirtyFlags[frameIdx]) {
            return;
        }

        vk::Device device = m_ctx->getDevice();

        std::vector<vk::WriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(m_constBuffers.size() + m_textures.size() + m_samplers.size());

        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(m_constBuffers.size());

        std::vector<vk::DescriptorImageInfo> textureInfos;
        textureInfos.reserve(m_textures.size());

        std::vector<vk::DescriptorImageInfo> samplerInfos;
        samplerInfos.reserve(m_samplers.size());

        for (size_t i = 0; i < m_constBuffers.size(); ++i) {
            auto* srcConstBuffer = m_constBuffers[i];
            if (!srcConstBuffer) {
                continue;
            }
            auto* constBuffer = static_cast<RHI::VK_ConstBuffer*>(srcConstBuffer);
            auto uniformLayout = constBuffer->layout();
            auto [setIndex, binding] = m_ctx->getGenerator()->getLayoutBinding(uniformLayout);

            if (setIndex >= m_descriptorSets[frameIdx].size()) {
                continue;
            }

            bufferInfos.push_back(constBuffer->currentBufferInfoWithoutUpdata());

            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setDstSet(m_descriptorSets[frameIdx][setIndex]);
            descriptorWrite.setDstBinding(binding);
            descriptorWrite.setDstArrayElement(0);
            descriptorWrite.setDescriptorType(vk::DescriptorType::eUniformBuffer);
            descriptorWrite.setDescriptorCount(1);
            descriptorWrite.setPBufferInfo(&bufferInfos[bufferInfos.size() - 1]);

            descriptorWrites.push_back(descriptorWrite);
        }

        for (auto& [element, texture ] : m_textures) {
            auto [setIndex, binding] = m_ctx->getGenerator()->getTextureBinding(element);

            if (setIndex >= m_descriptorSets[frameIdx].size()) {
                continue;
            }

            if (!texture) {
                continue;
            }
            auto* vkTexture = static_cast<RHI::VK_TextureView*>(texture->currentTextureView());
            if (!vkTexture || !vkTexture->view()) {
                continue;
            }

            vk::DescriptorImageInfo imageInfo;
            imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            imageInfo.setSampler(nullptr);
            imageInfo.setImageView(vkTexture->view());
            
            textureInfos.push_back(imageInfo);

            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setDstSet(m_descriptorSets[frameIdx][setIndex]);
            descriptorWrite.setDstBinding(binding);
            descriptorWrite.setDstArrayElement(0);
            descriptorWrite.setDescriptorType(vk::DescriptorType::eSampledImage);
            descriptorWrite.setDescriptorCount(1);
            descriptorWrite.setPImageInfo(&textureInfos.back());

            descriptorWrites.push_back(descriptorWrite);
        }

        for (size_t i = 0; i < m_samplers.size(); ++i) {
            auto& [sampler, element] = m_samplers[i];
            auto [setIndex, binding] = m_ctx->getGenerator()->getSamplerBinding(element);

            if (setIndex >= m_descriptorSets[frameIdx].size()) {
                continue;
            }

            if (!sampler) {
                continue;
            }
            auto* vkSampler = static_cast<RHI::VK_Sampler*>(sampler);
            if (!vkSampler || !vkSampler->getSampler()) {
                continue;
            }

            vk::DescriptorImageInfo samplerInfo;
            samplerInfo.setImageView(nullptr);
            samplerInfo.setImageLayout(vk::ImageLayout::eUndefined);
            samplerInfo.setSampler(vkSampler->getSampler());
            
            samplerInfos.push_back(samplerInfo);

            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setDstSet(m_descriptorSets[frameIdx][setIndex]);
            descriptorWrite.setDstBinding(binding);
            descriptorWrite.setDstArrayElement(0);
            descriptorWrite.setDescriptorType(vk::DescriptorType::eSampler);
            descriptorWrite.setDescriptorCount(1);
            descriptorWrite.setPImageInfo(&samplerInfos.back());

            descriptorWrites.push_back(descriptorWrite);
        }

        if (!descriptorWrites.empty()) {
            device.updateDescriptorSets(descriptorWrites, nullptr);
        }

        m_dirtyFlags[frameIdx] = false;
    }
    vk::DescriptorSet VK_PassResource::getDescriptorSet(uint32_t frameIdx, uint32_t setIndex)
    {
        recreateDescriptorSetsIfNeeded(frameIdx);
        updateDescriptorSetsIfNeeded(frameIdx);

        if (frameIdx < m_descriptorSets.size() && setIndex < m_descriptorSets[frameIdx].size()) {
            return m_descriptorSets[frameIdx][setIndex];
        }

        return nullptr;
    }
    void VK_PassResource::markAllDescriptorSetsNeedRecreate()
    {
        uint32_t maxFrames = m_ctx->maxFrameInFlight();
        m_needRecreate.resize(maxFrames, true);
        for (auto frameIdx = 0; frameIdx < maxFrames; ++frameIdx)
        {
            m_needRecreate[frameIdx] = true;
        }
    }
    bool VK_PassResource::recreateDescriptorSetsIfNeeded(uint32_t frameIdx)
    {
        if (frameIdx >= m_needRecreate.size() || !m_needRecreate[frameIdx])
        {
            return false;
        }
        if (createDescriptorSetsAndLayouts(frameIdx, m_descriptorSetLayouts, m_descriptorSets[frameIdx])) {
            m_needRecreate[frameIdx] = false;
            m_dirtyFlags[frameIdx] = true;
            return true;
        }
        return false;
    }

    void VK_PassResource::update()
    {
        uint32_t frameIdx = m_ctx->currentSubmitFrameIndex();
        recreateDescriptorSetsIfNeeded(frameIdx);
        updateDescriptorSetsIfNeeded(frameIdx);
    }

    void VK_PassResource::bind(RHI::CommandBuffer* cmdBuf, RHI::Pipeline* srcPipeline)
    {

        if (!srcPipeline || m_descriptorSets.empty()) {
            return;
        }

        uint32_t frameIdx = m_ctx->currentSubmitFrameIndex();

        checkTextureViewHash();
        recreateDescriptorSetsIfNeeded(frameIdx);
        updateDescriptorSetsIfNeeded(frameIdx);

        auto vkCmdBuf = static_cast<RHI::VK_CommandBuffer*>(cmdBuf);
        vk::CommandBuffer commandBuffer = vkCmdBuf->commandBuffer();

        auto pipeline = static_cast<RHI::VK_RasterizationPipeline*>(srcPipeline);
        vk::PipelineLayout pipelineLayout = pipeline->pipelineLayout();

        vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;

        if (!m_descriptorSets[frameIdx].empty()) {
            commandBuffer.bindDescriptorSets(
                bindPoint,
                pipelineLayout,
                0,
                static_cast<uint32_t>(m_descriptorSets[frameIdx].size()),
                m_descriptorSets[frameIdx].data(),
                0,
                nullptr
            );
        }
    }

    void VK_PassResource::checkTextureViewHash()
    {
        bool hasChange = false;
        for (auto& it : m_textureViewHashes)
        {
            if (m_textures[it.first] && m_textures[it.first]->textureViewHash() != it.second)
            {
                hasChange = true;
                it.second = m_textures[it.first]->textureViewHash();
            }
        }
        if (hasChange)
            markAllDescriptorSetsDirty();
    }
}

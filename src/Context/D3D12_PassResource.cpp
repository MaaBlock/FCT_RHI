#include "../FCTAPI.h"
#include "./D3D12_PassResource.h"
#include "./D3D12_Context.h"
#include "../RHI/D3D12_CommandBuffer.h"
#include "../RHI/D3D12_TraditionalPipeline.h"
#include "../RHI/D3D12_DescriptorPool.h"
#include "../RHI/D3D12_ConstBuffer.h"
#include "../RHI/D3D12_TextureView.h"
#include "../RHI/D3D12_Sampler.h"
#include "../RHI/D3D12_Image.h"

namespace FCT
{
    D3D12_PassResource::D3D12_PassResource(D3D12_Context* ctx)
        : m_ctx(ctx)
    {
    }

    void D3D12_PassResource::markAllDescriptorSetsNeedRecreate()
    {
        for (auto& [pipeline, frameVec] : m_pipelineTables)
        {
            for (auto& tables : frameVec)
            {
                for (int s = 0; s < 4; ++s)
                {
                    tables.cbvSrvTable[s].dirty = true;
                    tables.samplerTable[s].dirty = true;
                }
            }
        }
    }

    void D3D12_PassResource::addConstBuffer(RHI::ConstBuffer* buffer)
    {
        if (buffer)
        {
            for (auto*& existing : m_constBuffers)
            {
                if (existing && existing->layout() == buffer->layout())
                {
                    existing = buffer;
                    markAllDescriptorSetsNeedRecreate();
                    return;
                }
            }
            m_constBuffers.push_back(buffer);
            markAllDescriptorSetsNeedRecreate();
        }
    }

    void D3D12_PassResource::addSampler(Sampler* sampler, SamplerElement element)
    {
        if (sampler)
        {
            for (auto& [existingSampler, existingElem] : m_samplers)
            {
                if (existingElem == element)
                {
                    existingSampler = sampler;
                    markAllDescriptorSetsNeedRecreate();
                    return;
                }
            }
            m_samplers.emplace_back(sampler, element);
            markAllDescriptorSetsNeedRecreate();
        }
    }

    void D3D12_PassResource::addTexture(Image* texture, TextureElement element)
    {
        m_textures[element] = texture;
        markAllDescriptorSetsNeedRecreate();
    }

    void D3D12_PassResource::setTexture(Image* texture, TextureElement element)
    {
        m_textures[element] = texture;
        markAllDescriptorSetsNeedRecreate();
    }

    void D3D12_PassResource::create()
    {
        markAllDescriptorSetsNeedRecreate();
    }

    void D3D12_PassResource::update()
    {
    }

    void D3D12_PassResource::updateAndBindSetTables(
        ID3D12GraphicsCommandList* cmdList,
        RHI::D3D12_TraditionalPipeline* d3dPipeline,
        uint32_t frameIdx)
    {
        auto* pool = static_cast<RHI::D3D12_DescriptorPool*>(m_ctx->getDescriptorPool());
        if (!pool)
        {
            return;
        }

        uint32_t maxFrames = m_ctx->maxFrameInFlight();
        if (maxFrames == 0)
        {
            maxFrames = 1;
        }
        if (frameIdx >= maxFrames)
        {
            frameIdx = 0;
        }

        auto& frameVec = m_pipelineTables[d3dPipeline];
        if (frameVec.size() < maxFrames)
        {
            frameVec.resize(maxFrames);
        }
        D3D12_PipelineFrameTables& frameTables = frameVec[frameIdx];

        for (const auto& [texElem, img] : m_textures)
        {
            if (img && img->currentImage())
            {
                auto* d3dImg = static_cast<RHI::D3D12_Image*>(img->currentImage());
                d3dImg->transitionState(
                    cmdList,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            }
        }

        ID3D12Device* device = m_ctx->getDevice();

        for (uint32_t setIdx = 0; setIdx < 4; ++setIdx)
        {
            const auto& setLayout = d3dPipeline->getSetLayout(setIdx);

            if (!setLayout.cbvSrvSlots.empty() && setLayout.cbvSrvRootParamIndex != UINT32_MAX)
            {
                auto& slice = frameTables.cbvSrvTable[setIdx];
                uint32_t neededCount = static_cast<uint32_t>(setLayout.cbvSrvSlots.size());
                if (!slice.allocated || slice.count < neededCount)
                {
                    slice.allocated = pool->allocCbvSrvUavRange(
                        neededCount,
                        slice.cpuHandle,
                        slice.gpuHandle,
                        slice.baseIndex);
                    slice.count = neededCount;
                    slice.dirty = true;
                }

                if (slice.allocated)
                {
                    for (uint32_t i = 0; i < neededCount; ++i)
                    {
                        const auto& slot = setLayout.cbvSrvSlots[i];
                        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->getCbvSrvUavCpuHandle(slice.baseIndex + i);
                        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle{0};

                        if (slot.isCbv)
                        {
                            RHI::ConstBuffer* cb = nullptr;
                            for (auto* candidate : m_constBuffers)
                            {
                                if (candidate && candidate->layout() == slot.constLayout)
                                {
                                    cb = candidate;
                                    break;
                                }
                            }
                            if (!cb)
                            {
                                cb = m_ctx->getEmptyConstBuffer(slot.constLayout);
                            }
                            if (cb)
                            {
                                srcHandle = static_cast<RHI::D3D12_ConstBuffer*>(cb)->getCpuHandle(frameIdx);
                            }
                        }
                        else
                        {
                            auto it = m_textures.find(slot.textureElement);
                            if (it != m_textures.end() && it->second && it->second->currentTextureView())
                            {
                                auto* tv = static_cast<RHI::D3D12_TextureView*>(it->second->currentTextureView());
                                srcHandle = tv->getCpuHandle();
                            }
                        }

                        if (srcHandle.ptr != 0)
                        {
                            device->CopyDescriptorsSimple(
                                1,
                                dstHandle,
                                srcHandle,
                                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        }
                    }
                    slice.dirty = false;
                    cmdList->SetGraphicsRootDescriptorTable(setLayout.cbvSrvRootParamIndex, slice.gpuHandle);
                }
            }

            if (!setLayout.samplerSlots.empty() && setLayout.samplerRootParamIndex != UINT32_MAX)
            {
                auto& slice = frameTables.samplerTable[setIdx];
                uint32_t neededCount = static_cast<uint32_t>(setLayout.samplerSlots.size());
                if (!slice.allocated || slice.count < neededCount)
                {
                    slice.allocated = pool->allocSamplerRange(
                        neededCount,
                        slice.cpuHandle,
                        slice.gpuHandle,
                        slice.baseIndex);
                    slice.count = neededCount;
                    slice.dirty = true;
                }

                if (slice.allocated)
                {
                    if (slice.dirty)
                    {
                        for (uint32_t i = 0; i < neededCount; ++i)
                        {
                            const auto& slot = setLayout.samplerSlots[i];
                            D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->getSamplerCpuHandle(slice.baseIndex + i);
                            Sampler* samp = nullptr;
                            for (const auto& [candidateSampler, candidateElem] : m_samplers)
                            {
                                if (candidateSampler && candidateElem == slot.samplerElement)
                                {
                                    samp = candidateSampler;
                                    break;
                                }
                            }
                            if (!samp)
                            {
                                samp = m_ctx->getEmptySampler();
                            }
                            if (samp)
                            {
                                D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = static_cast<D3D12_Sampler*>(samp)->getCpuHandle();
                                if (srcHandle.ptr != 0)
                                {
                                    device->CopyDescriptorsSimple(
                                        1,
                                        dstHandle,
                                        srcHandle,
                                        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                                }
                            }
                        }
                        slice.dirty = false;
                    }
                    cmdList->SetGraphicsRootDescriptorTable(setLayout.samplerRootParamIndex, slice.gpuHandle);
                }
            }
        }
    }

    void D3D12_PassResource::bind(RHI::CommandBuffer* cmdBuf, RHI::Pipeline* pipeline)
    {
        if (!cmdBuf || !pipeline)
        {
            return;
        }
        auto* d3dCmd = static_cast<RHI::D3D12_CommandBuffer*>(cmdBuf);
        if (!d3dCmd->commandList())
        {
            return;
        }

        if (pipeline->getType() == PipelineType::Traditional)
        {
            auto* d3dPipeline = static_cast<RHI::D3D12_TraditionalPipeline*>(pipeline);
            uint32_t frameIdx = m_ctx->currentSubmitFrameIndex();
            updateAndBindSetTables(d3dCmd->commandList(), d3dPipeline, frameIdx);
        }
    }
}

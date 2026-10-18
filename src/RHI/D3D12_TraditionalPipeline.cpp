#include "../FCTAPI.h"
#include "./D3D12_TraditionalPipeline.h"
#include "./D3D12_CommandBuffer.h"
#include "../Context/D3D12_Context.h"
#include "../Context/VertexShader.h"
#include "../Context/PixelShader.h"
#include "../Context/Context.hpp"

namespace FCT
{
    namespace RHI
    {
        D3D12_TraditionalPipeline::D3D12_TraditionalPipeline(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_vertexShader(nullptr)
            , m_pixelShader(nullptr)
            , m_blendState(nullptr)
            , m_rasterizationState(nullptr)
            , m_depthStencilState(nullptr)
            , m_viewportState(nullptr)
            , m_inputLayout(nullptr)
            , m_pass(nullptr)
        {
        }

        D3D12_TraditionalPipeline::~D3D12_TraditionalPipeline()
        {
            m_pipelineState.Reset();
            m_rootSignature.Reset();
            FCT_SAFE_RELEASE(m_vertexShader);
            FCT_SAFE_RELEASE(m_pixelShader);
            FCT_SAFE_RELEASE(m_blendState);
            FCT_SAFE_RELEASE(m_rasterizationState);
            FCT_SAFE_RELEASE(m_depthStencilState);
            FCT_SAFE_RELEASE(m_viewportState);
            FCT_SAFE_RELEASE(m_inputLayout);
        }

        void D3D12_TraditionalPipeline::addResources(IPipelineResource* resource)
        {
            if (!resource)
            {
                return;
            }
            switch (resource->getType())
            {
            case PipelineResourceType::VertexShader:
                FCT_SAFE_RELEASE(m_vertexShader);
                m_vertexShader = static_cast<FCT::VertexShader*>(resource);
                m_vertexShader->addRef();
                break;
            case PipelineResourceType::PixelShader:
                FCT_SAFE_RELEASE(m_pixelShader);
                m_pixelShader = static_cast<FCT::PixelShader*>(resource);
                m_pixelShader->addRef();
                break;
            case PipelineResourceType::BlendState:
                FCT_SAFE_RELEASE(m_blendState);
                m_blendState = static_cast<D3D12_BlendState*>(resource);
                m_blendState->addRef();
                break;
            case PipelineResourceType::RasterizationState:
                FCT_SAFE_RELEASE(m_rasterizationState);
                m_rasterizationState = static_cast<D3D12_RasterizationState*>(resource);
                m_rasterizationState->addRef();
                break;
            case PipelineResourceType::DepthStencilState:
                FCT_SAFE_RELEASE(m_depthStencilState);
                m_depthStencilState = static_cast<D3D12_DepthStencilState*>(resource);
                m_depthStencilState->addRef();
                break;
            case PipelineResourceType::ViewportState:
                FCT_SAFE_RELEASE(m_viewportState);
                m_viewportState = static_cast<D3D12_ViewportState*>(resource);
                m_viewportState->addRef();
                break;
            case PipelineResourceType::InputLayout:
                FCT_SAFE_RELEASE(m_inputLayout);
                m_inputLayout = static_cast<D3D12_InputLayout*>(resource);
                m_inputLayout->addRef();
                break;
            default:
                break;
            }
        }

        void D3D12_TraditionalPipeline::bindPass(Pass* pass)
        {
            m_pass = static_cast<D3D12_Pass*>(pass);
        }

        void D3D12_TraditionalPipeline::generateDefaultResources()
        {
            if (!m_vertexShader)
            {
                m_vertexShader = m_ctx->createResource<FCT::VertexShader>();
                for (const auto& [slot, layout] : m_vertexLayouts)
                {
                    m_vertexShader->addLayout(slot, layout);
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
                m_inputLayout = static_cast<D3D12_InputLayout*>(m_vertexShader->createBindedInputLayout());
            }
            if (!m_blendState)
            {
                m_blendState = new D3D12_BlendState(m_ctx);
                if (m_pass && !m_pass->renderTargets().empty())
                {
                    m_blendState->targets(m_pass->renderTargets().rbegin()->first + 1);
                }
                m_blendState->create();
            }
            if (!m_rasterizationState)
            {
                m_rasterizationState = new D3D12_RasterizationState(m_ctx);
                m_rasterizationState->create();
            }
            if (!m_viewportState)
            {
                m_viewportState = new D3D12_ViewportState(m_ctx);
                m_viewportState->create();
            }
            if (!m_depthStencilState)
            {
                m_depthStencilState = new D3D12_DepthStencilState(m_ctx);
                m_depthStencilState->create();
            }
        }

        void D3D12_TraditionalPipeline::collectShaderResources(ShaderBinary binary)
        {
            const auto& cbLocations = binary.constBufferLocations();
            const auto& cbLayouts = binary.constBufferLayout();
            for (const auto& [name, loc] : cbLocations)
            {
                uint32_t setIdx = loc.first;
                uint32_t binding = loc.second;
                if (setIdx >= 4)
                {
                    continue;
                }
                auto layoutIt = cbLayouts.find(name);
                if (layoutIt == cbLayouts.end())
                {
                    continue;
                }
                auto& slots = m_setLayouts[setIdx].cbvSrvSlots;
                bool exists = false;
                for (const auto& s : slots)
                {
                    if (s.isCbv && s.registerIndex == binding)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    D3D12_CbvSrvSlot slot{};
                    slot.isCbv = true;
                    slot.registerIndex = binding;
                    slot.constLayout = layoutIt->second;
                    slots.push_back(slot);
                }
            }

            const auto& texLocations = binary.textureLocations();
            const auto& texLayouts = binary.textureLayout();
            for (const auto& [name, loc] : texLocations)
            {
                uint32_t setIdx = loc.first;
                uint32_t binding = loc.second;
                if (setIdx >= 4)
                {
                    continue;
                }
                auto layoutIt = texLayouts.find(name);
                if (layoutIt == texLayouts.end())
                {
                    continue;
                }
                auto& slots = m_setLayouts[setIdx].cbvSrvSlots;
                bool exists = false;
                for (const auto& s : slots)
                {
                    if (!s.isCbv && s.registerIndex == binding)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    D3D12_CbvSrvSlot slot{};
                    slot.isCbv = false;
                    slot.registerIndex = binding;
                    slot.textureElement = layoutIt->second;
                    slots.push_back(slot);
                }
            }

            const auto& sampLocations = binary.samplerLocations();
            const auto& sampLayouts = binary.samplerLayout();
            for (const auto& [name, loc] : sampLocations)
            {
                uint32_t setIdx = loc.first;
                uint32_t binding = loc.second;
                if (setIdx >= 4)
                {
                    continue;
                }
                auto layoutIt = sampLayouts.find(name);
                if (layoutIt == sampLayouts.end())
                {
                    continue;
                }
                auto& slots = m_setLayouts[setIdx].samplerSlots;
                bool exists = false;
                for (const auto& s : slots)
                {
                    if (s.registerIndex == binding)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    D3D12_SamplerSlot slot{};
                    slot.registerIndex = binding;
                    slot.samplerElement = layoutIt->second;
                    slots.push_back(slot);
                }
            }
        }

        void D3D12_TraditionalPipeline::createRootSignature()
        {
            for (int i = 0; i < 4; ++i)
            {
                m_setLayouts[i] = D3D12_SetRootLayout{};
            }

            if (m_vertexShader)
            {
                collectShaderResources(m_vertexShader->binaryCode());
            }
            if (m_pixelShader)
            {
                collectShaderResources(m_pixelShader->binaryCode());
            }

            std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> rangeStorage;
            rangeStorage.reserve(8);
            std::vector<D3D12_ROOT_PARAMETER> rootParams;
            rootParams.reserve(8);

            for (uint32_t setIdx = 0; setIdx < 4; ++setIdx)
            {
                auto& setLayout = m_setLayouts[setIdx];
                std::sort(setLayout.cbvSrvSlots.begin(), setLayout.cbvSrvSlots.end(),
                    [](const D3D12_CbvSrvSlot& a, const D3D12_CbvSrvSlot& b)
                    {
                        if (a.isCbv != b.isCbv)
                        {
                            return a.isCbv > b.isCbv;
                        }
                        return a.registerIndex < b.registerIndex;
                    });
                std::sort(setLayout.samplerSlots.begin(), setLayout.samplerSlots.end(),
                    [](const D3D12_SamplerSlot& a, const D3D12_SamplerSlot& b)
                    {
                        return a.registerIndex < b.registerIndex;
                    });

                if (!setLayout.cbvSrvSlots.empty())
                {
                    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
                    ranges.reserve(setLayout.cbvSrvSlots.size());
                    for (uint32_t i = 0; i < setLayout.cbvSrvSlots.size(); ++i)
                    {
                        const auto& slot = setLayout.cbvSrvSlots[i];
                        D3D12_DESCRIPTOR_RANGE range{};
                        range.RangeType = slot.isCbv ? D3D12_DESCRIPTOR_RANGE_TYPE_CBV : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        range.NumDescriptors = 1;
                        range.BaseShaderRegister = slot.registerIndex;
                        range.RegisterSpace = setIdx;
                        range.OffsetInDescriptorsFromTableStart = i;
                        ranges.push_back(range);
                    }
                    rangeStorage.push_back(std::move(ranges));

                    D3D12_ROOT_PARAMETER param{};
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(rangeStorage.back().size());
                    param.DescriptorTable.pDescriptorRanges = rangeStorage.back().data();
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                    setLayout.cbvSrvRootParamIndex = static_cast<uint32_t>(rootParams.size());
                    rootParams.push_back(param);
                }

                if (!setLayout.samplerSlots.empty())
                {
                    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
                    ranges.reserve(setLayout.samplerSlots.size());
                    for (uint32_t i = 0; i < setLayout.samplerSlots.size(); ++i)
                    {
                        const auto& slot = setLayout.samplerSlots[i];
                        D3D12_DESCRIPTOR_RANGE range{};
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                        range.NumDescriptors = 1;
                        range.BaseShaderRegister = slot.registerIndex;
                        range.RegisterSpace = setIdx;
                        range.OffsetInDescriptorsFromTableStart = i;
                        ranges.push_back(range);
                    }
                    rangeStorage.push_back(std::move(ranges));

                    D3D12_ROOT_PARAMETER param{};
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(rangeStorage.back().size());
                    param.DescriptorTable.pDescriptorRanges = rangeStorage.back().data();
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                    setLayout.samplerRootParamIndex = static_cast<uint32_t>(rootParams.size());
                    rootParams.push_back(param);
                }
            }

            D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
            rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
            rootSigDesc.pParameters = rootParams.empty() ? nullptr : rootParams.data();
            rootSigDesc.NumStaticSamplers = 0;
            rootSigDesc.pStaticSamplers = nullptr;
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
            HRESULT hr = D3D12SerializeRootSignature(
                &rootSigDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                &signatureBlob,
                &errorBlob);
            if (FAILED(hr))
            {
                if (errorBlob)
                {
                    ferr << "D3D12SerializeRootSignature failed: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << std::endl;
                }
                return;
            }

            hr = m_ctx->getDevice()->CreateRootSignature(
                0,
                signatureBlob->GetBufferPointer(),
                signatureBlob->GetBufferSize(),
                IID_PPV_ARGS(&m_rootSignature));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 RootSignature, hr = " << std::hex << hr << std::dec << std::endl;
            }
        }

        void D3D12_TraditionalPipeline::createPipelineState()
        {
            if (!m_vertexShader || !m_vertexShader->vertexShader() || !m_rootSignature)
            {
                return;
            }

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.VS = static_cast<D3D12_VertexShader*>(m_vertexShader->vertexShader())->getShaderBytecode();
            if (m_pixelShader && m_pixelShader->pixelShader())
            {
                psoDesc.PS = static_cast<D3D12_PixelShader*>(m_pixelShader->pixelShader())->getShaderBytecode();
            }
            psoDesc.BlendState = m_blendState->getBlendDesc();
            psoDesc.SampleMask = m_rasterizationState->sampleMask();
            psoDesc.RasterizerState = m_rasterizationState->getRasterizerDesc();
            psoDesc.DepthStencilState = m_depthStencilState->getDepthStencilDesc();
            psoDesc.InputLayout = m_inputLayout->getInputLayoutDesc();
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            Samples sampleCount = Samples::sample_1;
            uint32_t maxSlot = 0;
            bool hasAnyRtv = false;

            if (m_pass)
            {
                for (const auto& [slot, img] : m_pass->renderTargets())
                {
                    if (img && slot < 8)
                    {
                        hasAnyRtv = true;
                        psoDesc.RTVFormats[slot] = ToDxgiFormat(img->format());
                        if (slot + 1 > maxSlot)
                        {
                            maxSlot = slot + 1;
                        }
                        if (sampleCount == Samples::sample_1 && img->samples() > Samples::sample_1)
                        {
                            sampleCount = img->samples();
                        }
                    }
                }
                psoDesc.NumRenderTargets = hasAnyRtv ? maxSlot : 0;

                if (m_pass->depthStencil())
                {
                    psoDesc.DSVFormat = ToDxgiDsvFormat(m_pass->depthStencil()->format());
                    if (sampleCount == Samples::sample_1 && m_pass->depthStencil()->samples() > Samples::sample_1)
                    {
                        sampleCount = m_pass->depthStencil()->samples();
                    }
                }
                else
                {
                    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    psoDesc.DepthStencilState.DepthEnable = FALSE;
                    psoDesc.DepthStencilState.StencilEnable = FALSE;
                }
            }
            else
            {
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
            }

            psoDesc.SampleDesc.Count = ToDxgiSampleCount(sampleCount);
            psoDesc.SampleDesc.Quality = 0;

            HRESULT hr = m_ctx->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
            if (FAILED(hr))
            {
                ferr << "Failed to create D3D12 GraphicsPipelineState, hr = " << std::hex << hr << std::dec << std::endl;
                Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
                if (SUCCEEDED(m_ctx->getDevice()->QueryInterface(IID_PPV_ARGS(&infoQueue))))
                {
                    UINT64 msgCount = infoQueue->GetNumStoredMessages();
                    for (UINT64 i = 0; i < msgCount; ++i)
                    {
                        SIZE_T msgLen = 0;
                        infoQueue->GetMessage(i, nullptr, &msgLen);
                        if (msgLen > 0)
                        {
                            std::vector<char> msgBuf(msgLen);
                            auto* msg = reinterpret_cast<D3D12_MESSAGE*>(msgBuf.data());
                            if (SUCCEEDED(infoQueue->GetMessage(i, msg, &msgLen)))
                            {
                                ferr << "[D3D12 InfoQueue] " << msg->pDescription << std::endl;
                            }
                        }
                    }
                    infoQueue->ClearStoredMessages();
                }
            }
        }

        void D3D12_TraditionalPipeline::create()
        {
            m_pipelineState.Reset();
            m_rootSignature.Reset();

            generateDefaultResources();
            createRootSignature();
            createPipelineState();
        }

        void D3D12_TraditionalPipeline::bind(D3D12_CommandBuffer* cmdBuf)
        {
            if (!cmdBuf || !cmdBuf->commandList() || !m_pipelineState || !m_rootSignature)
            {
                return;
            }

            ID3D12GraphicsCommandList* cmdList = cmdBuf->commandList();
            cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
            cmdList->SetPipelineState(m_pipelineState.Get());
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdBuf->setBoundPipeline(this);
        }
    }
}

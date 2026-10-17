#include "../FCTAPI.h"
#include "./D3D12_PassGroup.h"
#include "./D3D12_Pass.h"
#include "./D3D12_CommandBuffer.h"
#include "./D3D12_Image.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_PassGroup::D3D12_PassGroup(D3D12_Context* ctx)
            : m_ctx(ctx)
            , m_currentPassIndex(0)
            , m_activeBoundPass(nullptr)
            , m_transitionsToPresent(false)
        {
        }

        D3D12_PassGroup::~D3D12_PassGroup()
        {
        }

        void D3D12_PassGroup::create()
        {
            m_transitionsToPresent = false;
            for (Pass* pass : m_passes)
            {
                if (pass)
                {
                    pass->create(this);
                }
            }
            for (const auto& desc : m_passDescs)
            {
                for (const auto& succ : desc.successors)
                {
                    if (succ.pass == Pass::present)
                    {
                        m_transitionsToPresent = true;
                    }
                }
            }
        }

        void D3D12_PassGroup::onPassBegin(D3D12_Pass* pass, D3D12_CommandBuffer* d3dCmd)
        {
            if (!pass || !d3dCmd)
            {
                return;
            }
            if (m_activeBoundPass != pass)
            {
                pass->bindTargets(d3dCmd);
                pass->executeClear(d3dCmd);
                m_activeBoundPass = pass;
            }
        }

        void D3D12_PassGroup::beginSubmit(CommandBuffer* cmdBuf)
        {
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(cmdBuf);
            if (!d3dCmd)
            {
                return;
            }
            d3dCmd->setCurrentPassGroup(this);
            m_currentPassIndex = 0;
            m_activeBoundPass = nullptr;

            if (!m_passes.empty() && m_passes[0])
            {
                auto* firstPass = static_cast<D3D12_Pass*>(m_passes[0]);
                onPassBegin(firstPass, d3dCmd);
            }
        }

        void D3D12_PassGroup::nextPass(CommandBuffer* cmdBuf)
        {
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(cmdBuf);
            if (!d3dCmd)
            {
                return;
            }
            if (m_currentPassIndex + 1 < m_passes.size())
            {
                ++m_currentPassIndex;
                auto* next = static_cast<D3D12_Pass*>(m_passes[m_currentPassIndex]);
                onPassBegin(next, d3dCmd);
            }
        }

        void D3D12_PassGroup::endSubmit(CommandBuffer* cmdBuf)
        {
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(cmdBuf);
            if (!d3dCmd || !d3dCmd->commandList())
            {
                return;
            }

            ID3D12GraphicsCommandList* cmdList = d3dCmd->commandList();
            for (Pass* pass : m_passes)
            {
                if (!pass)
                {
                    continue;
                }
                for (const auto& [slot, img] : pass->renderTargets())
                {
                    if (!img)
                    {
                        continue;
                    }
                    if (m_transitionsToPresent || img->getType() == RenderTargetType::WindowTarget)
                    {
                        auto* d3dImg = static_cast<D3D12_Image*>(img->currentImage());
                        if (d3dImg)
                        {
                            d3dImg->transitionState(cmdList, D3D12_RESOURCE_STATE_PRESENT);
                        }
                    }
                }
            }

            m_activeBoundPass = nullptr;
            d3dCmd->setCurrentPassGroup(nullptr);
        }
    }
}

#include "../FCTAPI.h"
#include "./D3D12_Pass.h"
#include "./D3D12_PassGroup.h"
#include "./D3D12_CommandBuffer.h"
#include "./D3D12_Image.h"
#include "./D3D12_RenderTargetView.h"
#include "./D3D12_DepthStencilView.h"
#include "../Context/D3D12_Context.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_Pass::D3D12_Pass(D3D12_Context* ctx)
            : m_ctx(ctx)
        {
            m_group = nullptr;
        }

        void D3D12_Pass::create(PassGroup* group)
        {
            m_group = group;
        }

        void D3D12_Pass::bindTargets(D3D12_CommandBuffer* d3dCmd)
        {
            if (!d3dCmd || !d3dCmd->commandList())
            {
                return;
            }
            ID3D12GraphicsCommandList* cmdList = d3dCmd->commandList();

            uint32_t maxSlot = 0;
            bool hasAnyRtv = false;
            for (const auto& [slot, img] : m_renderTargets)
            {
                if (img)
                {
                    hasAnyRtv = true;
                    if (slot + 1 > maxSlot)
                    {
                        maxSlot = slot + 1;
                    }
                }
            }
            if (maxSlot > 8)
            {
                maxSlot = 8;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8]{};
            for (uint32_t i = 0; i < maxSlot; ++i)
            {
                auto it = m_renderTargets.find(i);
                if (it != m_renderTargets.end() && it->second)
                {
                    FCT::Image* fctImg = it->second;
                    auto* d3dImg = static_cast<D3D12_Image*>(fctImg->currentImage());
                    auto* d3dRtv = static_cast<D3D12_RenderTargetView*>(fctImg->currentTargetView());
                    if (d3dImg && d3dRtv && d3dRtv->isValid())
                    {
                        d3dImg->transitionState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
                        rtvHandles[i] = d3dRtv->getCpuHandle();
                    }
                }
            }

            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{0};
            bool hasDsv = false;
            if (m_depthStencil)
            {
                auto* d3dDepthImg = static_cast<D3D12_Image*>(m_depthStencil->currentImage());
                auto* d3dDsv = static_cast<D3D12_DepthStencilView*>(m_depthStencil->currentDepthStencilView());
                if (d3dDepthImg && d3dDsv && d3dDsv->isValid())
                {
                    d3dDepthImg->transitionState(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                    dsvHandle = d3dDsv->getCpuHandle();
                    hasDsv = true;
                }
            }

            cmdList->OMSetRenderTargets(
                hasAnyRtv ? maxSlot : 0,
                hasAnyRtv ? rtvHandles : nullptr,
                FALSE,
                hasDsv ? &dsvHandle : nullptr);
        }

        void D3D12_Pass::executeClear(CommandBuffer* cmdBuf)
        {
            if (!m_clearValue.types)
            {
                return;
            }
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(cmdBuf);
            if (!d3dCmd || !d3dCmd->commandList())
            {
                return;
            }
            ID3D12GraphicsCommandList* cmdList = d3dCmd->commandList();

            if (m_clearValue.types & ClearType::color)
            {
                const float clearColor[4] = {
                    m_clearValue.color.x,
                    m_clearValue.color.y,
                    m_clearValue.color.z,
                    m_clearValue.color.w
                };
                for (const auto& [slot, img] : m_renderTargets)
                {
                    if (img && img->currentTargetView())
                    {
                        auto* d3dRtv = static_cast<D3D12_RenderTargetView*>(img->currentTargetView());
                        if (d3dRtv->isValid())
                        {
                            cmdList->ClearRenderTargetView(d3dRtv->getCpuHandle(), clearColor, 0, nullptr);
                        }
                    }
                }
            }

            if (m_depthStencil && ((m_clearValue.types & ClearType::depth) || (m_clearValue.types & ClearType::stencil)))
            {
                auto* d3dDsv = static_cast<D3D12_DepthStencilView*>(m_depthStencil->currentDepthStencilView());
                if (d3dDsv && d3dDsv->isValid())
                {
                    D3D12_CLEAR_FLAGS clearFlags = static_cast<D3D12_CLEAR_FLAGS>(0);
                    if (m_clearValue.types & ClearType::depth)
                    {
                        clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                    }
                    if (m_clearValue.types & ClearType::stencil)
                    {
                        clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                    }
                    cmdList->ClearDepthStencilView(
                        d3dDsv->getCpuHandle(),
                        clearFlags,
                        m_clearValue.depth,
                        m_clearValue.stencil,
                        0,
                        nullptr);
                }
            }
        }

        void D3D12_Pass::beginSubmit(CommandBuffer* cmdBuf)
        {
            auto* d3dCmd = static_cast<D3D12_CommandBuffer*>(cmdBuf);
            if (m_group)
            {
                static_cast<D3D12_PassGroup*>(m_group)->onPassBegin(this, d3dCmd);
            }
            else
            {
                bindTargets(d3dCmd);
                executeClear(cmdBuf);
            }
        }

        void D3D12_Pass::endSubmit()
        {
        }
    }
}

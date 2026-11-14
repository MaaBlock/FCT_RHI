//
// Created by Administrator on 2025/8/17.
//

#ifndef LAYOUT_HPP
#define LAYOUT_HPP
#include "layout.h"
#include "../RHI/RasterizationPipeline.h"
#include "PassResource.h"

namespace FCT
{
    template <typename ... Args>
    void Layout::proccessArgs(FCT::VertexLayout vertexLayout, Args... args)
    {
        uint32_t index = findNextAvailableIndex();
        m_vertexLayouts[index] = vertexLayout;
        m_hasVertexLayout = true;
        processUnhandledTextureSlots();
        proccessArgs(args...);
    }
    template <typename ... Args>
   void Layout::proccessArgs(const std::vector<FCT::VertexLayout>& vertexLayouts, Args... args)
    {
        for(const auto& vertexLayout : vertexLayouts)
        {
            uint32_t index = findNextAvailableIndex();
            m_vertexLayouts[index] = vertexLayout;
        }
        m_hasVertexLayout = !vertexLayouts.empty();
        processUnhandledTextureSlots();
        proccessArgs(args...);
    }

    template <typename ... Args>
    void Layout::proccessArgs(SamplerSlot samplerSlot, Args... args)
    {
        m_resourceLayout.addSampler(samplerSlot);

        if (samplerSlot.getShaderStages() & FCT::ShaderStage::Vertex)
        {
            m_vertexResourceLayout.addSampler(samplerSlot);
        }
        if (samplerSlot.getShaderStages() & FCT::ShaderStage::Fragment)
        {
            m_pixelResourceLayout.addSampler(samplerSlot);
        }
        m_resourceLayout.addSampler(samplerSlot);

        proccessArgs(args...);
    }

    template <typename ... Args>
    void Layout::proccessArgs(PassName passName, Args... args)
    {

        m_passName = passName.name;
        /*
            if (m_ctx && m_ctx->getModule<FCT::RenderGraph>()) {
                attachPass(m_ctx->getModule<FCT::RenderGraph>(), passName.name);
            }*/
        proccessArgs(args...);
    }

    template <typename ... Args>
    void Layout::proccessArgs(uint32_t index, FCT::VertexLayout vertexLayout, Args... args)
    {
        m_vertexLayouts[index] = vertexLayout;
        m_hasVertexLayout = true;
        processUnhandledTextureSlots();
        proccessArgs(args...);
    }

    template <typename ... Args>
    void Layout::proccessArgs(FCT::PixelLayout pixelLayout, Args... args)
    {
        m_pixelLayout = pixelLayout;
        m_hasPixelLayout = true;
        processUnhandledTextureSlots();
        proccessArgs(args...);
    }

    template <typename ... Args>
    void Layout::proccessArgs(TextureSlot textureSlot, Args... args)
    {
        m_unhandledTextureSlots.push_back(textureSlot);
        processUnhandledTextureSlots();
        proccessArgs(args...);
    }

    template <typename... Args>
    void Layout::proccessArgs(UniformSlot uniformSlot, Args... args)
    {
        m_uniformLayouts[uniformSlot.getName()] = uniformSlot;
        proccessArgs(args...);
    }
    template <typename... Args>
    void Layout::proccessArgs(DisableInjectPassResource disable, Args... args)
    {
        m_injectPassResource = false;
        proccessArgs(args...);
    }

    template <typename ... Args>
    Layout::Layout(FCT::Context* ctx, Args... args): m_ctx(ctx)
    {
        proccessArgs(args...);
        if (m_passName.size())
        {
            attachPass(m_passName);
        }
    }

    template <typename T>
    void Layout::drawMesh(RHI::CommandBuffer* cmdBuffer, T* mesh)
    {
        auto resource = getCurrentPassResource();
        auto pipeline = getCurrentPipeline();
        if (!pipeline || pipeline->getNativeHandle() == 0)
        {
            return;
        }

        pipeline->bind(cmdBuffer);
        resource->bind(cmdBuffer,pipeline);
        mesh->bind(cmdBuffer);
        mesh->draw(cmdBuffer);
    }

    template <typename T>
    void Layout::drawMeshIndirect(RHI::CommandBuffer* cmdBuffer, T* mesh, void* indirectBuffer, size_t byteOffset, uint32_t drawCount)
    {
        auto resource = getCurrentPassResource();
        auto pipeline = getCurrentPipeline();
        if (!pipeline || pipeline->getNativeHandle() == 0)
        {
            return;
        }

        pipeline->bind(cmdBuffer);
        resource->bind(cmdBuffer, pipeline);
        mesh->bind(cmdBuffer);
        mesh->drawIndirect(cmdBuffer, indirectBuffer, byteOffset, drawCount);
    }
    inline void _fct_layout_indirect_anchor() {
    }

    template <typename T>
    void Layout::drawMesh(RHI::CommandBuffer* cmdBuffer, T& mesh)
    {
        auto resource = getCurrentPassResource();
        auto pipeline = getCurrentPipeline();
        if (!pipeline || pipeline->getNativeHandle() == 0)
        {
            return;
        }

        pipeline->bind(cmdBuffer);
        resource->bind(cmdBuffer,pipeline);
        mesh.bind(cmdBuffer);
        mesh.draw(cmdBuffer);
    }
}
#endif //LAYOUTS_H

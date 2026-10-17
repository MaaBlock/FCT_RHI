#include "./CommandBufferGraph.h"
#include "./Context.h"
#include "./Context.hpp"
#include "../UI/Window.h"
namespace FCT {
    void CommandBufferNodes::InputFromWindow::fillSynchronization(uint32_t frameIndex)
    {
        if (frameIndex >= m_frameDirty.size() || !m_frameDirty[frameIndex]) {
            return;
        }

        if (m_outputEdge && frameIndex < m_collectedImageAvailable.size()) {
            auto* semaphore = m_outputEdge->getImageAvailableSemaphore(frameIndex);
            m_collectedImageAvailable[frameIndex] = semaphore;

            markFrameClean(frameIndex);
        }
    }

    std::vector<RHI::Fence*> CommandBufferNodes::OutputToWindow::getCollectedFence(uint32_t frameIndex) const
    {
        if (frameIndex < m_collectedFences.size()) {
            return m_collectedFences[frameIndex];
        }
        return std::vector<RHI::Fence*>{} ;
    }

    std::vector<RHI::Semaphore*> CommandBufferNodes::OutputToWindow::getCollectedRenderFinishedSemaphore(
        uint32_t frameIndex) const
    {
        if (frameIndex < m_collectedRenderFinished.size()) {
            return m_collectedRenderFinished[frameIndex];
        }
        return std::vector<RHI::Semaphore*>{} ;
    }


    void CommandBufferNodes::OutputToWindow::fillSynchronization(uint32_t frameIndex)
    {
        if (frameIndex >= m_frameDirty.size() || !m_frameDirty[frameIndex]) {
            return;
        }

        if (frameIndex >= m_collectedFences.size() || frameIndex >= m_collectedRenderFinished.size()) {
            return;
        }

        m_collectedFences[frameIndex].clear();
        m_collectedRenderFinished[frameIndex].clear();

        for (auto* edge : m_inputEdges) {
            if (edge) {
                auto* fence = edge->getFence(frameIndex);
                auto* semaphore = edge->getRenderFinishedSemaphore(frameIndex);

                if (fence) {
                    m_collectedFences[frameIndex].push_back(fence);
                }
                if (semaphore) {
                    m_collectedRenderFinished[frameIndex].push_back(semaphore);
                }
            }
        }

        markFrameClean(frameIndex);
    }

    CommandBufferNodes::CommandBuffer::CommandBuffer(RHI::CommandPool* pool)
    {
        m_pool = pool;
    }

    CommandBufferNodes::CommandBuffer::~CommandBuffer()
    {
        for (auto& cmdBuffer : m_cmdBuffers) {
            cmdBuffer->release();
        }
        m_cmdBuffers.clear();
    }

    void CommandBufferNodes::CommandBuffer::updateSynchronization(uint32_t newMaxFrameInFlight)
    {
        if (m_cmdBuffers.size() != newMaxFrameInFlight) {
            for (auto& cmdBuffer : m_cmdBuffers)
            {
                cmdBuffer->release();
            }
            m_cmdBuffers.clear();
            m_cmdBuffers.reserve(newMaxFrameInFlight);
            for (uint32_t i = 0; i < newMaxFrameInFlight; ++i) {
                auto cmdBuf = m_pool->createCommandBuffer();
                cmdBuf->create();
                m_cmdBuffers.push_back(cmdBuf);
            }
        }
        m_frameDirty.clear();
        m_frameDirty.resize(newMaxFrameInFlight, 1);
    }

    void CommandBufferNodes::CommandBuffer::addOutputEdge(CommandBufferEdges::EdgeBase* edge)
    {
        std::fill(m_frameDirty.begin(), m_frameDirty.end(), true);
        m_outputEdges.push_back(edge);
    }

    void CommandBufferNodes::CommandBuffer::addInputEdge(CommandBufferEdges::EdgeBase* edge)
    {
        std::fill(m_frameDirty.begin(), m_frameDirty.end(), 1);
        m_inputEdges.push_back(edge);
    }

    RHI::CommandBuffer* CommandBufferNodes::CommandBuffer::getCommandBuffer(uint32_t frameIndex) const
    {
        if (frameIndex < m_cmdBuffers.size()) {
            return m_cmdBuffers[frameIndex];
        }
        return nullptr;
    }


    void CommandBufferNodes::CommandBuffer::fillSynchronization(uint32_t frameIndex)
    {
        auto* cmdBuf = getCommandBuffer(frameIndex);
        if (!cmdBuf || frameIndex >= m_frameDirty.size() || !m_frameDirty[frameIndex]) {
            return;
        }

        cmdBuf->clearWaitSemaphores();
        cmdBuf->signalSemaphores().clear();

        for (auto* edge : m_inputEdges) {
            if (auto* inputEdge = dynamic_cast<CommandBufferEdges::InputFromWindow*>(edge)) {
                if (auto* semaphore = inputEdge->getImageAvailableSemaphore(frameIndex)) {
                    cmdBuf->addWaitSemaphore(semaphore, PipelineStage::colorAttachmentOutput);
                }
            }
            // TODO: 处理其他类型的输入边
        }

        for (auto* edge : m_outputEdges) {
            if (auto* outputEdge = dynamic_cast<CommandBufferEdges::OutputToWindow*>(edge)) {
                auto* semaphore = outputEdge->getRenderFinishedSemaphore(frameIndex);
                if (semaphore) {
                    cmdBuf->addSignalSemaphore(semaphore);
                }

                auto* fence = outputEdge->getFence(frameIndex);
                if (fence) {
                    cmdBuf->fence(fence);
                }
            }
            // TODO: 处理其他类型的输出边
        }

        m_frameDirty[frameIndex] = false;
    }

    void CommandBufferEdges::InputFromWindow::updateSynchronization(uint32_t newMaxFrameInFlight)
    {

        if (m_imageAvailable.size() != newMaxFrameInFlight) {
            for (auto& semaphore : m_imageAvailable)
            {
                semaphore->release();
            }
            m_imageAvailable.clear();
            m_imageAvailable.reserve(newMaxFrameInFlight);
            for (uint32_t i = 0; i < newMaxFrameInFlight; ++i) {
                auto semaphore = m_semaphorePool->alloc();
                semaphore->create();
                m_imageAvailable.push_back(semaphore);
            }
        }
    }
    void CommandBufferEdges::OutputToWindow::updateSynchronization(uint32_t newMaxFrameInFlight)
    {
        if (m_renderFinished.size() != newMaxFrameInFlight) {
            for (auto& semaphore : m_renderFinished)
            {
                semaphore->release();
            }
            m_renderFinished.clear();
            m_renderFinished.reserve(newMaxFrameInFlight);
            for (uint32_t i = 0; i < newMaxFrameInFlight; ++i) {
                auto semaphore = m_semaphorePool->alloc();
                semaphore->create();
                m_renderFinished.push_back(semaphore);
            }
        }

        if (m_fences.size() != newMaxFrameInFlight) {
            for (auto& fence : m_fences)
            {
                fence->release();
            }
            m_fences.clear();
            m_fences.reserve(newMaxFrameInFlight);
            for (uint32_t i = 0; i < newMaxFrameInFlight; ++i) {
                auto fence = m_fencePool->alloc();
                fence->createSignaled();
                m_fences.push_back(fence);
            }
        }
    }

    CommandBufferGraph::CommandBufferGraph(Device* device)
    {
        m_cmdPool = device->createResource<RHI::CommandPool>();
        m_cmdPool->create();
        m_semaphorePool = device->createResource<SemaphorePool>();
        m_fencePool = device->createResource<FencePool>();
        m_device = device;
    }

    CommandBufferNodes::InputFromWindow* CommandBufferGraph::getOrCreateWindowInputNode(Window* window)
    {
        m_windows.insert(window);
        auto it = m_windowInputNodes.find(window);
        if (it != m_windowInputNodes.end()) {
            return it->second;
        }

        auto* nodePtr = FCT_NEW(CommandBufferNodes::InputFromWindow,window);
        nodePtr->updateSynchronization(m_maxFrameInFlight);
        m_nodes.insert(nodePtr);
        m_windowInputNodes[window] = nodePtr;
        return nodePtr;
    }

    CommandBufferNodes::OutputToWindow* CommandBufferGraph::getOrCreateWindowOutputNode(Window* window)
    {
        m_windows.insert(window);
        auto it = m_windowOutputNodes.find(window);
        if (it != m_windowOutputNodes.end()) {
            return it->second;
        }

        auto* nodePtr = FCT_NEW(CommandBufferNodes::OutputToWindow,window);
        nodePtr->updateSynchronization(m_maxFrameInFlight);
        m_nodes.insert(nodePtr);
        m_windowOutputNodes[window] = nodePtr;
        return nodePtr;
    }

    void CommandBufferGraph::connectPredecessors(CommandBufferNodes::CommandBuffer* buffer,
        const std::vector<NodeRef>& predecessors)
    {
        for (const auto& pred : predecessors) {
            if (pred.type == NodeRef::WindowType) {
                connectWindowPredecessor(buffer, pred.window);
            }
            else if (pred.type == NodeRef::CommandBufferType) {
                // CommandBuffer -> CommandBuffer
                // todo:
                // 创建CommandBuffer之间的边，并对CommandBuffers执行fillAllSynchronization
            }
        }
    }

    void CommandBufferGraph::connectSuccessors(CommandBufferNodes::CommandBuffer* buffer,
        const std::vector<NodeRef>& successors)
    {
        for (const auto& succ : successors) {
            if (succ.type == NodeRef::WindowType) {
                connectWindowSuccessor(buffer, succ.window);
            }
            else if (succ.type == NodeRef::CommandBufferType) {
                // CommandBuffer -> CommandBuffer
                // todo:
                // 创建CommandBuffer之间的边，并对CommandBuffers执行fillAllSynchronization
            }
        }
    }

    void CommandBufferGraph::connectWindowPredecessor(CommandBufferNodes::CommandBuffer* buffer, Window* window)
    {
        auto* windowInputNode = getOrCreateWindowInputNode(window);
        auto edge = std::make_unique<CommandBufferEdges::InputFromWindow>(m_semaphorePool);
        auto* edgePtr = edge.get();

        edgePtr->setSourceNode(windowInputNode);
        edgePtr->setTargetNode(buffer);
        edgePtr->updateSynchronization(m_maxFrameInFlight);

        windowInputNode->addOutputEdge(edgePtr);
        buffer->addInputEdge(edgePtr);

        m_edges.insert(std::move(edge));

        windowInputNode->fillAllSynchronization();
    }

    void CommandBufferGraph::connectWindowSuccessor(CommandBufferNodes::CommandBuffer* buffer, Window* window)
    {
        auto* windowOutputNode = getOrCreateWindowOutputNode(window);
        auto edge = std::make_unique<CommandBufferEdges::OutputToWindow>(m_fencePool,m_semaphorePool);
        auto* edgePtr = edge.get();

        edgePtr->setSourceNode(buffer);
        edgePtr->setTargetNode(windowOutputNode);
        edgePtr->updateSynchronization(m_maxFrameInFlight);

        buffer->addOutputEdge(edgePtr);
        windowOutputNode->addInputEdge(edgePtr);

        m_edges.insert(std::move(edge));

        windowOutputNode->fillAllSynchronization();
    }
    void CommandBufferGraph::maxFrameInFlight(uint32_t max)
    {
        m_maxFrameInFlight = max;
        for (auto& edge : m_edges) {
            edge->updateSynchronization(m_maxFrameInFlight);
        }
        for (auto& node : m_nodes) {
            node->updateSynchronization(m_maxFrameInFlight);
        }
        for (auto& node : m_nodes) {
            node->fillAllSynchronization();
        }
    }

    void CommandBufferGraph::swapBuffer()
    {
        ZoneScoped();
        for (auto& wnd : m_windows)
        {
            if (m_windowOutputNodes.count(wnd))
            {
                wnd->clearRenderFinshSemaphores();
                for (auto semaphore : m_windowOutputNodes[wnd]->getCollectedRenderFinishedSemaphores(m_frameIndex))
                {
                    wnd->addRenderFinshSemaphore(semaphore);
                }
            }
        }
        m_frameIndex = (m_frameIndex + 1) % m_maxFrameInFlight;
        for (auto& wnd : m_windows) {
            if (m_windowInputNodes.count(wnd))
            {
                wnd->setPresentFinshSemaphore(m_windowInputNodes[wnd]->getCollectedImageAvailableSemaphore(m_frameIndex));
            }
            if (m_windowOutputNodes.count(wnd))
            {
                wnd->clearRenderFinshFences();
                for (auto fence : m_windowOutputNodes[wnd]->getCollectedFences(m_frameIndex))
                {
                    wnd->addRenderFinshFence(fence);
                }
            }
        }
        for (auto wnd : m_windows)
        {
            wnd->swapBuffers();
        }
    }

    RHI::CommandBuffer* CommandBufferGraph::getCommandBuffer(CommandBufferToken token) const
    {
        return token->getCommandBuffer(m_frameIndex);
    }

    CommandBufferToken CommandBufferGraph::addBuffer(const std::vector<NodeRef>& predecessors,
                                                     const std::vector<NodeRef>& successors)
    {
        auto* bufferPtr = FCT_NEW(CommandBufferNodes::CommandBuffer,m_cmdPool);
        bufferPtr->updateSynchronization(m_maxFrameInFlight);
        m_nodes.insert(bufferPtr);

        connectPredecessors(bufferPtr, predecessors);
        connectSuccessors(bufferPtr, successors);

        bufferPtr->fillAllSynchronization();
        return bufferPtr;
    }
    void CommandBufferGraph::removeEdge(CommandBufferEdges::EdgeBase* edge)
    {
        if (auto* inputEdge = dynamic_cast<CommandBufferEdges::InputFromWindow*>(edge))
        {
            inputEdge->getSourceNode()->removeOutputEdge(edge);
            inputEdge->getSourceNode()->fillAllSynchronization();
            inputEdge->getTargetNode()->removeInputEdge(edge);
            inputEdge->getTargetNode()->fillAllSynchronization();
            auto it = std::find_if(m_edges.begin(), m_edges.end(),
                [edge](const std::unique_ptr<CommandBufferEdges::EdgeBase>& ptr) {
                    return ptr.get() == edge;
                });
            if (it != m_edges.end()) {
                m_edges.erase(it);
            }
        } else if (auto* outputEdge = dynamic_cast<CommandBufferEdges::OutputToWindow*>(edge))
        {
            outputEdge->getSourceNode()->removeOutputEdge(edge);
            outputEdge->getSourceNode()->fillAllSynchronization();
            outputEdge->getTargetNode()->removeInputEdge(edge);
            outputEdge->getTargetNode()->fillAllSynchronization();
            auto it = std::find_if(m_edges.begin(), m_edges.end(),
                [edge](const std::unique_ptr<CommandBufferEdges::EdgeBase>& ptr) {
                    return ptr.get() == edge;
                });
            if (it!= m_edges.end()) {
                m_edges.erase(it);
            }
        } else
        {

        }
    }
    void CommandBufferGraph::removeBuffer(CommandBufferToken token)
    {
        auto outputEdge = token->getOutputEdges();
        for (auto& edge : outputEdge) {
            removeEdge(edge);
        }
        auto inputEdge = token->getInputEdges();
        for (auto& edge : inputEdge) {
            removeEdge(edge);
        }
        m_nodes.erase(token);
        FCT_DELETE(token);
    }
} // FCT
#ifndef COMMANDBUFFERGRAPH_H
#define COMMANDBUFFERGRAPH_H
#include "../RHI/CommandBuffer.h"
#include "../RHI/CommandPool.h"

namespace FCT
{
    class Device;
    class SemaphorePool;
    class FencePool;
    class Window;
    class Context;
    namespace RHI
    {
        class Semaphore;
        class Fence;
        class CommandPool;
        class CommandBuffer;
    }
    namespace CommandBufferEdges
    {
        class EdgeBase;
    }
    namespace CommandBufferNodes
    {
        class NodeBase;
        class InputFromWindow;
        class OutputToWindow;
        class CommandBuffer;
    }
    namespace CommandBufferEdges
    {
        class EdgeBase
        {
        public:
            virtual ~EdgeBase() = default;
            EdgeBase(SemaphorePool* semaphorePool) : m_semaphorePool(semaphorePool) {}
            virtual void updateSynchronization(uint32_t newMaxFrameInFlight) = 0;
        protected:
            SemaphorePool* m_semaphorePool;
        };
        class InputFromWindow : public EdgeBase
        {
        public:
            InputFromWindow(SemaphorePool* semaphorePool) : EdgeBase(semaphorePool) {}
            ~InputFromWindow() override;

            void setSourceNode(CommandBufferNodes::InputFromWindow* source) {
                m_sourceNode = source;
            }
            void updateSynchronization(uint32_t newMaxFrameInFlight) override;

            void setTargetNode(CommandBufferNodes::CommandBuffer* target) {
                m_targetNode = target;
            }

            CommandBufferNodes::InputFromWindow* getSourceNode() const {
                return m_sourceNode;
            }
            CommandBufferNodes::CommandBuffer* getTargetNode() const {
                return m_targetNode;
            }

            RHI::Semaphore* getImageAvailableSemaphore(uint32_t frameIndex) const {
                if (frameIndex < m_imageAvailable.size()) {
                    return m_imageAvailable[frameIndex];
                }
                return nullptr;
            }
        private:
            std::vector<RHI::Semaphore*> m_imageAvailable;
            CommandBufferNodes::InputFromWindow* m_sourceNode = nullptr;
            CommandBufferNodes::CommandBuffer* m_targetNode = nullptr;
        };
        class OutputToWindow : public EdgeBase
        {
        public:
            OutputToWindow(FencePool* fencePool,SemaphorePool* semaphorePool) : EdgeBase(semaphorePool),
                m_fencePool(fencePool) {}
            ~OutputToWindow() override;
            void setSourceNode(CommandBufferNodes::CommandBuffer* source) {
                m_sourceNode = source;
            }

            void setTargetNode(CommandBufferNodes::OutputToWindow* target) {
                m_targetNode = target;
            }
            CommandBufferNodes::CommandBuffer* getSourceNode() const {
                return m_sourceNode;
            }

            CommandBufferNodes::OutputToWindow* getTargetNode() const {
                return m_targetNode;
            }

            RHI::Semaphore* getRenderFinishedSemaphore(uint32_t frameIndex) const {
                if (frameIndex < m_renderFinished.size()) {
                    return m_renderFinished[frameIndex];
                }
                return nullptr;
            }

            RHI::Fence* getFence(uint32_t frameIndex) const {
                if (frameIndex < m_fences.size()) {
                    return m_fences[frameIndex];
                }
                return nullptr;
            }
            void updateSynchronization(uint32_t newMaxFrameInFlight) override;
        private:
            FencePool* m_fencePool;
            std::vector<RHI::Semaphore*> m_renderFinished;
            std::vector<RHI::Fence*> m_fences;
            CommandBufferNodes::CommandBuffer* m_sourceNode = nullptr;
            CommandBufferNodes::OutputToWindow* m_targetNode = nullptr;
        };
    }
    namespace CommandBufferNodes
    {
        class NodeBase {
        public:
            virtual ~NodeBase() = default;
            /**
             * 用来更新/设置mafFrameInFlight的
             * @param newMaxFrameInFlight
             */
            virtual void updateSynchronization(uint32_t newMaxFrameInFlight) = 0;
            virtual void addOutputEdge(CommandBufferEdges::EdgeBase* edge) {}
            virtual void addInputEdge(CommandBufferEdges::EdgeBase* edge) {}
            virtual void fillSynchronization(uint32_t frameIndex) {}
            virtual void removeOutputEdge(CommandBufferEdges::EdgeBase* edge) {}
            virtual void removeInputEdge(CommandBufferEdges::EdgeBase* edge) {}

            /**
             *  @brief  更新边的变化
             */
            virtual void fillAllSynchronization() {
                for (uint32_t i = 0; i < m_frameDirty.size(); ++i) {
                    fillSynchronization(i);
                }
            }
            void markFrameDirty(uint32_t frameIndex) {
                if (frameIndex < m_frameDirty.size()) {
                    m_frameDirty[frameIndex] = 1;
                }
            }

            void markFrameClean(uint32_t frameIndex) {
                if (frameIndex < m_frameDirty.size()) {
                    m_frameDirty[frameIndex] = 0;
                }
            }

            void markAllFramesDirty() {
                std::fill(m_frameDirty.begin(), m_frameDirty.end(), 1);
            }
        protected:
            std::vector<uint8_t> m_frameDirty;
        };
        class InputFromWindow : public NodeBase {
        public:
            InputFromWindow(Window* window) : m_window(window) {}
            void updateSynchronization(uint32_t newMaxFrameInFlight) override
            {
                m_collectedImageAvailable.clear();
                m_collectedImageAvailable.resize(newMaxFrameInFlight, nullptr);
                m_frameDirty.clear();
                m_frameDirty.resize(newMaxFrameInFlight, 1);
            }
            void addOutputEdge(CommandBufferEdges::EdgeBase* edge) override {
                markAllFramesDirty();
                if (auto* inputEdge = dynamic_cast<CommandBufferEdges::InputFromWindow*>(edge)) {
                    if (m_outputEdge != nullptr) {
                        throw std::runtime_error("InputFromWindow node can only have one output edge");
                    }
                    m_outputEdge = inputEdge;
                    return;
                }
                throw std::runtime_error("InputFromWindow node can only accept InputFromWindow edge type");
            }

            void addInputEdge(CommandBufferEdges::EdgeBase* edge) override {
                markAllFramesDirty();
                throw std::runtime_error("InputFromWindow node cannot accept input edges");
            }
            void removeOutputEdge(CommandBufferEdges::EdgeBase* edge) override {
                if (edge == m_outputEdge) {
                    m_outputEdge = nullptr;
                    markAllFramesDirty();
                }
            }

            void fillSynchronization(uint32_t frameIndex) override;

            RHI::Semaphore* getCollectedImageAvailableSemaphore(uint32_t frameIndex) const {
                if (frameIndex < m_collectedImageAvailable.size()) {
                    return m_collectedImageAvailable[frameIndex];
                }
                return nullptr;
            }
        private:
            Window* m_window;
            CommandBufferEdges::InputFromWindow* m_outputEdge = nullptr;
            std::vector<RHI::Semaphore*> m_collectedImageAvailable;
        };
        class OutputToWindow : public NodeBase
        {
        public:
            OutputToWindow(Window* window) : m_window(window) {}
            void updateSynchronization(uint32_t newMaxFrameInFlight) override {
                m_collectedFences.clear();
                m_collectedFences.resize(newMaxFrameInFlight);
                m_collectedRenderFinished.clear();
                m_collectedRenderFinished.resize(newMaxFrameInFlight);

                m_frameDirty.clear();
                m_frameDirty.resize(newMaxFrameInFlight, 1);
            }

            void addInputEdge(CommandBufferEdges::EdgeBase* edge) override {
                markAllFramesDirty();
                if (auto* outputEdge = dynamic_cast<CommandBufferEdges::OutputToWindow*>(edge)) {
                    m_inputEdges.push_back(outputEdge);
                    return;
                }
                throw std::runtime_error("OutputToWindow node can only accept OutputToWindow edge type");
            }

            void addOutputEdge(CommandBufferEdges::EdgeBase* edge) override {
                markAllFramesDirty();
                throw std::runtime_error("OutputToWindow node cannot accept output edges");
            }

            std::vector<RHI::Fence*> getCollectedFence(uint32_t frameIndex) const;
            std::vector<RHI::Semaphore*> getCollectedRenderFinishedSemaphore(uint32_t frameIndex) const;
            void fillSynchronization(uint32_t frameIndex) override;

            const std::vector<RHI::Fence*>& getCollectedFences(uint32_t frameIndex) const {
                static const std::vector<RHI::Fence*> empty;
                if (frameIndex < m_collectedFences.size()) {
                    return m_collectedFences[frameIndex];
                }
                return empty;
            }
            void removeInputEdge(CommandBufferEdges::EdgeBase* edge) override
            {
                for (auto it = m_inputEdges.begin(); it!= m_inputEdges.end(); ++it) {
                    if (*it == edge) {
                        m_inputEdges.erase(it);
                        markAllFramesDirty();
                        return;
                    }
                }
                throw std::runtime_error("OutputToWindow node does not have the specified input edge");
            }

            const std::vector<RHI::Semaphore*>& getCollectedRenderFinishedSemaphores(uint32_t frameIndex) const {
                static const std::vector<RHI::Semaphore*> empty;
                if (frameIndex < m_collectedRenderFinished.size()) {
                    return m_collectedRenderFinished[frameIndex];
                }
                return empty;
            }

        private:
            Window* m_window;
            std::vector<CommandBufferEdges::OutputToWindow*> m_inputEdges;
            std::vector<std::vector<RHI::Fence*>> m_collectedFences;                    // 收集的每帧的所有fence
            std::vector<std::vector<RHI::Semaphore*>> m_collectedRenderFinished;        // 收集的每帧的所有renderFinished信号量
        };

        class CommandBuffer : public NodeBase
        {
        public:
            CommandBuffer(RHI::CommandPool* pool);
            ~CommandBuffer() override;
            void updateSynchronization(uint32_t newMaxFrameInFlight) override;
            void addOutputEdge(CommandBufferEdges::EdgeBase* edge) override;
            void addInputEdge(CommandBufferEdges::EdgeBase* edge) override;
            RHI::CommandBuffer* getCommandBuffer(uint32_t frameIndex) const;
            void fillSynchronization(uint32_t frameIndex) override;

            std::vector<CommandBufferEdges::EdgeBase*>& getOutputEdges() {
                return m_outputEdges;
            }

            std::vector<CommandBufferEdges::EdgeBase*>& getInputEdges() {
                return m_inputEdges;
            }
            void removeInputEdge(CommandBufferEdges::EdgeBase* edge) override
            {
                auto it = std::find(m_inputEdges.begin(), m_inputEdges.end(), edge);
                if (it != m_inputEdges.end()) {
                    m_inputEdges.erase(it);
                    markAllFramesDirty();
                }
            }
            void removeOutputEdge(CommandBufferEdges::EdgeBase* edge) override
            {
                auto it = std::find(m_outputEdges.begin(), m_outputEdges.end(), edge);
                if (it != m_outputEdges.end()) {
                    m_outputEdges.erase(it);
                    markAllFramesDirty();
                }
            }
        private:
            std::vector<CommandBufferEdges::EdgeBase*> m_outputEdges;
            std::vector<CommandBufferEdges::EdgeBase*> m_inputEdges;
            std::vector<RHI::CommandBuffer*> m_cmdBuffers;
            RHI::CommandPool* m_pool;
        };
    }
    using CommandBufferToken = CommandBufferNodes::CommandBuffer*;
    class CommandBufferGraph
    {
    private:
        RHI::CommandPool* m_cmdPool;
        SemaphorePool* m_semaphorePool;
        FencePool* m_fencePool;

        std::unordered_set<CommandBufferNodes::NodeBase*> m_nodes;
        std::unordered_set<std::unique_ptr<CommandBufferEdges::EdgeBase>> m_edges;

        std::unordered_map<Window*, CommandBufferNodes::InputFromWindow*> m_windowInputNodes;
        std::unordered_map<Window*, CommandBufferNodes::OutputToWindow*> m_windowOutputNodes;
        std::set<Window*> m_windows;
        uint32_t m_maxFrameInFlight = 0;
        Device* m_device;

    public:
        CommandBufferGraph(Device* device);
        ~CommandBufferGraph();
        struct NodeRef {
            enum Type {
                WindowType,
                CommandBufferType
            };

            Type type;
            union {
                Window* window;
                CommandBufferNodes::CommandBuffer* commandBuffer;
            };

            NodeRef(Window* wnd) : type(WindowType), window(wnd) {}
            NodeRef(CommandBufferNodes::CommandBuffer* cb) : type(CommandBufferType), commandBuffer(cb) {}
        };
        CommandBufferToken addBuffer(
            const std::vector<NodeRef>& predecessors,
            const std::vector<NodeRef>& successors);
        void removeEdge(CommandBufferEdges::EdgeBase* edge);
        void removeBuffer(CommandBufferToken token);
        void maxFrameInFlight(uint32_t max);
        void swapBuffer();
        RHI::CommandBuffer* getCommandBuffer(CommandBufferToken token) const;

    private:
        CommandBufferNodes::InputFromWindow* getOrCreateWindowInputNode(Window* window);
        CommandBufferNodes::OutputToWindow* getOrCreateWindowOutputNode(Window* window);
        void connectPredecessors(CommandBufferNodes::CommandBuffer* buffer, const std::vector<NodeRef>& predecessors);
        void connectSuccessors(CommandBufferNodes::CommandBuffer* buffer, const std::vector<NodeRef>& successors);
        void connectWindowPredecessor(CommandBufferNodes::CommandBuffer* buffer, Window* window);
        void connectWindowSuccessor(CommandBufferNodes::CommandBuffer* buffer, Window* window);
        size_t m_frameIndex = 0;
    };



} // FCT

#endif //COMMANDBUFFERGRAPH_H

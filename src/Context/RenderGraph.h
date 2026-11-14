//
// Created by Administrator on 2025/8/10.
//

#ifndef RENDERGRAPH_H
#define RENDERGRAPH_H

#include "../Runtime/PipeHub.h"
#include "./ResourceManager.h"
#include "../RHI/ImageAspect.h"
#include "../RHI/ImageLayout.h"
#include "../RHI/PipelineStage.h"
#include "../RHI/AccessFlags.h"

#include "../ThirdParty.h"
#include "./Format.h"
#include "../Base/TokenGraph.h"
#include "../Base/UnionFind.h"
#include "../RHI/ImageUsageFlags.h"
#include "./ShaderStage.h"
#include "./ClearTypes.h"
#include "Vec.h"
#include "../Base/Graph.h"
#include "./SizeNode.h"
#include "CommandBufferGraph.h"
#include "../Base/UnionFind.h"
#include "../Base/IEventSystem.h"
#include "./FlowControl.h"
#include "./Device.h"

namespace FCT
{
    class TextureEdge;
    class Context;
    class Window;
    namespace RHI
    {
        class CommandBuffer;
    }
}
#include "./Image.h"
#include "./TransientResourcePool.h"
#include "./RenderGraph_EnablePassClear.h"
#include "./RenderGraph_TextureSize.h"
#include "./RenderGraph_Texture.h"
#include "./RenderGraph_Target.h"
#include "./RenderGraph_DepthStencil.h"
#include "./RenderGraph_PassDesc.h"
namespace FCT
{
    /**
     * 大小约定：
     *  同一个Pass的Target和DepthStencil的大小 相同
     *  Texture如果Undefined Size，在未指定大小的情况下 等同于FullSize
     *  Target如果 Undefined Size就会自动根据同一个Pass的Target和DepthStencil的大小来推导大小
     */
    class RenderGraphNode
    {
    protected:
        std::string m_name;
    public:
        RenderGraphNode() = default;
        RenderGraphNode(const std::string& name) : m_name(name) {}
        virtual ~RenderGraphNode() = default;
        const std::string& getName() const noexcept { return m_name; }
    };

    /**
     * @brief 渲染图资源组节点，支撑 PassNode <-> ResourceGroup <-> PassNode 拓扑
     */
    class RenderGraphResourceGroupNode : public RenderGraphNode
    {
    public:
        RenderGraphResourceGroupNode() = default;
        RenderGraphResourceGroupNode(const std::string& name, ResourceGroup* group = nullptr)
            : RenderGraphNode(name), m_group(group) {}

        ResourceGroup* getResourceGroup() const noexcept { return m_group; }
        void setResourceGroup(ResourceGroup* group) noexcept { m_group = group; }

        void addIncomingPass(const std::string& passName) {
            if (std::find(m_incomingPasses.begin(), m_incomingPasses.end(), passName) == m_incomingPasses.end()) {
                m_incomingPasses.push_back(passName);
            }
        }

        void addOutgoingPass(const std::string& passName) {
            if (std::find(m_outgoingPasses.begin(), m_outgoingPasses.end(), passName) == m_outgoingPasses.end()) {
                m_outgoingPasses.push_back(passName);
            }
        }

        const std::vector<std::string>& getIncomingPasses() const noexcept { return m_incomingPasses; }
        const std::vector<std::string>& getOutgoingPasses() const noexcept { return m_outgoingPasses; }

    private:
        ResourceGroup* m_group = nullptr;
        std::vector<std::string> m_incomingPasses;
        std::vector<std::string> m_outgoingPasses;
    };

    class Edge
    {
    public:
        virtual ~Edge() = default;
    };

    class TargetEdge : public Edge
    {
    public:
        std::string fromPass;
        std::string toImage;
        uint32_t order;
    };

    class DepthStencilEdge : public Edge
    {
    public:
        std::string fromPass;
        std::string toImage;
        uint32_t order;
    };

    class RenderGraphImageNode : public SizeNode, public RenderGraphNode
    {
    public:
        RenderGraphImageNode() = default;
        RenderGraphImageNode(const std::string& name) : RenderGraphNode(name), m_targetOrderCounter(0), m_depthStencilOrderCounter(0) {}
        virtual RenderGraphImageNode& operator|=(const Texture& other) = 0;
        virtual RenderGraphImageNode& operator|=(const Target& other) = 0;
        virtual RenderGraphImageNode& operator|=(const DepthStencil& other) = 0;

        void addOutgoingEdge(TextureEdge* edge) {
            m_textureOutgoingEdges.push_back(edge);
        }

        void addIncomingEdge(TargetEdge* edge) {
            edge->order = m_targetOrderCounter++;
            m_targetIncomingEdges.push_back(edge);
        }

        void addIncomingEdge(DepthStencilEdge* edge) {
            edge->order = m_depthStencilOrderCounter++;
            m_depthStencilIncomingEdges.push_back(edge);
        }

        void removeOutgoingEdge(TextureEdge* edge) {
            m_textureOutgoingEdges.erase(
                std::remove(m_textureOutgoingEdges.begin(), m_textureOutgoingEdges.end(), edge),
                m_textureOutgoingEdges.end()
            );
        }

        void removeIncomingEdge(TargetEdge* edge) {
            m_targetIncomingEdges.erase(
                std::remove(m_targetIncomingEdges.begin(), m_targetIncomingEdges.end(), edge),
                m_targetIncomingEdges.end()
            );
        }

        void removeIncomingEdge(DepthStencilEdge* edge) {
            m_depthStencilIncomingEdges.erase(
                std::remove(m_depthStencilIncomingEdges.begin(), m_depthStencilIncomingEdges.end(), edge),
                m_depthStencilIncomingEdges.end()
            );
        }

        const std::vector<TextureEdge*>& getTextureOutgoingEdges() const {
            return m_textureOutgoingEdges;
        }

        const std::vector<TargetEdge*>& getTargetIncomingEdges() const {
            return m_targetIncomingEdges;
        }

        const std::vector<DepthStencilEdge*>& getDepthStencilIncomingEdges() const {
            return m_depthStencilIncomingEdges;
        }
        virtual Image* getImage() const = 0;
        virtual void fillDefaultData() = 0;
    private:
        std::vector<TextureEdge*> m_textureOutgoingEdges;
        std::vector<TargetEdge*> m_targetIncomingEdges;
        std::vector<DepthStencilEdge*> m_depthStencilIncomingEdges;
        uint32_t m_targetOrderCounter;
        uint32_t m_depthStencilOrderCounter;
    };
}
#include "./RenderGraph_RenderGraphBufferNode.h"
namespace FCT
{
    class RenderGraphPassNode;
    class TextureEdge : public Edge
    {
    public:
        TextureSize size;
        std::string fromImage;
        std::string toPass;
        /**
         * @cond CHIESE
         * @brief 最早被使用的阶段
         * @endcond
         */
        ShaderStage stage;
        bool isStorage = false;
        uint32_t order = 0;
    };

    class RenderGraphPassNode : public RenderGraphNode
    {
    public:
        RenderGraphPassNode(){}

        RenderGraphPassNode(std::string name) : RenderGraphNode(name)
        {

        }

        void addIncomingEdge(TextureEdge* edge) {
            m_textureIncomingEdges.push_back(edge);
        }

        void addOutgoingEdge(TargetEdge* edge) {
            m_targetOutgoingEdges.push_back(edge);
        }

        void addOutgoingEdge(DepthStencilEdge* edge) {
            m_depthStencilOutgoingEdges.push_back(edge);
        }

        const std::vector<TextureEdge*>& getTextureIncomingEdges() const {
            return m_textureIncomingEdges;
        }

        const std::vector<TargetEdge*>& getTargetOutgoingEdges() const {
            return m_targetOutgoingEdges;
        }

        const std::vector<DepthStencilEdge*>& getDepthStencilOutgoingEdges() const {
            return m_depthStencilOutgoingEdges;
        }
        void removeIncomingEdge(TextureEdge* edge) {
            m_textureIncomingEdges.erase(
                std::remove(m_textureIncomingEdges.begin(), m_textureIncomingEdges.end(), edge),
                m_textureIncomingEdges.end()
            );
        }

        void removeOutgoingEdge(TargetEdge* edge) {
            m_targetOutgoingEdges.erase(
                std::remove(m_targetOutgoingEdges.begin(), m_targetOutgoingEdges.end(), edge),
                m_targetOutgoingEdges.end()
            );
        }

        void removeOutgoingEdge(DepthStencilEdge* edge) {
            m_depthStencilOutgoingEdges.erase(
                std::remove(m_depthStencilOutgoingEdges.begin(), m_depthStencilOutgoingEdges.end(), edge),
                m_depthStencilOutgoingEdges.end()
            );
        }

        void applyPassDesc(const PassDesc& desc) {
            m_clearInfo = desc.clear;
        }
        const EnablePassClear& getClearInfo() const {
            return m_clearInfo;
        }

        void addIncomingResourceGroup(const std::string& groupName) {
            if (std::find(m_incomingResourceGroups.begin(), m_incomingResourceGroups.end(), groupName) == m_incomingResourceGroups.end()) {
                m_incomingResourceGroups.push_back(groupName);
            }
        }

        void addOutgoingResourceGroup(const std::string& groupName) {
            if (std::find(m_outgoingResourceGroups.begin(), m_outgoingResourceGroups.end(), groupName) == m_outgoingResourceGroups.end()) {
                m_outgoingResourceGroups.push_back(groupName);
            }
        }

        const std::vector<std::string>& getIncomingResourceGroups() const noexcept { return m_incomingResourceGroups; }
        const std::vector<std::string>& getOutgoingResourceGroups() const noexcept { return m_outgoingResourceGroups; }

    private:
        std::vector<TextureEdge*> m_textureIncomingEdges;
        std::vector<TargetEdge*> m_targetOutgoingEdges;
        std::vector<DepthStencilEdge*> m_depthStencilOutgoingEdges;
        std::vector<std::string> m_incomingResourceGroups;
        std::vector<std::string> m_outgoingResourceGroups;
        EnablePassClear m_clearInfo;
    };
    struct PassSubmitEvent
    {
        std::string passName;
        RHI::CommandBuffer* cmdBuf;
    };

    namespace RenderGraphTickers
    {
        constexpr const char* RenderGraphSubmit = "RenderGraphSubmit";
        constexpr const char* CheckRecompiledSync = "RenderGraphCheckRecompiled";
    }
    class RenderGraph : private IEventSystem<EventSystemConfig::TriggerOnly>{
        struct ImageState {
            ImageLayout currentLayout = ImageLayout::undefined;
            PipelineStages lastStage = PipelineStage::topOfPipe;
            AccessFlags lastAccess = AccessFlag::none;
            std::string lastWriterGroup;
            bool needsBarrier = false;
        };
        struct BufferState {
            PipelineStages lastStage = PipelineStage::topOfPipe;
            AccessFlags lastAccess = AccessFlag::none;
            std::string lastWriterGroup;
            bool needsBarrier = false;
        };
        struct BarrierInfo {
            bool isBuffer = false;
            Image* image = nullptr;
            Buffer* buffer = nullptr;
            size_t bufferOffset = 0;
            size_t bufferSize = 0;
            ImageLayout oldLayout = ImageLayout::undefined;
            ImageLayout newLayout = ImageLayout::undefined;
            PipelineStages srcStage = PipelineStage::none;
            PipelineStages dstStage = PipelineStage::none;
            AccessFlags srcAccess = AccessFlag::none;
            AccessFlags dstAccess = AccessFlag::none;
            ImageAspects aspect = ImageAspect::color;
        };
        struct EdgeHash {
            size_t operator()(const UniquePtr<Edge>& p) const noexcept {
                return std::hash<Edge*>{}(p.get());
            }
        };
        struct EdgeEqual {
            bool operator()(const UniquePtr<Edge>& a, const UniquePtr<Edge>& b) const noexcept {
                return a.get() == b.get();
            }
        };
    private:
        bool m_needRecompiled = false;
        Device* m_resourceDevice;
        FlowControl* m_flowControl;
        CommandBufferGraph* m_commandBufferGraph;
        ResourceManager* m_resourceManager;
    private:
        /**
         * @cond CHINESE
         *  todo:修改以做多CommandBufferGraph支持
         * @endcond
         */
        CommandBufferToken m_commandBufferToken;
        void initForSubmit();
        void allocateCommandBuffer();
    private:
        std::unordered_map<std::string, std::vector<std::string>> m_passGroupOrders;
        std::vector<std::string> m_passGroupExecutionOrder;
    public:
    private:
        std::unordered_map<std::string, std::vector<BarrierInfo>> m_passGroupBarriers;
        std::map<std::string, std::set<std::string>> m_passGroupDependencies;
        std::unordered_map<std::string, UniquePtr<RenderGraphImageNode>> m_imageNodes;
        std::unordered_map<std::string,RenderGraphPassNode> m_passNodes;
        UnionFind<std::string,char> m_passesUnions;
        std::unordered_set<UniquePtr<Edge>, EdgeHash, EdgeEqual> m_edges;
        std::unordered_map<std::string, UniquePtr<RenderGraphResourceGroupNode>> m_resourceGroupNodes;
        std::unordered_map<std::string, Image*> m_allocatedImages;
        std::unordered_map<std::string, RHI::Pass*> m_allocatedPasses;
        std::vector<PassDesc> m_originalPasses;
        std::unordered_map<std::string, PassDesc> m_passDescs;
        std::vector<std::string> m_topologicalSortPasses;
        std::unordered_map<std::string, RHI::PassGroup*> m_allocatedPassGroups; // 存储创建的PassGroup
        void cleanUpCompile();
        PipeHub& pipeHub;
    private:
        PipelineStage convertShaderStageToPipelineStage(ShaderStage stage) const;
    public:
        RenderGraph(PipeHub& pipeHub, Device* device, FlowControl* flowControl, CommandBufferGraph* commandBufferGraph,
                    ResourceManager* resourceManager);
        ~RenderGraph();
        void recompile();
        std::vector<PassDesc> getOriginalPasses() const
        {
            return m_originalPasses;
        }
        void clearOriginalPasses()
        {
            m_originalPasses.clear();
            m_passDescs.clear();
        }

        RenderGraphResourceGroupNode* getOrCreateResourceGroupNode(const std::string& name);
        void connectPassToResourceGroup(const std::string& passName, const std::string& groupName);
        void connectResourceGroupToPass(const std::string& groupName, const std::string& passName);
        void addResourceGroupEdge(const std::string& fromPass, const std::string& groupName, const std::string& toPass);
        ResourceGroup* getResourceGroup(const std::string& name) const;
        const std::unordered_map<std::string, UniquePtr<RenderGraphResourceGroupNode>>& getResourceGroupNodes() const noexcept { return m_resourceGroupNodes; }

    private:
        RenderGraphImageNode* getOrCreateImageNode(const std::string& name, const Texture& texture);
        RenderGraphImageNode* getOrCreateImageNode(const std::string& name, const Target& target);
        RenderGraphImageNode* getOrCreateImageNode(const std::string& name, const DepthStencil& depthStencil);
        void createTextureEdge(const std::string& passName, const std::string& textureName, const Texture& texture);
        void createTargetEdge(const std::string& passName, const std::string& targetName, const Target& target);
        void createDepthStencilEdge(const std::string& passName, const std::string& depthStencilName, const DepthStencil& depthStencil);
        void removeTextureEdge(TextureEdge* edgeToRemove);
        void simulatePassGroupExecution(const std::string& groupLeader,
                                        const std::vector<std::string>& groupMembers,
                                        std::unordered_map<Image*, ImageState>& imageStates,
                                        std::unordered_map<Buffer*, BufferState>& bufferStates);
        void simulateExecutionAndAnalyzeBarriers();
        std::vector<BarrierInfo> checkBarriersBeforePassGroup(const std::string& groupLeader,
                                                                const std::vector<std::string>& groupMembers,
                                                                const std::unordered_map<Image*, ImageState>& imageStates,
                                                                const std::unordered_map<Buffer*, BufferState>& bufferStates);

        void executeBarriers(RHI::CommandBuffer* cmdBuffer, const std::vector<BarrierInfo>& barriers);
        void removeTargetEdge(TargetEdge* edgeToRemove);
        void removeDepthStencilEdge(DepthStencilEdge* edgeToRemove);
        void addPass(const PassDesc& desc);
        void cleanUp();
        /**
         * @cond CHINESE
         * @note 在compile调用时执行，遍历每个pass，
         *       然后遍历pass的texture边，
         *       如果TextureSize.type=Undefined，
         *       并且调用对应texture的SizeNode的isSizeDetermined为false，
         *       那么就调用SizeNode的
         *       bool unite(SizeNode* other, const Scale2D& scale = Scale2D())
         *       把texture和任意一个target或depthStencilunite起来，
         *       如果TextureSize.type=Relative，
         *       不管texture的size决定没有，
         *       把texture和（target或depth stencil 其中的一个）unite起来，
         *       并把传
         *       Scale2D(TextureSize.relativeWidth,TextureSize.relativeHeight)
         *       进去
         * @endcond
         */
        void resolveTextureSizes();
        void cullPasses();
        void cullPass(const std::string& passName);
        void cullSinglePass(const std::string& passName);
        void cullUnusedImageNode(const std::string& imageName);
        /**
          * @cond CHINESE
          * @note 在compile时调用，用于将Pass分组到PassGroup中
          *       1. 遍历所有pass，首先pass先unite自己
          *       2. 遍历texture边对应的image和(任一target或depthStencil)是否相同大小
          *       3. 如果相同大小，找到把texture当target的pass，unite起来
          *       4. 遍历所有image，遍历他们的target边和depthstencil边，fromPass全部unite起来
          * @endcond
          */
        void groupPasses();
        void clearResources();

        void allocateResources();
        /**
        * @cond CHINESE
        * @note 创建所有RHI Pass并绑定对应的图片资源
        *       遍历所有pass节点，为每个pass创建RHI::Pass
        *       通过ImageNode的getImage()获取对应的Image并绑定到Pass
        *       Target按order绑定，DepthStencil直接绑定，Texture不绑定
        * @endcond
        */
        void createRHIPasses();
        void analyzePassGroupDependencies(const std::string& groupLeader, const std::vector<std::string>& groupMembers,
                                          std::map<std::string, std::set<std::string>>& dependencies) const;
        std::vector<std::string> topologicalSortPasses() const;
        std::map<std::string, std::set<std::string>> buildPassDependencyGraph() const;
        std::vector<std::string> topologicalSort(const std::map<std::string, std::set<std::string>>& dependencies) const;
        void analyzePassGroupBarriers();
        void computePassGroupExecutionOrder();
        /**
          * @cond CHINESE
          * @note 创建PassGroup并设置Pass之间的依赖关系
          *       根据m_passesUnions的分组结果创建PassGroup
          *       通过TextureEdge分析Pass之间的依赖关系
          *       设置正确的PipelineStage和AccessFlags
          *       注意点:
          *       1.PassGroup内部在create的时候会把找不到的Pass*设置为external
          *       2.
          * @endcond
          */
        void createPassGroups();
    public:
        template<typename Func>
        SubscribeId subscribe(std::string passName,Func&& func)
        {
            SubscribeId subId = IEventSystem<EventSystemConfig::TriggerOnly>::subscribe<PassSubmitEvent>([passName, func](const PassSubmitEvent& env)
            {
                if (env.passName == passName)
                {
                    func(env);
                }
            });
            return subId;
        }
        void unsubscribe(SubscribeId subscribeId)
        {
            IEventSystem<EventSystemConfig::TriggerOnly>::unsubscribe(subscribeId);
        }
        template<typename... Args>
        void addPass(std::string name, Args&&... args)
        {
            PassDesc desc(name);
            desc.processArgs(std::forward<Args>(args)...);
            addPassDesc(desc);
        }

        template<typename... Args>
        void addComputePass(std::string name, IComputeDispatcher* dispatcher, Args&&... args)
        {
            PassDesc desc(name);
            desc.type = PassType::Compute;
            desc.computeDispatcher = dispatcher;
            desc.processArgs(std::forward<Args>(args)...);
            addPassDesc(desc);
        }

        template<typename... Args>
        void addExternalInteropPass(std::string name, IComputeDispatcher* dispatcher, Args&&... args)
        {
            PassDesc desc(name);
            desc.type = PassType::ExternalInterop;
            desc.computeDispatcher = dispatcher;
            desc.processArgs(std::forward<Args>(args)...);
            addPassDesc(desc);
        }

        void addPassDesc(const PassDesc& desc)
        {
            m_originalPasses.push_back(desc);
            m_passDescs[desc.name] = desc;
        }
        void buildGraph()
        {
            for (auto pass : m_originalPasses)
            {
                addPass(pass);
            }
        }
        void compile()
        {
            ZoneScoped();
            buildGraph();
            allocateCommandBuffer();
            m_topologicalSortPasses = topologicalSortPasses();
            resolveTextureSizes();
            cullPasses();
            groupPasses();
            allocateResources();
            createRHIPasses();
            createPassGroups();
            computePassGroupExecutionOrder();
            pushPipe();
        }
        void pushPipe();
        void executeAllPassGroups(RHI::CommandBuffer* cmdBuffer);
        RHI::Pass* getPass(const std::string& name) const;
        Image* getImage(const std::string& name) const;
        std::vector<TextureEdge*> getTextureEdges(const std::string& passName) const;
        TransientResourcePool* getTransientResourcePool() const noexcept { return m_transientResourcePool.get(); }
        bool canMergePasses(const RenderGraphPassNode& a, const RenderGraphPassNode& b) const;
    private:
        UniquePtr<TransientResourcePool> m_transientResourcePool;
        uint32_t m_currentFrameIndex{0};
        void submitPassGroup(RHI::CommandBuffer* cmdBuffer,
                             const std::string& groupLeader);

    };

} // FCT

#endif //RENDERGRAPH_H

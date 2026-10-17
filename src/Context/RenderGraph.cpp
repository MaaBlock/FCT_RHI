#include "../UI/Window.h"
#include "./Context.h"
#include "./Context.hpp"
#include "../RHI/PassGroup.h"

namespace FCT
{
    Image* RenderGraphWindowTargetNode::getImage() const
    {
        return m_window->getCurrentTarget()->targetImage();
    }

    Image* RenderGraphWindowDepthStencilNode::getImage() const
    {
        return m_window->getCurrentTarget()->depthStencilBuffer();
    }

    void RenderGraph::initForSubmit()
    {
        m_commandBufferToken = nullptr;
        auto& submitGraph = m_flowControl->submitTickers();
        submitGraph[RenderGraphTickers::RenderGraphSubmit] = {[this]()
        {
            auto cmdGraph = m_commandBufferGraph;
            auto cmdBuf = cmdGraph->getCommandBuffer(m_commandBufferToken);
            cmdBuf->reset();
            cmdBuf->begin();
            executeAllPassGroups(cmdBuf);
            cmdBuf->end();
            cmdBuf->submit();
        },{},
        {SwapBufferSubmitTicker}
        };
        submitGraph.update();
    }

    void RenderGraph::allocateCommandBuffer()
    {
        auto cmdGraph = m_commandBufferGraph;

        if (m_commandBufferToken)
        {
            /**
             * @cond CHINESE
             *  todo: 添加移除机制来支持重新编译
             * @endcond
             **/
        }
        std::vector<CommandBufferGraph::NodeRef> windowNodes;

        for (const auto& [imageName, imageNode] : m_imageNodes) {
            Window* window = nullptr;

            auto* windowTargetNode = dynamic_cast<RenderGraphWindowTargetNode*>(imageNode.get());
            if (windowTargetNode && windowTargetNode->isValidWindowTarget()) {
                window = windowTargetNode->getWindow();
            }

            if (!window) {
                auto* windowDepthNode = dynamic_cast<RenderGraphWindowDepthStencilNode*>(imageNode.get());
                if (windowDepthNode && windowDepthNode->isValidWindowDepthStencil()) {
                    window = windowDepthNode->getWindow();
                }
            }

            if (window) {
                bool alreadyAdded = false;
                for (const auto& node : windowNodes) {
                    if (node.type == CommandBufferGraph::NodeRef::WindowType && node.window == window) {
                        alreadyAdded = true;
                        break;
                    }
                }
                if (!alreadyAdded) {
                    windowNodes.emplace_back(window);
                }
            }
        }

        m_commandBufferToken = cmdGraph->addBuffer(windowNodes, windowNodes);
    }

    RenderGraph::RenderGraph(PipeHub& pipeHub,Device* device, FlowControl* flowControl, CommandBufferGraph* commandBufferGraph,
        ResourceManager* resourceManager) : pipeHub(pipeHub)
    {
        m_resourceDevice = device;
        m_flowControl = flowControl;
        m_commandBufferGraph = commandBufferGraph;
        m_resourceManager = resourceManager;
        initForSubmit();
        auto& syncTickers = m_flowControl->syncTickers();
        syncTickers[RenderGraphTickers::CheckRecompiledSync] = {
            [this]()
            {
               if (m_needRecompiled)
               {
                   cleanUpCompile();
                   compile();
                   m_needRecompiled = false;
               }
            },
            {},
            {}
        };
        syncTickers.update();
    }

    RenderGraph::~RenderGraph()
    {
        auto& syncTickers = m_flowControl->syncTickers();
        syncTickers.removeNode(RenderGraphTickers::CheckRecompiledSync);
        syncTickers.update();
        auto& submitGraph = m_flowControl->submitTickers();
        submitGraph.removeNode(RenderGraphTickers::RenderGraphSubmit);
        submitGraph.update();
    }

    RenderGraphImageNode* RenderGraph::getOrCreateImageNode(const std::string& name, const Texture& texture)
    {
        auto it = m_imageNodes.find(name);
        if (it != m_imageNodes.end()) {
            *(it->second) |= texture;
            return it->second.get();
        } else {
            auto node = std::make_unique<RenderGraphBufferNode>(texture);
            RenderGraphImageNode* nodePtr = node.get();
            m_imageNodes[name] = std::move(node);
            return nodePtr;
        }
    }

    RenderGraphImageNode* RenderGraph::getOrCreateImageNode(const std::string& name, const Target& target)
    {
        auto it = m_imageNodes.find(name);
        if (it != m_imageNodes.end()) {
            *(it->second) |= target;
            return it->second.get();
        } else {
            if (target.isWindow) {
                auto node = std::make_unique<RenderGraphWindowTargetNode>(target);
                RenderGraphImageNode* nodePtr = node.get();
                m_imageNodes[name] = std::move(node);
                return nodePtr;
            } else {
                auto node = std::make_unique<RenderGraphBufferNode>(target);
                RenderGraphImageNode* nodePtr = node.get();
                m_imageNodes[name] = std::move(node);
                return nodePtr;
            }
        }
    }

    RenderGraphImageNode* RenderGraph::getOrCreateImageNode(const std::string& name, const DepthStencil& depthStencil)
    {
        auto it = m_imageNodes.find(name);
        if (it != m_imageNodes.end()) {
            *(it->second) |= depthStencil;
            return it->second.get();
        } else {
            if (depthStencil.isWindow) {
                auto node = std::make_unique<RenderGraphWindowDepthStencilNode>(depthStencil);
                RenderGraphImageNode* nodePtr = node.get();
                m_imageNodes[name] = std::move(node);
                return nodePtr;
            } else {
                auto node = std::make_unique<RenderGraphBufferNode>(depthStencil);
                RenderGraphImageNode* nodePtr = node.get();
                m_imageNodes[name] = std::move(node);
                return nodePtr;
            }
        }
    }

    void RenderGraph::createTextureEdge(const std::string& passName, const std::string& textureName,
        const Texture& texture)
    {
        auto& pass = m_passNodes[passName];

        auto edge = std::make_unique<TextureEdge>();
        edge->size = texture.size;
        edge->fromImage = textureName;
        edge->toPass = passName;
        edge->stage = getEarliestStage(texture.stages);

        TextureEdge* edgePtr = edge.get();

        auto imageNodeIt = m_imageNodes.find(textureName);
        if (imageNodeIt != m_imageNodes.end()) {
            imageNodeIt->second->addOutgoingEdge(edgePtr);
        }
        pass.addIncomingEdge(edgePtr);

        m_edges.insert(std::move(edge));
    }

    void RenderGraph::createTargetEdge(const std::string& passName, const std::string& targetName, const Target& target)
    {
        auto& pass = m_passNodes[passName];

        auto edge = std::make_unique<TargetEdge>();
        edge->fromPass = passName;
        edge->toImage = targetName;

        TargetEdge* edgePtr = edge.get();

        auto imageNodeIt = m_imageNodes.find(targetName);
        if (imageNodeIt != m_imageNodes.end()) {
            imageNodeIt->second->addIncomingEdge(edgePtr);
        }
        pass.addOutgoingEdge(edgePtr);

        m_edges.insert(std::move(edge));
    }

    void RenderGraph::createDepthStencilEdge(const std::string& passName, const std::string& depthStencilName,
        const DepthStencil& depthStencil)
    {
        auto& pass = m_passNodes[passName];

        auto edge = std::make_unique<DepthStencilEdge>();
        edge->fromPass = passName;
        edge->toImage = depthStencilName;

        DepthStencilEdge* edgePtr = edge.get();

        auto imageNodeIt = m_imageNodes.find(depthStencilName);
        if (imageNodeIt != m_imageNodes.end()) {
            imageNodeIt->second->addIncomingEdge(edgePtr);
        }
        pass.addOutgoingEdge(edgePtr);

        m_edges.insert(std::move(edge));
    }
void RenderGraph::removeTextureEdge(TextureEdge* edgeToRemove) {
    if (!edgeToRemove) return;

    auto passIt = m_passNodes.find(edgeToRemove->toPass);
    if (passIt != m_passNodes.end()) {
        auto& pass = passIt->second;
        auto& incomingEdges = const_cast<std::vector<TextureEdge*>&>(pass.getTextureIncomingEdges());
        incomingEdges.erase(
            std::remove(incomingEdges.begin(), incomingEdges.end(), edgeToRemove),
            incomingEdges.end()
        );
    }

    auto imageNodeIt = m_imageNodes.find(edgeToRemove->fromImage);
    if (imageNodeIt != m_imageNodes.end()) {
        imageNodeIt->second->removeOutgoingEdge(edgeToRemove);
    }

    auto it = std::find_if(m_edges.begin(), m_edges.end(),
        [edgeToRemove](const std::unique_ptr<Edge>& edge) {
            return edge.get() == edgeToRemove;
        });

    if (it != m_edges.end()) {
        m_edges.erase(it);
    }
}

void RenderGraph::removeTargetEdge(TargetEdge* edgeToRemove) {
    if (!edgeToRemove) return;

    auto passIt = m_passNodes.find(edgeToRemove->fromPass);
    if (passIt != m_passNodes.end()) {
        auto& pass = passIt->second;
        auto& outgoingEdges = const_cast<std::vector<TargetEdge*>&>(pass.getTargetOutgoingEdges());
        outgoingEdges.erase(
            std::remove(outgoingEdges.begin(), outgoingEdges.end(), edgeToRemove),
            outgoingEdges.end()
        );
    }

    auto imageNodeIt = m_imageNodes.find(edgeToRemove->toImage);
    if (imageNodeIt != m_imageNodes.end()) {
        imageNodeIt->second->removeIncomingEdge(edgeToRemove);
    }

    auto it = std::find_if(m_edges.begin(), m_edges.end(),
        [edgeToRemove](const std::unique_ptr<Edge>& edge) {
            return edge.get() == edgeToRemove;
        });

    if (it != m_edges.end()) {
        m_edges.erase(it);
    }
}

void RenderGraph::removeDepthStencilEdge(DepthStencilEdge* edgeToRemove) {
    if (!edgeToRemove) return;

    // 从 pass 的 outgoing edges 中移除
    auto passIt = m_passNodes.find(edgeToRemove->fromPass);
    if (passIt != m_passNodes.end()) {
        auto& pass = passIt->second;
        auto& outgoingEdges = const_cast<std::vector<DepthStencilEdge*>&>(pass.getDepthStencilOutgoingEdges());
        outgoingEdges.erase(
            std::remove(outgoingEdges.begin(), outgoingEdges.end(), edgeToRemove),
            outgoingEdges.end()
        );
    }

    // 从 image node 的 incoming edges 中移除
    auto imageNodeIt = m_imageNodes.find(edgeToRemove->toImage);
    if (imageNodeIt != m_imageNodes.end()) {
        imageNodeIt->second->removeIncomingEdge(edgeToRemove);
    }

    // 从 m_edges 中移除并销毁
    auto it = std::find_if(m_edges.begin(), m_edges.end(),
        [edgeToRemove](const std::unique_ptr<Edge>& edge) {
            return edge.get() == edgeToRemove;
        });

    if (it != m_edges.end()) {
        m_edges.erase(it);
    }
}
    void RenderGraph::addPass(const PassDesc& desc)
    {
        m_passNodes[desc.name] = desc.name;
        auto& pass = m_passNodes[desc.name];

        pass.applyPassDesc(desc);

        for (auto& texture : desc.textures)
        {
            getOrCreateImageNode(texture.name, texture);
            createTextureEdge(desc.name, texture.name, texture);
        }

        std::vector<RenderGraphImageNode*> renderTargetNodes;

        for (auto& target : desc.targets)
        {
            auto* node = getOrCreateImageNode(target.name, target);
            renderTargetNodes.push_back(node);
            createTargetEdge(desc.name, target.name, target);
        }

        for (auto& depthStencil : desc.depthStencils)
        {
            auto* node = getOrCreateImageNode(depthStencil.name, depthStencil);
            renderTargetNodes.push_back(node);
            createDepthStencilEdge(desc.name, depthStencil.name, depthStencil);
        }

        if (renderTargetNodes.size() > 1) {
            RenderGraphImageNode* firstNode = renderTargetNodes[0];
            for (size_t i = 1; i < renderTargetNodes.size(); ++i) {
                firstNode->unite(renderTargetNodes[i]);
            }
        }
    }

    void RenderGraph::recompile()
    {
        m_needRecompiled = true;
        /*
         *
        cleanUpCompile();
        compile();
        */
    }

    void RenderGraph::resolveTextureSizes()
    {

        for (auto& passName : m_topologicalSortPasses) {
            auto& pass = m_passNodes[passName];
            std::vector<RenderGraphImageNode*> referenceNodes;

            for (auto* targetEdge : pass.getTargetOutgoingEdges()) {
                auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                if (imageNodeIt != m_imageNodes.end()) {
                    referenceNodes.push_back(imageNodeIt->second.get());
                }
            }

            for (auto* depthStencilEdge : pass.getDepthStencilOutgoingEdges()) {
                auto imageNodeIt = m_imageNodes.find(depthStencilEdge->toImage);
                if (imageNodeIt != m_imageNodes.end()) {
                    referenceNodes.push_back(imageNodeIt->second.get());
                }
            }

            if (referenceNodes.empty()) {
                continue;
            }

            RenderGraphImageNode* referenceNode = referenceNodes[0];

            for (auto* textureEdge : pass.getTextureIncomingEdges()) {
                auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                if (imageNodeIt == m_imageNodes.end()) {
                    continue;
                }

                RenderGraphImageNode* textureNode = imageNodeIt->second.get();

                if (textureEdge->size.type == TextureSize::Type::Undefined) {
                    if (!textureNode->isSizeDetermined()) {//
                        textureEdge->size = TextureSize(Fraction(),Fraction());
                        //textureNode->unite(referenceNode);
                    }
                }
                if (textureEdge->size.type == TextureSize::Type::Relative) {
                    FractionScale2D scale(textureEdge->size.relativeWidth, textureEdge->size.relativeHeight);
                    textureNode->unite(referenceNode, scale);
                }
            }
        }
    }

    void RenderGraph::groupPasses()
    {
        for (const auto& [passName, passNode] : m_passNodes) {
            m_passesUnions.unite(passName, passName);
        }

        for (const auto& [passName, passNode] : m_passNodes) {
            for (auto* textureEdge : passNode.getTextureIncomingEdges()) {
                std::string imageName = textureEdge->fromImage;

                auto imageIt = m_imageNodes.find(imageName);
                if (imageIt == m_imageNodes.end()) continue;

                RenderGraphImageNode* imageNode = imageIt->second.get();

                RenderGraphImageNode* passRenderTarget = nullptr;

                for (auto* targetEdge : passNode.getTargetOutgoingEdges()) {
                    auto targetIt = m_imageNodes.find(targetEdge->toImage);
                    if (targetIt != m_imageNodes.end()) {
                        passRenderTarget = targetIt->second.get();
                        break;
                    }
                }

                if (!passRenderTarget) {
                    for (auto* depthEdge : passNode.getDepthStencilOutgoingEdges()) {
                        auto depthIt = m_imageNodes.find(depthEdge->toImage);
                        if (depthIt != m_imageNodes.end()) {
                            passRenderTarget = depthIt->second.get();
                            break;
                        }
                    }
                }

                if (passRenderTarget && imageNode->isSameSize(passRenderTarget)) {
                    for (auto* targetEdge : imageNode->getTargetIncomingEdges()) {
                        m_passesUnions.unite(passName, targetEdge->fromPass);
                    }

                    for (auto* depthEdge : imageNode->getDepthStencilIncomingEdges()) {
                        m_passesUnions.unite(passName, depthEdge->fromPass);
                    }
                }
            }
        }

        for (const auto& [imageName, imageNode] : m_imageNodes) {
            std::vector<std::string> targetPasses;
            std::vector<std::string> depthStencilPasses;

            for (auto* targetEdge : imageNode->getTargetIncomingEdges()) {
                targetPasses.push_back(targetEdge->fromPass);
            }

            for (auto* depthEdge : imageNode->getDepthStencilIncomingEdges()) {
                depthStencilPasses.push_back(depthEdge->fromPass);
            }

            for (size_t i = 1; i < targetPasses.size(); i++) {
                m_passesUnions.unite(targetPasses[0], targetPasses[i]);
            }

            for (size_t i = 1; i < depthStencilPasses.size(); i++) {
                m_passesUnions.unite(depthStencilPasses[0], depthStencilPasses[i]);
            }

            if (!targetPasses.empty() && !depthStencilPasses.empty()) {
                m_passesUnions.unite(targetPasses[0], depthStencilPasses[0]);
            }
        }
    }
    void RenderGraph::clearResources()
    {
        for (auto& [name, image] : m_allocatedImages)
        {
            m_resourceManager->clearImage(name);
        }
        m_allocatedImages.clear();
    }

    void RenderGraph::allocateResources()
    {
        for (auto& [name, imageNode] : m_imageNodes) {
            imageNode->fillDefaultData();
        }
        auto* resourceManager = m_resourceManager;
        if (!resourceManager) {
            throw std::runtime_error("ResourceManager not available");
        }

        std::set<const SizeNode*> processedRoots;

        for (auto& [name, imageNode] : m_imageNodes) {
            auto* bufferNode = dynamic_cast<RenderGraphBufferNode*>(imageNode.get());
            if (!bufferNode || !bufferNode->isFilled()) {
                continue;
            }

            const SizeNode* root = bufferNode->getRoot();

            if (root == bufferNode && processedRoots.find(root) == processedRoots.end()) {
                processedRoots.insert(root);

                uint32_t width, height;
                bool hasFixedSize = bufferNode->getComputedSize(width, height);

                if (hasFixedSize) {
                    TargetDesc targetDesc;
                    targetDesc.width = static_cast<int>(width);
                    targetDesc.height = static_cast<int>(height);
                    targetDesc.samples = bufferNode->getSamples();
                    targetDesc.format = bufferNode->getFormat();
                    targetDesc.usage = bufferNode->getUsage();

                    Image* allocatedImage = resourceManager->allocateTarget(name, targetDesc);
                    bufferNode->setAllocatedImage(allocatedImage); // 调用setAllocatedImage保存图像
                    m_allocatedImages[name] = allocatedImage; // 存储分配的图像
                } else {
                    throw std::runtime_error("Root node has no fixed size: " + name);
                }
            }
        }

        for (auto& [name, imageNode] : m_imageNodes) {
            auto* bufferNode = dynamic_cast<RenderGraphBufferNode*>(imageNode.get());
            if (!bufferNode || !bufferNode->isFilled()) {
                continue;
            }

            const SizeNode* root = bufferNode->getRoot();

            if (root != bufferNode) {
                ImageDesc imageDesc;
                imageDesc.format = bufferNode->getFormat();
                imageDesc.samples = bufferNode->getSamples();
                imageDesc.usage = bufferNode->getUsage();

                Image* allocatedImage = nullptr;

                std::string dependencyName;
                for (auto& [otherName, otherNode] : m_imageNodes) {
                    if (otherNode->getRoot() == root && otherNode->getRoot() == otherNode.get()) {
                        dependencyName = otherName;
                        break;
                    }
                }

                if (!dependencyName.empty()) {
                    auto* windowTargetNode = dynamic_cast<RenderGraphWindowTargetNode*>(
                        m_imageNodes[dependencyName].get());
                    auto* windowDepthNode = dynamic_cast<RenderGraphWindowDepthStencilNode*>(
                        m_imageNodes[dependencyName].get());

                    if (windowTargetNode && windowTargetNode->isValidWindowTarget()) {
                        allocatedImage = resourceManager->allocateImage(
                            name, windowTargetNode->getWindow(), imageDesc);
                    } else if (windowDepthNode && windowDepthNode->isValidWindowDepthStencil()) {
                        allocatedImage = resourceManager->allocateImage(
                            name, windowDepthNode->getWindow(), imageDesc);
                    } else {
                        allocatedImage = resourceManager->allocateImage(
                            name, dependencyName, imageDesc);
                    }

                    bufferNode->setAllocatedImage(allocatedImage);
                    m_allocatedImages[name] = allocatedImage;
                } else {
                    throw std::runtime_error("Cannot determine dependency for image: " + name);
                }
            }
        }
    }

    void RenderGraph::createRHIPasses() {
        for (const auto& [passName, passNode] : m_passNodes) {
            RHI::Pass* rhiPass = m_resourceDevice->createResource<RHI::Pass>();

            const EnablePassClear& clearInfo = passNode.getClearInfo();
            if (clearInfo.types) {
                rhiPass->enableClear(clearInfo.types, clearInfo.color, clearInfo.depth, clearInfo.stencil);
            }

            const auto& targetEdges = passNode.getTargetOutgoingEdges();
            for (size_t i = 0; i < targetEdges.size(); ++i) {
                const auto& edge = targetEdges[i];
                auto imageNodeIt = m_imageNodes.find(edge->toImage);
                if (imageNodeIt != m_imageNodes.end()) {
                    Image* image = imageNodeIt->second->getImage();
                    if (image) {
                        rhiPass->bindTarget(static_cast<int>(i), image);
                    }
                }
            }

            const auto& depthStencilEdges = passNode.getDepthStencilOutgoingEdges();
            for (const auto& edge : depthStencilEdges) {
                auto imageNodeIt = m_imageNodes.find(edge->toImage);
                if (imageNodeIt != m_imageNodes.end()) {
                    Image* image = imageNodeIt->second->getImage();
                    if (image) {
                        rhiPass->depthStencil(image);
                    }
                }
            }

            m_allocatedPasses[passName] = rhiPass;
        }
    }
    void RenderGraph::analyzePassGroupDependencies(
    const std::string& groupLeader,
    const std::vector<std::string>& groupMembers,
    std::map<std::string, std::set<std::string>>& dependencies
) const {
        if (dependencies.find(groupLeader) == dependencies.end()) {
            dependencies[groupLeader] = std::set<std::string>();
        }

        for (const std::string& passName : groupMembers) {
            auto passNodeIt = m_passNodes.find(passName);
            if (passNodeIt == m_passNodes.end()) continue;

            const auto& passNode = passNodeIt->second;

            for (const auto* textureEdge : passNode.getTextureIncomingEdges()) {
                auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                if (imageNodeIt == m_imageNodes.end()) continue;

                const auto& imageNode = imageNodeIt->second;

                for (const auto* targetEdge : imageNode->getTargetIncomingEdges()) {
                    std::string producerPass = targetEdge->fromPass;

                    auto allGroups = m_passesUnions.getGroups();
                    for (const auto& [otherGroupLeader, otherGroupMembers] : allGroups) {
                        if (otherGroupLeader != groupLeader &&
                            std::find(otherGroupMembers.begin(), otherGroupMembers.end(), producerPass) != otherGroupMembers.end()) {
                            dependencies[groupLeader].insert(otherGroupLeader);
                            break;
                            }
                    }
                }

                for (const auto* depthEdge : imageNode->getDepthStencilIncomingEdges()) {
                    std::string producerPass = depthEdge->fromPass;

                    auto allGroups = m_passesUnions.getGroups();
                    for (const auto& [otherGroupLeader, otherGroupMembers] : allGroups) {
                        if (otherGroupLeader != groupLeader &&
                            std::find(otherGroupMembers.begin(), otherGroupMembers.end(), producerPass) != otherGroupMembers.end()) {
                            dependencies[groupLeader].insert(otherGroupLeader);
                            break;
                            }
                    }
                }
            }
        }
    }
    std::vector<std::string> RenderGraph::topologicalSortPasses() const
    {
        auto dependencies = buildPassDependencyGraph();
        return topologicalSort(dependencies);
    }

    std::map<std::string, std::set<std::string>> RenderGraph::buildPassDependencyGraph() const
    {
        std::map<std::string, std::set<std::string>> dependencies;

        for (const auto& [passName, passNode] : m_passNodes) {
            dependencies[passName] = std::set<std::string>();
        }

        for (const auto& [consumerPassName, consumerPassNode] : m_passNodes) {
            for (const auto* textureEdge : consumerPassNode.getTextureIncomingEdges()) {
                std::string imageName = textureEdge->fromImage;

                auto imageIt = m_imageNodes.find(imageName);
                if (imageIt != m_imageNodes.end()) {
                    const auto& imageNode = imageIt->second;

                    for (const auto* targetEdge : imageNode->getTargetIncomingEdges()) {
                        std::string producerPassName = targetEdge->fromPass;
                        if (producerPassName != consumerPassName) {
                            dependencies[consumerPassName].insert(producerPassName);
                        }
                    }

                    for (const auto* depthEdge : imageNode->getDepthStencilIncomingEdges()) {
                        std::string producerPassName = depthEdge->fromPass;
                        if (producerPassName != consumerPassName) {
                            dependencies[consumerPassName].insert(producerPassName);
                        }
                    }
                }
            }
        }

        return dependencies;
    }

    std::vector<std::string> RenderGraph::topologicalSort(
        const std::map<std::string, std::set<std::string>>& dependencies
    ) const {
        std::vector<std::string> result;
        std::map<std::string, int> inDegree;
        std::queue<std::string> queue;

        for (const auto& [node, deps] : dependencies) {
            inDegree[node] = 0;
        }

        for (const auto& [node, deps] : dependencies) {
            for (const std::string& dep : deps) {
                if (inDegree.find(dep) != inDegree.end()) {
                    inDegree[node]++;
                }
            }
        }

        for (const auto& [node, degree] : inDegree) {
            if (degree == 0) {
                queue.push(node);
            }
        }

        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();
            result.push_back(current);

            for (const auto& [node, deps] : dependencies) {
                if (deps.find(current) != deps.end()) {
                    inDegree[node]--;
                    if (inDegree[node] == 0) {
                        queue.push(node);
                    }
                }
            }
        }

        if (result.size() != dependencies.size()) {
            throw std::runtime_error("Circular dependency detected in PassGroups");
        }

        return result;
    }

    void RenderGraph::computePassGroupExecutionOrder() {
        auto passGroups = m_passesUnions.getGroups();
        std::map<std::string, std::set<std::string>> dependencies;

        for (const auto& [groupLeader, groupMembers] : passGroups) {
            analyzePassGroupDependencies(groupLeader, groupMembers, dependencies);
        }

        m_passGroupExecutionOrder = topologicalSort(dependencies);

        simulateExecutionAndAnalyzeBarriers();
    }


    void RenderGraph::createPassGroups() {
        auto passGroups = m_passesUnions.getGroups();

        for (const auto& [groupLeader, groupMembers] : passGroups) {
            RHI::PassGroup* passGroup = m_resourceDevice->createResource<RHI::PassGroup>();

            std::vector<std::string> orderedPassNames;

            for (const std::string& passName : groupMembers) {
                auto passIt = m_allocatedPasses.find(passName);
                if (passIt == m_allocatedPasses.end()) continue;

                RHI::Pass* currentPass = passIt->second;
                std::vector<RHI::PassNeighbors> predecessors;
                std::vector<RHI::PassNeighbors> successors;

                auto passNodeIt = m_passNodes.find(passName);
                if (passNodeIt != m_passNodes.end()) {
                    const auto& passNode = passNodeIt->second;

                    bool outputsToWindow = false;
                    //判断 target和depth是否窗口
                    for (const auto* targetEdge : passNode.getTargetOutgoingEdges()) {
                        auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                        if (imageNodeIt != m_imageNodes.end()) {
                            auto* windowTargetNode = dynamic_cast<RenderGraphWindowTargetNode*>(
                                imageNodeIt->second.get());
                            if (windowTargetNode && windowTargetNode->isValidWindowTarget()) {
                                outputsToWindow = true;
                                break;
                            }
                        }
                    }

                    if (!outputsToWindow) {
                        for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges()) {
                            auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
                            if (imageNodeIt != m_imageNodes.end()) {
                                auto* windowDepthNode = dynamic_cast<RenderGraphWindowDepthStencilNode*>(
                                    imageNodeIt->second.get());
                                if (windowDepthNode && windowDepthNode->isValidWindowDepthStencil()) {
                                    outputsToWindow = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (outputsToWindow) {
                        successors.emplace_back(
                            RHI::Pass::present
                        );
                    }

                    for (const auto* textureEdge : passNode.getTextureIncomingEdges()) {
                        auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                        if (imageNodeIt != m_imageNodes.end()) {
                            const auto& imageNode = imageNodeIt->second;

                            // 检查Target边
                            for (const auto* targetEdge : imageNode->getTargetIncomingEdges()) {
                                if (std::find(groupMembers.begin(), groupMembers.end(), targetEdge->fromPass) != groupMembers.end()) {
                                    auto predecessorPassIt = m_allocatedPasses.find(targetEdge->fromPass);
                                    if (predecessorPassIt != m_allocatedPasses.end()) {
                                        predecessors.emplace_back(
                                            predecessorPassIt->second,
                                            PipelineStage::colorAttachmentOutput,
                                            AccessFlag::colorAttachmentWrite,
                                            convertShaderStageToPipelineStage(textureEdge->stage),
                                            AccessFlag::shaderRead
                                        );
                                    }
                                }
                            }

                            // 检查DepthStencil边
                            for (const auto* depthEdge : imageNode->getDepthStencilIncomingEdges()) {
                                if (std::find(groupMembers.begin(), groupMembers.end(), depthEdge->fromPass) != groupMembers.end()) {
                                    auto predecessorPassIt = m_allocatedPasses.find(depthEdge->fromPass);
                                    if (predecessorPassIt != m_allocatedPasses.end()) {
                                        predecessors.emplace_back(
                                            predecessorPassIt->second,
                                            PipelineStage::lateFragmentTests,
                                            AccessFlag::depthStencilAttachmentWrite,
                                            convertShaderStageToPipelineStage(textureEdge->stage),
                                            AccessFlag::shaderRead
                                        );
                                    }
                                }
                            }
                        }
                    }
                }

                orderedPassNames.push_back(passName);

                passGroup->addPass({
                    currentPass,
                    predecessors,
                    successors
                });
            }

            m_passGroupOrders[groupLeader] = std::move(orderedPassNames);

            passGroup->create();

            m_allocatedPassGroups[groupLeader] = passGroup;
        }
    }
    void RenderGraph::submitPassGroup(RHI::CommandBuffer* cmdBuffer,
                                     const std::string& groupLeader) {
        auto passGroup = m_allocatedPassGroups.at(groupLeader);
        auto orderIt = m_passGroupOrders.find(groupLeader);
        if (orderIt == m_passGroupOrders.end()) {
            return;
        }

        const std::vector<std::string>& orderedPassNames = orderIt->second;

        bool isPureComputeGroup = true;
        for (const auto& passName : orderedPassNames) {
            auto descIt = m_passDescs.find(passName);
            if (descIt == m_passDescs.end() || (!descIt->second.computeDispatcher &&
                descIt->second.type != PassType::Compute &&
                descIt->second.type != PassType::ExternalInterop)) {
                isPureComputeGroup = false;
                break;
            }
        }

        if (!isPureComputeGroup) {
            passGroup->beginSubmit(cmdBuffer);
        }

        for (size_t i = 0; i < orderedPassNames.size(); ++i) {
            const std::string& passName = orderedPassNames[i];

            auto descIt = m_passDescs.find(passName);
            if (descIt != m_passDescs.end() && descIt->second.computeDispatcher) {
                class RenderGraphComputeContext : public ComputeExecutionContext {
                public:
                    RenderGraphComputeContext(RenderGraph* graph, RHI::CommandBuffer* cmdBuf, std::string name)
                        : m_graph(graph), m_cmdBuf(cmdBuf), m_name(std::move(name)) {}

                    Image* getImage(const std::string& name) override {
                        if (!m_graph) return nullptr;
                        return m_graph->getImage(name);
                    }

                    void* getExternalNativeHandle(const std::string& name) override {
                        if (!m_graph) return nullptr;
                        Image* img = m_graph->getImage(name);
                        if (!img) return nullptr;
                        return img->currentImage();
                    }

                    RHI::CommandBuffer* getCommandBuffer() override {
                        return m_cmdBuf;
                    }

                private:
                    RenderGraph* m_graph;
                    RHI::CommandBuffer* m_cmdBuf;
                    std::string m_name;
                };

                RenderGraphComputeContext execCtx(this, cmdBuffer, passName);
                descIt->second.computeDispatcher->dispatch(execCtx);
            }

            PassSubmitEvent env;
            env.passName = passName;
            env.cmdBuf = cmdBuffer;
            trigger(env);

            if (!isPureComputeGroup && i < orderedPassNames.size() - 1) {
                passGroup->nextPass(cmdBuffer);
            }
        }

        if (!isPureComputeGroup) {
            passGroup->endSubmit(cmdBuffer);
        }
    }
    void RenderGraph::simulateExecutionAndAnalyzeBarriers() {
        auto passGroups = m_passesUnions.getGroups();
        std::unordered_map<Image*, ImageState> imageStates;

        for (const auto& [imageName, imageNode] : m_imageNodes) {
            Image* image = imageNode->getImage();
            if (image) {
                imageStates[image] = ImageState{};
                if (dynamic_cast<RenderGraphWindowTargetNode*>(imageNode.get()) || dynamic_cast<RenderGraphWindowDepthStencilNode*>(imageNode.get()))
                {
                    imageStates[image].currentLayout = ImageLayout::undefined;
                }
            }
        }

        for (const std::string& groupLeader : m_passGroupExecutionOrder) {
            auto groupIt = passGroups.find(groupLeader);
            if (groupIt == passGroups.end()) continue;

            const auto& groupMembers = groupIt->second;

            std::vector<BarrierInfo> barriers = checkBarriersBeforePassGroup(groupLeader, groupMembers, imageStates);

            if (!barriers.empty()) {
                m_passGroupBarriers[groupLeader] = std::move(barriers);
            }

            simulatePassGroupExecution(groupLeader, groupMembers, imageStates);
        }
    }

    std::vector<RenderGraph::BarrierInfo> RenderGraph::checkBarriersBeforePassGroup(
        const std::string& groupLeader,
        const std::vector<std::string>& groupMembers,
        const std::unordered_map<Image*, ImageState>& imageStates) {

        std::vector<BarrierInfo> barriers;
        std::set<Image*> processedImages;

        for (const std::string& passName : groupMembers) {
            auto passIt = m_passNodes.find(passName);
            if (passIt == m_passNodes.end()) continue;

            const auto& passNode = passIt->second;

            for (const auto* textureEdge : passNode.getTextureIncomingEdges()) {
                auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                if (imageNodeIt == m_imageNodes.end()) continue;

                Image* image = imageNodeIt->second->getImage();
                if (!image || processedImages.count(image)) continue;

                auto stateIt = imageStates.find(image);
                if (stateIt == imageStates.end()) continue;

                const ImageState& currentState = stateIt->second;

                PipelineStage requiredStage = convertShaderStageToPipelineStage(textureEdge->stage);
                AccessFlag requiredAccess = AccessFlag::shaderRead;
                ImageLayout requiredLayout = ImageLayout::shaderReadOnlyOptimal;

                bool needsBarrier = false;

                if (currentState.currentLayout != requiredLayout) {
                    needsBarrier = true;
                }

                if (!currentState.lastWriterGroup.empty() &&
                    currentState.lastWriterGroup != groupLeader &&
                    (currentState.lastAccess & (AccessFlag::colorAttachmentWrite | AccessFlag::depthStencilAttachmentWrite))) {
                    needsBarrier = true;
                    }

                if (needsBarrier) {
                    BarrierInfo barrier;
                    barrier.image = image;
                    barrier.oldLayout = currentState.currentLayout;
                    barrier.newLayout = requiredLayout;
                    barrier.srcStage = currentState.lastStage;
                    barrier.dstStage = requiredStage;
                    barrier.srcAccess = currentState.lastAccess;
                    barrier.dstAccess = requiredAccess;

                    if (currentState.currentLayout == ImageLayout::depthAttachmentOptimal ||
                        currentState.currentLayout == ImageLayout::depthStencilAttachmentOptimal) {
                        barrier.aspect = ImageAspect::depth;
                        } else {
                            barrier.aspect = ImageAspect::color;
                        }

                    barriers.push_back(barrier);
                    processedImages.insert(image);
                }
            }
        }

        return barriers;
    }

    void RenderGraph::simulatePassGroupExecution(
        const std::string& groupLeader,
        const std::vector<std::string>& groupMembers,
        std::unordered_map<Image*, ImageState>& imageStates) {

        for (const std::string& passName : groupMembers) {
            auto passIt = m_passNodes.find(passName);
            if (passIt == m_passNodes.end()) continue;

            const auto& passNode = passIt->second;

            for (const auto* targetEdge : passNode.getTargetOutgoingEdges()) {
                auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                if (imageNodeIt == m_imageNodes.end()) continue;

                Image* image = imageNodeIt->second->getImage();
                if (!image) continue;

                ImageState& state = imageStates[image];
                state.currentLayout = ImageLayout::colorAttachmentOptimal;
                state.lastStage = PipelineStage::colorAttachmentOutput;
                state.lastAccess = AccessFlag::colorAttachmentWrite;
                state.lastWriterGroup = groupLeader;
            }

            for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges()) {
                auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
                if (imageNodeIt == m_imageNodes.end()) continue;

                Image* image = imageNodeIt->second->getImage();
                if (!image) continue;

                ImageState& state = imageStates[image];
                state.currentLayout = ImageLayout::depthAttachmentOptimal;
                state.lastStage = PipelineStage::lateFragmentTests;
                state.lastAccess = AccessFlag::depthStencilAttachmentWrite;
                state.lastWriterGroup = groupLeader;
            }

            for (const auto* textureEdge : passNode.getTextureIncomingEdges()) {
                auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                if (imageNodeIt == m_imageNodes.end()) continue;

                Image* image = imageNodeIt->second->getImage();
                if (!image) continue;

                ImageState& state = imageStates[image];
                if (state.currentLayout != ImageLayout::shaderReadOnlyOptimal) {
                    state.currentLayout = ImageLayout::shaderReadOnlyOptimal;
                    state.lastStage = convertShaderStageToPipelineStage(textureEdge->stage);
                    state.lastAccess = AccessFlag::shaderRead;
                }
            }
        }
    }

    void RenderGraph::cleanUpCompile()
    {
        m_commandBufferGraph->removeBuffer(m_commandBufferToken);
        for (auto& [name, pass] : m_allocatedPasses)
        {
            pipeHub.passPipe.remove<PassInfo>(name);
        }
        m_passGroupOrders.clear();
        m_passGroupExecutionOrder.clear();
        m_passGroupBarriers.clear();
        m_passGroupDependencies.clear();
        m_imageNodes.clear();
        m_passNodes.clear();
        m_passesUnions = UnionFind<std::string,char>();
        m_edges.clear();
        clearResources();//m_allocatedImages
        for (auto& [name, pass] : m_allocatedPasses)
        {
            pass->release();
        }
        m_allocatedPasses.clear();
        m_topologicalSortPasses.clear();
        for (auto& [name, passGroup] : m_allocatedPassGroups)
        {
            passGroup->release();
        }
    }

    PipelineStage RenderGraph::convertShaderStageToPipelineStage(ShaderStage stage) const {
        switch (stage) {
        case ShaderStage::Vertex:
            return PipelineStage::vertexShader;
        case ShaderStage::Fragment:
            return PipelineStage::fragmentShader;
        case ShaderStage::Compute:
            return PipelineStage::computeShader;
        default:
            return PipelineStage::fragmentShader;
        }
    }
    void RenderGraph::executeBarriers(RHI::CommandBuffer* cmdBuffer, const std::vector<BarrierInfo>& barriers) {
        for (const auto& barrier : barriers) {
            cmdBuffer->barrier(
                barrier.image,
                barrier.oldLayout,
                barrier.newLayout,
                barrier.srcStage,
                barrier.dstStage,
                barrier.srcAccess,
                barrier.dstAccess,
                barrier.aspect
            );
        }
    }
void RenderGraph::cullPasses()
    {
        std::vector<std::string> passesToCull;

        for (const auto& [passName, passNode] : m_passNodes) {
            bool shouldCull = false;

            bool hasTargetOutput = !passNode.getTargetOutgoingEdges().empty();
            bool hasDepthStencilOutput = !passNode.getDepthStencilOutgoingEdges().empty();
            
            if (!hasTargetOutput && !hasDepthStencilOutput) {
                shouldCull = true;
            } else {
                for (const auto* targetEdge : passNode.getTargetOutgoingEdges()) {
                    auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                    if (imageNodeIt != m_imageNodes.end()) {
                        const auto* imageNode = imageNodeIt->second.get();

                        const auto* bufferNode = dynamic_cast<const RenderGraphBufferNode*>(imageNode);
                        if (bufferNode && !bufferNode->isSizeDetermined()) {
                            shouldCull = true;
                            break;
                        }
                    }
                }

                if (!shouldCull) {
                    for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges()) {
                        auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
                        if (imageNodeIt != m_imageNodes.end()) {
                            const auto* imageNode = imageNodeIt->second.get();

                            const auto* bufferNode = dynamic_cast<const RenderGraphBufferNode*>(imageNode);
                            if (bufferNode && !bufferNode->isSizeDetermined()) {
                                shouldCull = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (shouldCull) {
                passesToCull.push_back(passName);
            }
        }

        for (const std::string& passName : passesToCull) {
            cullPass(passName);
        }
    }

void RenderGraph::cullPass(const std::string& passName)
    {
        auto passIt = m_passNodes.find(passName);
        if (passIt == m_passNodes.end()) {
            return;
        }

        const auto& passNode = passIt->second;
        std::vector<std::string> imagesToCull;

        std::vector<TextureEdge*> textureEdgesToRemove;
        std::vector<TargetEdge*> targetEdgesToRemove;
        std::vector<DepthStencilEdge*> depthStencilEdgesToRemove;

        for (const auto* textureEdge : passNode.getTextureIncomingEdges()) {
            textureEdgesToRemove.push_back(const_cast<TextureEdge*>(textureEdge));
        }

        for (const auto* targetEdge : passNode.getTargetOutgoingEdges()) {
            targetEdgesToRemove.push_back(const_cast<TargetEdge*>(targetEdge));

            auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
            if (imageNodeIt != m_imageNodes.end()) {
                const auto& imageNode = imageNodeIt->second;

                if (imageNode->getTargetIncomingEdges().size() == 1 &&
                    imageNode->getDepthStencilIncomingEdges().empty() &&
                    imageNode->getTextureOutgoingEdges().empty()) {
                    imagesToCull.push_back(targetEdge->toImage);
                    }
            }
        }

        for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges()) {
            depthStencilEdgesToRemove.push_back(const_cast<DepthStencilEdge*>(depthEdge));

            auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
            if (imageNodeIt != m_imageNodes.end()) {
                const auto& imageNode = imageNodeIt->second;

                if (imageNode->getDepthStencilIncomingEdges().size() == 1 &&
                    imageNode->getTargetIncomingEdges().empty() &&
                    imageNode->getTextureOutgoingEdges().empty()) {
                    imagesToCull.push_back(depthEdge->toImage);
                    }
            }
        }

        for (auto* edge : textureEdgesToRemove) {
            removeTextureEdge(edge);
        }

        for (auto* edge : targetEdgesToRemove) {
            removeTargetEdge(edge);
        }

        for (auto* edge : depthStencilEdgesToRemove) {
            removeDepthStencilEdge(edge);
        }

        for (const std::string& imageName : imagesToCull) {
            m_imageNodes.erase(imageName);
        }

        m_passNodes.erase(passName);

        auto it = std::find(m_topologicalSortPasses.begin(), m_topologicalSortPasses.end(), passName);
        if (it != m_topologicalSortPasses.end()) {
            m_topologicalSortPasses.erase(it);
        }
    }

    void RenderGraph::pushPipe()
    {
        for (auto [passName,pass] : m_allocatedPasses)
        {
            PassInfo info{
                *pass
            };
            pipeHub
                .passPipe
                .provide(passName,
                         PassInfo{
                             *pass,
                             [this, passName]() -> std::map<std::string, Image*>
                             {
                                 std::map<std::string, Image*> textureImages;
                                 auto passNodeIt = m_passNodes.find(passName);
                                 if (passNodeIt != m_passNodes.end())
                                 {
                                     const auto& passNode = passNodeIt->second;

                                     for (const auto* textureEdge : passNode.getTextureIncomingEdges())
                                     {
                                         auto imageNodeIt = m_imageNodes.find(textureEdge->fromImage);
                                         if (imageNodeIt != m_imageNodes.end())
                                         {
                                             Image* image = imageNodeIt->second->getImage();
                                             if (image)
                                             {
                                                 textureImages[textureEdge->fromImage] = image;
                                             }
                                         }
                                     }
                                 }
                                 return textureImages;
                             }.operator()(),
                             [this, passName]() -> std::map<std::string, ShaderStage>
                             {
                                 std::map<std::string, ShaderStage> textureStages;
                                 auto passNodeIt = m_passNodes.find(passName);
                                 if (passNodeIt != m_passNodes.end())
                                 {
                                     const auto& passNode = passNodeIt->second;

                                     for (const auto* textureEdge : passNode.getTextureIncomingEdges())
                                     {
                                         textureStages[textureEdge->fromImage] = textureEdge->stage;
                                     }
                                 }
                                 return textureStages;
                             }.operator()(),
                             [this, passName]() -> std::map<std::string, Image*>
                             {
                                 std::map<std::string, Image*> targetImages;
                                 auto passNodeIt = m_passNodes.find(passName);
                                 if (passNodeIt != m_passNodes.end())
                                 {
                                     const auto& passNode = passNodeIt->second;

                                     for (const auto* targetEdge : passNode.getTargetOutgoingEdges())
                                     {
                                         auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                                         if (imageNodeIt != m_imageNodes.end())
                                         {
                                             Image* image = imageNodeIt->second->getImage();
                                             if (image)
                                             {
                                                 targetImages[targetEdge->toImage] = image;
                                             }
                                         }
                                     }
                                 }
                                 return targetImages;
                             }.operator()(),
                             [this, passName]() -> std::map<std::string, Image*>
                             {
                                 std::map<std::string, Image*> depthStencilImages;
                                 auto passNodeIt = m_passNodes.find(passName);
                                 if (passNodeIt != m_passNodes.end())
                                 {
                                     const auto& passNode = passNodeIt->second;

                                     for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges())
                                     {
                                         auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
                                         if (imageNodeIt != m_imageNodes.end())
                                         {
                                             Image* image = imageNodeIt->second->getImage();
                                             if (image)
                                             {
                                                 depthStencilImages[depthEdge->toImage] = image;
                                             }
                                         }
                                     }
                                 }
                                 return depthStencilImages;
                             }.operator()(),
                             [this, passName]() -> OutputInfo
                             {
                                 OutputInfo outputInfo;
                                 auto passNodeIt = m_passNodes.find(passName);
                                 if (passNodeIt != m_passNodes.end())
                                 {
                                     const auto& passNode = passNodeIt->second;

                                     const SizeNode* rootNode = nullptr;

                                     for (const auto* targetEdge : passNode.getTargetOutgoingEdges())
                                     {
                                         auto imageNodeIt = m_imageNodes.find(targetEdge->toImage);
                                         if (imageNodeIt != m_imageNodes.end())
                                         {
                                             rootNode = imageNodeIt->second->getRoot();
                                             break;
                                         }
                                     }

                                     if (!rootNode)
                                     {
                                         for (const auto* depthEdge : passNode.getDepthStencilOutgoingEdges())
                                         {
                                             auto imageNodeIt = m_imageNodes.find(depthEdge->toImage);
                                             if (imageNodeIt != m_imageNodes.end())
                                             {
                                                 rootNode = imageNodeIt->second->getRoot();
                                                 break;
                                             }
                                         }
                                     }
                                     
                                     if (rootNode)
                                     {
                                         if (auto windowTarget = dynamic_cast<const RenderGraphWindowTargetNode*>(rootNode))
                                         {
                                             outputInfo.isWindow = true;
                                             outputInfo.window = windowTarget->getWindow();
                                             if (outputInfo.window)
                                             {
                                                 outputInfo.width = outputInfo.window->getWidth();
                                                 outputInfo.height = outputInfo.window->getHeight();
                                             }
                                         }
                                         else if (auto windowDepth = dynamic_cast<const RenderGraphWindowDepthStencilNode*>(rootNode))
                                         {
                                             outputInfo.isWindow = true;
                                             outputInfo.window = windowDepth->getWindow();
                                             if (outputInfo.window)
                                             {
                                                 outputInfo.width = outputInfo.window->getWidth();
                                                 outputInfo.height = outputInfo.window->getHeight();
                                             }
                                         }
                                         else
                                         {
                                             outputInfo.isWindow = false;
                                             outputInfo.window = nullptr;
                                             rootNode->getComputedSize(outputInfo.width, outputInfo.height);
                                         }
                                     }
                                 }
                                 return outputInfo;
                             }.operator()()
                         });
        }
    }

    void RenderGraph::executeAllPassGroups(RHI::CommandBuffer* cmdBuffer)
    {
        for (const std::string& groupLeader : m_passGroupExecutionOrder) {
            auto barrierIt = m_passGroupBarriers.find(groupLeader);
            if (barrierIt != m_passGroupBarriers.end()) {
                executeBarriers(cmdBuffer, barrierIt->second);
            }
            submitPassGroup(cmdBuffer, groupLeader);
        }
    }

    RHI::Pass* RenderGraph::getPass(const std::string& name) const
    {
        return m_allocatedPasses.at(name);
    }

    Image* RenderGraph::getImage(const std::string& name) const
    {
        auto imageNodeIt = m_imageNodes.find(name);
        if (imageNodeIt!= m_imageNodes.end()) {
            return imageNodeIt->second->getImage();
        }
        return nullptr;
    }

    std::vector<TextureEdge*> RenderGraph::getTextureEdges(const std::string& passName) const
    {
        auto passNodeIt = m_passNodes.find(passName);
        if (passNodeIt!= m_passNodes.end()) {
            return passNodeIt->second.getTextureIncomingEdges();
        }
        return {};
    }

}// FCT
#ifndef TOKENGRAPH_H
#define TOKENGRAPH_H

#include "../ThirdParty.h"
#include "Noncopyable.h"
#include "Containers.h"
#include "Status.h"

namespace FCT {

    template<typename Token, typename Value>
    class TokenGraph : public Noncopyable {
    public:
        class NodeProbe;

        struct NodeInfo {
            Value value{};
            Vector<Token> predecessors{};
            Vector<Token> successors{};

            NodeInfo() = default;

            NodeInfo(
                Value value,
                Vector<Token> predecessors = {},
                Vector<Token> successors = {})
                : value(std::move(value))
                , predecessors(std::move(predecessors))
                , successors(std::move(successors)) {}

            NodeInfo(
                Token token,
                Value value,
                Vector<Token> predecessors = {},
                Vector<Token> successors = {})
                : value(std::move(value))
                , predecessors(std::move(predecessors))
                , successors(std::move(successors))
                , token(std::move(token)) {}

        private:
            Token token{};
            friend class TokenGraph;
            friend class NodeProbe;
        };

        class NodeProbe {
        public:
            friend class TokenGraph;

            ~NodeProbe() {
                if (!m_modified) {
                    return;
                }
                m_nodeInfo.token = m_token;
                if (!m_exists) {
                    m_graph.addNode(std::move(m_nodeInfo));
                    return;
                }
                m_graph.updateNode(m_nodeInfo);
            }

            NodeProbe(const NodeProbe&) = delete;
            NodeProbe& operator=(const NodeProbe&) = delete;

            NodeProbe(NodeProbe&& other) noexcept
                : m_graph(other.m_graph)
                , m_token(std::move(other.m_token))
                , m_nodeInfo(std::move(other.m_nodeInfo))
                , m_modified(other.m_modified)
                , m_exists(other.m_exists) {
                other.m_modified = false;
            }

            NodeProbe& operator=(const NodeInfo& info) {
                m_nodeInfo = info;
                m_nodeInfo.token = m_token;
                m_modified = true;
                return *this;
            }

            NodeProbe& operator=(NodeInfo&& info) {
                m_nodeInfo = std::move(info);
                m_nodeInfo.token = m_token;
                m_modified = true;
                return *this;
            }

            NodeInfo* operator->() noexcept {
                m_modified = true;
                return &m_nodeInfo;
            }

            const NodeInfo* operator->() const noexcept {
                return &m_nodeInfo;
            }

            NodeInfo& operator*() noexcept {
                m_modified = true;
                return m_nodeInfo;
            }

            const NodeInfo& operator*() const noexcept {
                return m_nodeInfo;
            }

            [[nodiscard]] bool exists() const noexcept {
                return m_exists;
            }

            [[nodiscard]] const Token& token() const noexcept {
                return m_token;
            }

            NodeProbe& addPredecessor(const Token& predecessor) {
                if (Contains(m_nodeInfo.predecessors, predecessor)) {
                    return *this;
                }
                m_nodeInfo.predecessors.push_back(predecessor);
                m_modified = true;
                return *this;
            }

            NodeProbe& removePredecessor(const Token& predecessor) {
                if (!Erase(m_nodeInfo.predecessors, predecessor)) {
                    return *this;
                }
                m_modified = true;
                return *this;
            }

            NodeProbe& addSuccessor(const Token& successor) {
                if (Contains(m_nodeInfo.successors, successor)) {
                    return *this;
                }
                m_nodeInfo.successors.push_back(successor);
                m_modified = true;
                return *this;
            }

            NodeProbe& removeSuccessor(const Token& successor) {
                if (!Erase(m_nodeInfo.successors, successor)) {
                    return *this;
                }
                m_modified = true;
                return *this;
            }

        private:
            NodeProbe(TokenGraph& graph, Token token)
                : m_graph(graph)
                , m_token(std::move(token)) {
                auto* node = FindOrNull(m_graph.m_nodeMap, m_token);
                if (!node) {
                    return;
                }
                m_nodeInfo = *node;
                m_exists = true;
            }

            TokenGraph& m_graph;
            Token m_token;
            NodeInfo m_nodeInfo;
            bool m_modified = false;
            bool m_exists = false;
        };

        TokenGraph() = default;
        ~TokenGraph() = default;

        void addNode(NodeInfo info) {
            Token token = info.token;
            addVertex(token);
            auto [it, inserted] = m_nodeMap.insert_or_assign(token, std::move(info));

            BoostVertex vertex = m_tokenToVertex[token];
            m_graph[vertex].nodeInfo = &it->second;

            for (const auto& predecessor : it->second.predecessors) {
                addEdge(token, predecessor, false);
            }
            for (const auto& successor : it->second.successors) {
                addEdge(token, successor, true);
            }
        }

        void addNode(
            Token token,
            Value value,
            Vector<Token> predecessors = {},
            Vector<Token> successors = {}) {
            NodeInfo info(token, std::move(value), std::move(predecessors), std::move(successors));
            addNode(std::move(info));
        }

        void removeNode(const Token& token) {
            auto it = m_nodeMap.find(token);
            if (it == m_nodeMap.end()) {
                return;
            }

            for (const auto& predecessor : it->second.predecessors) {
                removeEdge(predecessor, token, false);
            }
            for (const auto& successor : it->second.successors) {
                removeEdge(token, successor, true);
            }

            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (vertex) {
                m_graph[*vertex].nodeInfo = nullptr;
            }

            m_nodeMap.erase(it);

            if (!isZeroRefVertex(token)) {
                return;
            }
            removeVertex(token);
        }

        void updateNode(const NodeInfo& info) {
            auto* existing = FindOrNull(m_nodeMap, info.token);
            if (!existing) {
                addNode(info);
                return;
            }

            if (existing->predecessors == info.predecessors && 
                existing->successors == info.successors) {
                existing->value = info.value;
                return;
            }

            removeNode(info.token);
            addNode(info);
        }

        [[nodiscard]] bool containsNode(const Token& token) const noexcept {
            return m_nodeMap.contains(token);
        }

        [[nodiscard]] bool hasNode(const Token& token) const noexcept {
            return m_nodeMap.contains(token);
        }

        [[nodiscard]] NodeProbe operator[](Token token) {
            return NodeProbe(*this, std::move(token));
        }

        Status update() {
            TopoResult topo = kahnTopologicalSort();
            m_order.clear();
            m_order.reserve(m_nodeMap.size());

            for (BoostVertex vertex : topo.sortedVertices) {
                if (m_graph[vertex].nodeInfo == nullptr) {
                    continue;
                }
                m_order.push_back(m_graph[vertex].nodeInfo->value);
            }

            if (topo.hasCycle) {
                return FailedPreconditionError("Cyclic dependency detected during topological sort in TokenGraph!");
            }

            return OkStatus();
        }

        [[nodiscard]] Vector<Value> order() const {
            return m_order;
        }

        StatusOr<Vector<Value>> computeOrder(std::function<bool(const Value&)> canVisit) {
            TopoResult topo = kahnTopologicalSort();
            if (topo.hasCycle) {
                return FailedPreconditionError("Cyclic dependency detected during computeOrder in TokenGraph!");
            }

            Vector<Value> result;
            result.reserve(m_nodeMap.size());

            for (BoostVertex vertex : topo.sortedVertices) {
                if (m_graph[vertex].nodeInfo == nullptr) {
                    continue;
                }
                if (!canVisit(m_graph[vertex].nodeInfo->value)) {
                    continue;
                }
                result.push_back(m_graph[vertex].nodeInfo->value);
            }
            return result;
        }

        [[nodiscard]] bool hasCycle() const {
            return kahnTopologicalSort().hasCycle;
        }

        template<typename Visitor>
        void visitBFS(Token startToken, Visitor visitor, bool includeStart = true) const {
            auto* startVertex = FindOrNull(m_tokenToVertex, startToken);
            if (!startVertex) {
                return;
            }

            UnorderedSet<BoostVertex> visited;
            Vector<BoostVertex> queue;
            queue.push_back(*startVertex);
            visited.insert(*startVertex);

            size_t head = 0;
            while (head < queue.size()) {
                BoostVertex current = queue[head++];
                bool shouldVisit = (includeStart || current != *startVertex) && (m_graph[current].nodeInfo != nullptr);
                if (shouldVisit) {
                    visitor(m_graph[current].nodeInfo->value);
                }

                auto [outIt, outEnd] = boost::out_edges(current, m_graph);
                for (; outIt != outEnd; ++outIt) {
                    BoostVertex nextVertex = boost::target(*outIt, m_graph);
                    if (!visited.insert(nextVertex).second) {
                        continue;
                    }
                    queue.push_back(nextVertex);
                }
            }
        }

        template<typename Visitor>
        void visitDFSFromRoots(Visitor visitor) const {
            Vector<BoostVertex> roots;
            for (auto [vIt, vEnd] = boost::vertices(m_graph); vIt != vEnd; ++vIt) {
                if (boost::in_degree(*vIt, m_graph) != 0) {
                    continue;
                }
                if (m_graph[*vIt].nodeInfo == nullptr) {
                    continue;
                }
                roots.push_back(*vIt);
            }

            UnorderedSet<BoostVertex> visited;
            for (BoostVertex root : roots) {
                dfsVisitInternal(root, visitor, visited);
            }
        }

        [[nodiscard]] Vector<Token> getActivePredecessors(const Token& token) const {
            Vector<Token> activePredecessors;
            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (!vertex) {
                return activePredecessors;
            }

            UnorderedSet<Token> visited;
            collectActivePredecessors(*vertex, activePredecessors, visited);
            return activePredecessors;
        }

        [[nodiscard]] Vector<Token> getActiveSuccessors(const Token& token) const {
            Vector<Token> activeSuccessors;
            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (!vertex) {
                return activeSuccessors;
            }

            UnorderedSet<Token> visited;
            collectActiveSuccessors(*vertex, activeSuccessors, visited);
            return activeSuccessors;
        }

    private:
        struct VertexData {
            int weakRef = 0;
            Token token{};
            NodeInfo* nodeInfo = nullptr;
        };

        struct TopoResult {
            Vector<BoostVertex> sortedVertices{};
            bool hasCycle = false;
        };

        using TokenGraphSavedBoostGraph =
            boost::adjacency_list<
                boost::listS,
                boost::listS,
                boost::bidirectionalS,
                VertexData,
                int
            >;

        using BoostVertex = typename TokenGraphSavedBoostGraph::vertex_descriptor;
        using BoostEdge = typename TokenGraphSavedBoostGraph::edge_descriptor;

        void addVertex(const Token& token, int weakRef = 0) {
            if (m_tokenToVertex.contains(token)) {
                return;
            }
            VertexData data;
            data.weakRef = weakRef;
            data.token = token;
            data.nodeInfo = nullptr;
            BoostVertex vertex = boost::add_vertex(data, m_graph);
            m_tokenToVertex.emplace(token, vertex);
        }

        void removeVertex(const Token& token) {
            auto it = m_tokenToVertex.find(token);
            if (it == m_tokenToVertex.end()) {
                return;
            }
            boost::remove_vertex(it->second, m_graph);
            m_tokenToVertex.erase(it);
        }

        [[nodiscard]] bool hasVertex(const Token& token) const noexcept {
            return m_tokenToVertex.contains(token);
        }

        void ensureWeakVertex(const Token& token) {
            if (!hasVertex(token)) {
                addVertex(token, 1);
                return;
            }
            addWeakRef(token);
        }

        void addWeakRef(const Token& token) {
            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (!vertex) {
                return;
            }
            ++m_graph[*vertex].weakRef;
        }

        void removeWeakRef(const Token& token) {
            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (!vertex) {
                return;
            }
            --m_graph[*vertex].weakRef;
        }

        void addWeakRef(const Token& from, const Token& to) {
            auto* fromVertex = FindOrNull(m_tokenToVertex, from);
            if (!fromVertex) {
                return;
            }
            auto* toVertex = FindOrNull(m_tokenToVertex, to);
            if (!toVertex) {
                return;
            }
            auto edgePair = boost::edge(*fromVertex, *toVertex, m_graph);
            if (!edgePair.second) {
                return;
            }
            ++m_graph[edgePair.first];
        }

        void removeWeakRef(const Token& from, const Token& to) {
            auto* fromVertex = FindOrNull(m_tokenToVertex, from);
            if (!fromVertex) {
                return;
            }
            auto* toVertex = FindOrNull(m_tokenToVertex, to);
            if (!toVertex) {
                return;
            }
            auto edgePair = boost::edge(*fromVertex, *toVertex, m_graph);
            if (!edgePair.second) {
                return;
            }
            --m_graph[edgePair.first];
            if (m_graph[edgePair.first] != 0) {
                return;
            }
            removeBoostEdge(from, to);
        }

        [[nodiscard]] bool hasEdge(const Token& from, const Token& to) const {
            auto* fromVertex = FindOrNull(m_tokenToVertex, from);
            if (!fromVertex) {
                return false;
            }
            auto* toVertex = FindOrNull(m_tokenToVertex, to);
            if (!toVertex) {
                return false;
            }
            return boost::edge(*fromVertex, *toVertex, m_graph).second;
        }

        void addBoostEdge(const Token& from, const Token& to) {
            boost::add_edge(m_tokenToVertex.at(from), m_tokenToVertex.at(to), 1, m_graph);
        }

        void removeBoostEdge(const Token& from, const Token& to) {
            auto* fromVertex = FindOrNull(m_tokenToVertex, from);
            if (!fromVertex) {
                return;
            }
            auto* toVertex = FindOrNull(m_tokenToVertex, to);
            if (!toVertex) {
                return;
            }
            auto edgePair = boost::edge(*fromVertex, *toVertex, m_graph);
            if (!edgePair.second) {
                return;
            }
            boost::remove_edge(edgePair.first, m_graph);
        }

        void addEdge(const Token& node, const Token& rhs, bool isSuccessor) {
            ensureWeakVertex(rhs);

            const Token& from = isSuccessor ? node : rhs;
            const Token& to = isSuccessor ? rhs : node;

            if (hasEdge(from, to)) {
                addWeakRef(from, to);
                return;
            }
            addBoostEdge(from, to);
        }

        void removeEdge(const Token& node, const Token& rhs, bool isSuccessor) {
            const Token& from = isSuccessor ? node : rhs;
            const Token& to = isSuccessor ? rhs : node;

            if (hasEdge(from, to)) {
                removeWeakRef(from, to);
            }
            removeWeakRef(rhs);

            if (!isZeroRefVertex(rhs) || hasNode(rhs)) {
                return;
            }
            removeVertex(rhs);
        }

        [[nodiscard]] bool isZeroRefVertex(const Token& token) const {
            auto* vertex = FindOrNull(m_tokenToVertex, token);
            if (!vertex) {
                return false;
            }
            return m_graph[*vertex].weakRef == 0;
        }

        TopoResult kahnTopologicalSort() const {
            TopoResult result;
            size_t totalVertices = 0;

            UnorderedMap<BoostVertex, size_t> inDegrees;

            for (auto [vIt, vEnd] = boost::vertices(m_graph); vIt != vEnd; ++vIt) {
                ++totalVertices;
                size_t inDegree = boost::in_degree(*vIt, m_graph);
                inDegrees[*vIt] = inDegree;
                if (inDegree == 0) {
                    result.sortedVertices.push_back(*vIt);
                }
            }

            size_t head = 0;
            while (head < result.sortedVertices.size()) {
                BoostVertex current = result.sortedVertices[head++];
                auto [outIt, outEnd] = boost::out_edges(current, m_graph);
                for (; outIt != outEnd; ++outIt) {
                    BoostVertex target = boost::target(*outIt, m_graph);
                    auto it = inDegrees.find(target);
                    if (it == inDegrees.end() || it->second == 0) {
                        continue;
                    }
                    --it->second;
                    if (it->second != 0) {
                        continue;
                    }
                    result.sortedVertices.push_back(target);
                }
            }

            if (result.sortedVertices.size() < totalVertices) {
                result.hasCycle = true;
                for (auto [vIt, vEnd] = boost::vertices(m_graph); vIt != vEnd; ++vIt) {
                    auto it = inDegrees.find(*vIt);
                    if (it == inDegrees.end() || it->second == 0) {
                        continue;
                    }
                    result.sortedVertices.push_back(*vIt);
                }
            }

            return result;
        }

        template<typename Visitor>
        void dfsVisitInternal(
            BoostVertex vertex,
            Visitor& visitor,
            UnorderedSet<BoostVertex>& visited) const {
            if (!visited.insert(vertex).second) {
                return;
            }
            if (m_graph[vertex].nodeInfo != nullptr) {
                visitor(m_graph[vertex].nodeInfo->value);
            }
            auto [outIt, outEnd] = boost::out_edges(vertex, m_graph);
            for (; outIt != outEnd; ++outIt) {
                dfsVisitInternal(boost::target(*outIt, m_graph), visitor, visited);
            }
        }

        void collectActivePredecessors(
            BoostVertex vertex,
            Vector<Token>& result,
            UnorderedSet<Token>& visited) const {
            auto [inEdgeIt, inEdgeEnd] = boost::in_edges(vertex, m_graph);
            for (; inEdgeIt != inEdgeEnd; ++inEdgeIt) {
                BoostVertex source = boost::source(*inEdgeIt, m_graph);
                const Token& sourceToken = m_graph[source].token;
                if (!visited.insert(sourceToken).second) {
                    continue;
                }
                if (m_graph[source].nodeInfo != nullptr) {
                    result.push_back(sourceToken);
                    continue;
                }
                collectActivePredecessors(source, result, visited);
            }
        }

        void collectActiveSuccessors(
            BoostVertex vertex,
            Vector<Token>& result,
            UnorderedSet<Token>& visited) const {
            auto [outEdgeIt, outEdgeEnd] = boost::out_edges(vertex, m_graph);
            for (; outEdgeIt != outEdgeEnd; ++outEdgeIt) {
                BoostVertex target = boost::target(*outEdgeIt, m_graph);
                const Token& targetToken = m_graph[target].token;
                if (!visited.insert(targetToken).second) {
                    continue;
                }
                if (m_graph[target].nodeInfo != nullptr) {
                    result.push_back(targetToken);
                    continue;
                }
                collectActiveSuccessors(target, result, visited);
            }
        }

        TokenGraphSavedBoostGraph m_graph;
        UnorderedMap<Token, BoostVertex> m_tokenToVertex;
        UnorderedMap<Token, NodeInfo> m_nodeMap;
        Vector<Value> m_order;
    };

} // namespace FCT

#endif // TOKENGRAPH_H

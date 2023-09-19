#pragma once

#include <memory>
#include <set>
#include <vector>
#include <VirtualGraph/Editor/Node.hpp>

namespace Nebula::RenderGraph::Algorithm
{
    class Bfs
    {
    private:
        const std::vector<std::shared_ptr<Editor::Node>>& m_nodes;

    public:
        Bfs(const std::vector<std::shared_ptr<Editor::Node>>& nodes): m_nodes(nodes) {}

        /**
         * @return Set of node IDs which were visited during execution.
         */
        std::set<int32_t> execute(const std::shared_ptr<Editor::Node>& root);
    };
}
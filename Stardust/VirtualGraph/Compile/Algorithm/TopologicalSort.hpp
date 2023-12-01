#pragma once

#include <memory>
#include <vector>
#include <VirtualGraph/Editor/Node.hpp>

namespace Nebula::RenderGraph::Algorithm
{
    class Node;

    class TopologicalSort
    {
    public:
        explicit TopologicalSort(const std::vector<std::shared_ptr<Editor::Node>>& nodes): m_nodes(nodes) {}

        std::vector<std::shared_ptr<Editor::Node>> execute() const;

    private:
        const std::vector<std::shared_ptr<Editor::Node>>& m_nodes;
    };
}
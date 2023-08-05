#pragma once

#include <memory>
#include <vector>

namespace Nebula::Editor
{
    class Node;

    class TopologicalSort
    {
    private:
        using node_ptr = std::shared_ptr<Node>;
        const std::vector<node_ptr>& m_nodes;

    public:
        TopologicalSort(const std::vector<node_ptr>& nodes): m_nodes(nodes) {}

        std::vector<node_ptr> execute();
    };
}
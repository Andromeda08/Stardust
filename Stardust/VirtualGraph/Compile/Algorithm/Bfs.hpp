#pragma once

#include <memory>
#include <set>
#include <vector>
#include <VirtualGraph/Node.hpp>

namespace Nebula::Editor
{
    class Bfs
    {
    private:
        using node_ptr = std::shared_ptr<Node>;
        const std::vector<node_ptr>& m_nodes;

    public:
        Bfs(const std::vector<node_ptr>& nodes): m_nodes(nodes) {}

        /**
         * @return Set of node IDs which were visited during execution.
         */
        std::set<int32_t> execute(const node_ptr& root);
    };
}
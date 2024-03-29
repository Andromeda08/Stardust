#include "TopologicalSort.hpp"

#include <map>
#include <stdexcept>
#include <queue>

namespace Nebula::RenderGraph::Algorithm
{
    std::vector<std::shared_ptr<Editor::Node>> TopologicalSort::execute() const
    {
        std::map<int32_t, int32_t> in_degrees;
        for (const auto& node : m_nodes)
        {
            in_degrees.emplace(node->id(), node->in_degree());
        }

        std::vector<std::shared_ptr<Editor::Node>> T;
        std::queue<std::shared_ptr<Editor::Node>>  Q;

        for (const auto& node : m_nodes)
        {
            if (in_degrees[node->id()] == 0)
            {
                Q.push(node);
            }
        }

        while (Q.size() > 0)
        {
            auto v = Q.front();
            Q.pop();
            T.push_back(v);

            for (const auto& w : v->get_outgoing_edges())
            {
                auto w_id = w->id();

                in_degrees[w_id]--;
                if (in_degrees[w_id] == 0)
                {
                    auto i = std::ranges::find_if(m_nodes, [w_id](const auto& it){ return it->id() == w_id; });
                    Q.push(*i);
                }
            }
        }

        for (const auto& node : m_nodes)
        {
            if (in_degrees[node->id()] != 0)
            {
                throw std::runtime_error("[Error] Given graph was not acyclic.");
            }
        }

        return T;
    }
}
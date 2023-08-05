#include "TopologicalSort.hpp"

#include <map>
#include <stdexcept>
#include <queue>
#include <VirtualGraph/Node.hpp>

namespace Nebula::Editor
{
    std::vector<TopologicalSort::node_ptr> TopologicalSort::execute()
    {
        std::map<int32_t, int32_t> in_degrees;
        for (const auto& node : m_nodes)
        {
            in_degrees.emplace(node->id(), node->in_degree());
        }

        std::vector<node_ptr> T;
        std::queue<node_ptr>  Q;

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
                    auto i = std::find_if(m_nodes.begin(), m_nodes.end(), [w_id](const auto& i){ return i->id() == w_id; });
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
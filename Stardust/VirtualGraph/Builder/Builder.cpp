#include "Builder.h"
#include <stdexcept>
#include <VirtualGraph/Compile/DefaultCompileStrategy.hpp>

namespace Nebula::RenderGraph
{
    Builder::Builder(const std::shared_ptr<RenderGraphContext>& rgctx)
    : m_ctx(rgctx)
    {
    }

    std::shared_ptr<Editor::Node>& Builder::add_pass(NodeType pass_type)
    {
        auto node = Editor::Node::Factory::create(pass_type);
        m_nodes.insert({ node->id(), std::shared_ptr<Editor::Node>(node) });
        return m_nodes[node->id()];
    }

    Builder& Builder::make_connection(const node_ptr& start_node, const node_ptr& end_node,
                                      const std::string& start_resource, const std::string& end_resource)
    {
        auto& s_node = m_nodes[start_node->id()];
        auto& e_node = m_nodes[end_node->id()];
        auto& s_attr = s_node->get_resource(start_resource);
        auto& e_attr = e_node->get_resource(end_resource);

        Editor::Node::make_directed_edge(s_node, e_node);
        m_edges.emplace_back(*s_node, s_attr, *e_node, e_attr, s_attr.type);
        e_attr.input_is_connected = true;

        return *this;
    }

    Builder& Builder::make_connection(const Builder::node_ptr& start_node, const Builder::node_ptr& end_node,
                                      const std::string& resource_name)
    {
        return make_connection(start_node, end_node, resource_name, resource_name);
    }

    Compiler::CompileResult Builder::compile()
    {
        auto compiler = std::make_shared<Compiler::DefaultCompileStrategy>(*m_ctx);

        std::vector<node_ptr> nodes_vector;
        for (const auto& [k, v] : m_nodes)
        {
            nodes_vector.push_back(v);
        }

        return compiler->compile(nodes_vector, m_edges, true);
    }


}
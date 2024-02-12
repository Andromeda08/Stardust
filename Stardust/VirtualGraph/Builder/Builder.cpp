#include "Builder.h"

#include <VirtualGraph/Common/NodeFactory.hpp>
#include <VirtualGraph/Compile/DefaultCompileStrategy.hpp>
#include <VirtualGraph/Compile/OptimizedCompileStrategy.hpp>

namespace Nebula::RenderGraph
{
    Builder::Builder(const std::shared_ptr<RenderGraphContext>& rgctx)
    : m_ctx(rgctx)
    {
    }

    std::shared_ptr<Editor::Node>& Builder::add_pass(NodeType pass_type)
    {
        auto node = NodeFactory::create_editor(pass_type);
        m_nodes.insert({ node->id(), node });
        return m_nodes[node->id()];
    }

    Builder& Builder::make_connection(const node_ptr& start_node, const node_ptr& end_node,
                                      const std::string& start_resource, const std::string& end_resource)
    {
        const auto& s_node = m_nodes[start_node->id()];
        const auto& e_node = m_nodes[end_node->id()];
        auto& s_attr = s_node->get_resource(start_resource);
        auto& e_attr = e_node->get_resource(end_resource);

        Editor::Node::make_directed_edge(s_node, e_node);
        m_edges.emplace_back(*s_node, s_attr, *e_node, e_attr, s_attr.type);
        e_attr.input_is_connected = true;

        return *this;
    }

    Builder& Builder::make_connection(const node_ptr& start_node, const node_ptr& end_node,
                                      const std::string& resource_name)
    {
        return make_connection(start_node, end_node, resource_name, resource_name);
    }

    Compiler::CompileResult Builder::compile(const Compiler::CompilerType mode)
    {
        std::shared_ptr<Compiler::GraphCompileStrategy> compiler;
        if (mode == Compiler::CompilerType::eNaive)
        {
            compiler = std::make_shared<Compiler::DefaultCompileStrategy>(*m_ctx);
        }
        if (mode == Compiler::CompilerType::eResourceOptimized)
        {
            compiler = std::make_shared<Compiler::OptimizedCompileStrategy>(*m_ctx);
        }

        std::vector<node_ptr> nodes_vector;
        for (const auto& [k, v] : m_nodes)
        {
            nodes_vector.push_back(v);
        }

        return compiler->compile(nodes_vector, m_edges, true);
    }

    Compiler::CompileResult Builder::create_initial_graph(const std::shared_ptr<RenderGraphContext>& rgctx)
    {
        Builder builder(rgctx);

        const auto pass_a = builder.add_pass(NodeType::eHairRenderer);
        const auto pass_c = builder.add_pass(NodeType::eSceneProvider);
        const auto pass_d = builder.add_pass(NodeType::ePresent);

        auto compile_result = builder
            .make_connection(pass_c, pass_a, "Scene Data")
            .make_connection(pass_a, pass_d, "Output Image", "Final Image")
            .compile(Compiler::CompilerType::eResourceOptimized);

        return compile_result;
    }
}

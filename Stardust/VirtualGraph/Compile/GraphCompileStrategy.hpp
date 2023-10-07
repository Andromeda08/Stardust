#pragma once

#include <map>
#include <memory>
#include <string>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Compile/CompileResult.hpp>
#include <VirtualGraph/Compile/NodeFactory.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node;
    struct Edge;
}

namespace Nebula::RenderGraph::Compiler
{
    class GraphCompileStrategy
    {
    public:
        explicit GraphCompileStrategy(const RenderGraphContext& context)
        : m_context(context)
        {
            m_node_factory = std::make_unique<NodeFactory>(context);
        }

        virtual CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>&,
                                      const std::vector<Editor::Edge>& edges,
                                      bool verbose) = 0;

        virtual ~GraphCompileStrategy() = default;

    protected:
        void write_logs_to_file(const std::string& file_name);

        static void write_graph_state_dump(const RenderPath& render_path, const std::string& file_name);

    protected:
        std::vector<std::string> logs;

        std::unique_ptr<NodeFactory> m_node_factory;

        const RenderGraphContext& m_context;
    };
}
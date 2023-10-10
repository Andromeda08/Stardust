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
        explicit GraphCompileStrategy(const RenderGraphContext& context);

        virtual CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>&,
                                      const std::vector<Editor::Edge>& edges,
                                      bool verbose) = 0;

        virtual ~GraphCompileStrategy() = default;

    protected:
        void write_logs_to_file(const std::string& file_name);

        CompileResult make_failed_result(const std::string& message);

        static void write_graph_state_dump(const RenderPath& render_path, const std::string& file_name);

        static std::vector<std::shared_ptr<Editor::Node>>
        filter_unreachable_nodes(const std::vector<std::shared_ptr<Editor::Node>>& nodes);

        static std::vector<std::shared_ptr<Editor::Node>>
        get_execution_order(const std::vector<std::shared_ptr<Editor::Node>>& nodes);

    protected:
        std::vector<std::string> logs;

        std::unique_ptr<NodeFactory> m_node_factory;

        const RenderGraphContext& m_context;
    };
}
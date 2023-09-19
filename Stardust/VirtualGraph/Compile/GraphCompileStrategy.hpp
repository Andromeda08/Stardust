#pragma once

#include <map>
#include <memory>
#include <string>
#include <VirtualGraph/Common/GraphContext.hpp>
#include "CompileResult.hpp"

namespace Nebula::Editor
{
    class Node;

    class GraphCompileStrategy
    {
    public:
        explicit GraphCompileStrategy(const RenderGraphContext& context): m_context(context) {}

        virtual CompileResult compile(const std::vector<std::shared_ptr<Node>>&, bool verbose) = 0;

        virtual ~GraphCompileStrategy() = default;

    protected:
        void write_logs_to_file(const std::string& file_name);

        static void write_graph_state_dump(const RenderGraph::RenderPath& render_path, const std::string& file_name);

        std::vector<std::string> logs;

        const RenderGraphContext& m_context;
    };
}
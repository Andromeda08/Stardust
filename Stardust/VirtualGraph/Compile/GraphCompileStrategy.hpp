#pragma once

#include <map>
#include <memory>
#include <string>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Compile/CompileResult.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node;
}

namespace Nebula::RenderGraph::Compiler
{
    class GraphCompileStrategy
    {
    public:
        explicit GraphCompileStrategy(const RenderGraphContext& context): m_context(context) {}

        virtual CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>&, bool verbose) = 0;

        virtual ~GraphCompileStrategy() = default;

    protected:
        void write_logs_to_file(const std::string& file_name);

        static void write_graph_state_dump(const RenderPath& render_path, const std::string& file_name);

        std::vector<std::string> logs;

        const RenderGraphContext& m_context;
    };
}
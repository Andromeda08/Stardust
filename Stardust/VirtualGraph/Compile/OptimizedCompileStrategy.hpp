#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <string>
#include <vector>
#include <Vulkan/Context.hpp>
#include <VirtualGraph/Compile/CompileResult.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node;
    struct Edge;
}

namespace Nebula::RenderGraph::Compiler
{
    class OptimizedCompileStrategy : public GraphCompileStrategy
    {
    public:
        explicit OptimizedCompileStrategy(const RenderGraphContext& context): GraphCompileStrategy(context) {}

        CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>& nodes,
                              const std::vector<Editor::Edge>& edges,
                              bool verbose) override;

    private:
        std::vector<std::shared_ptr<Editor::Node>>
        filter_unreachable_nodes(const std::vector<std::shared_ptr<Editor::Node>>& nodes);

        std::vector<std::shared_ptr<Editor::Node>>
        get_execution_order(const std::vector<std::shared_ptr<Editor::Node>>& nodes);
    };
}
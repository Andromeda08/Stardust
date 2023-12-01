#pragma once

#include <memory>
#include <vector>
#include <VirtualGraph/Compile/CompileResult.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node;
    struct Edge;
}

namespace Nebula::RenderGraph::Compiler
{
    class DefaultCompileStrategy final : public GraphCompileStrategy
    {
    public:
        explicit DefaultCompileStrategy(const RenderGraphContext& context): GraphCompileStrategy(context) {}

        CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>& nodes,
                              const std::vector<Editor::Edge>& edges,
                              bool verbose) override;

        ~DefaultCompileStrategy() override = default;
    };
}
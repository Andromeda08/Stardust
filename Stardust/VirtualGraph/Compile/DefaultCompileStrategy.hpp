#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <Vulkan/Context.hpp>
#include <VirtualGraph/Compile/CompileResult.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node;
}

namespace Nebula::RenderGraph::Compiler
{
    class DefaultCompileStrategy : public GraphCompileStrategy
    {
    public:
        DefaultCompileStrategy(const RenderGraphContext& context): GraphCompileStrategy(context) {}

        CompileResult compile(const std::vector<std::shared_ptr<Editor::Node>>& nodes, bool verbose) override;

        ~DefaultCompileStrategy() = default;
    };
}
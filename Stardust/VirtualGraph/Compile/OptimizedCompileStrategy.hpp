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
#include <VirtualGraph/Compile/Algorithm/ResourceOptimizer.hpp>
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
        static void write_optimization_results(const Algorithm::ResourceOptimizationResult& optres, const std::string& file_name);
    };
}
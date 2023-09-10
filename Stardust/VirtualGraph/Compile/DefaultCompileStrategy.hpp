#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <Vulkan/Context.hpp>
#include "CompileResult.hpp"
#include "GraphCompileStrategy.hpp"

namespace Nebula::Editor
{
    class Node;

    class DefaultCompileStrategy : public GraphCompileStrategy
    {
    public:
        DefaultCompileStrategy(const sdvk::Context& context): GraphCompileStrategy(context) {}

        CompileResult compile(const std::vector<std::shared_ptr<Node>>& nodes, bool verbose = false) override;

        ~DefaultCompileStrategy() = default;
    };
}
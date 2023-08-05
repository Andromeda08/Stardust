#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "CompileResult.hpp"
#include "GraphCompileStrategy.hpp"

namespace Nebula::Editor
{
    class Node;

    class DefaultCompileStrategy : public GraphCompileStrategy
    {
    private:
        std::vector<std::string> logs;

    public:
        DefaultCompileStrategy() = default;

        CompileResult compile(const std::vector<std::shared_ptr<Node>>& nodes) override;

        ~DefaultCompileStrategy() = default;
    };
}
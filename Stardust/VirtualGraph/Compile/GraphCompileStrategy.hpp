#pragma once

#include <map>
#include <memory>
#include "CompileResult.hpp"

namespace Nebula::Editor
{
    class Node;

    class GraphCompileStrategy
    {
    public:
        GraphCompileStrategy() = default;

        virtual CompileResult compile(const std::vector<std::shared_ptr<Node>>&) = 0;

        virtual ~GraphCompileStrategy() = default;
    };
}
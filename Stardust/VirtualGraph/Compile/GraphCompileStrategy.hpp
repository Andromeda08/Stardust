#pragma once

#include <map>
#include <memory>
#include <string>
#include <Vulkan/Context.hpp>
#include "CompileResult.hpp"

namespace Nebula::Editor
{
    class Node;

    class GraphCompileStrategy
    {
    public:
        GraphCompileStrategy(const sdvk::Context& context): m_context(context) {}

        virtual CompileResult compile(const std::vector<std::shared_ptr<Node>>&, bool verbose = false) = 0;

        virtual ~GraphCompileStrategy() = default;

    protected:
        void write_logs_to_file(const std::string& file_name);

        std::vector<std::string> logs;

        const sdvk::Context& m_context;
    };
}
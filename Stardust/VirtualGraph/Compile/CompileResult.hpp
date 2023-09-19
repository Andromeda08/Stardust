#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <VirtualGraph/Common/RenderPath.hpp>

namespace Nebula::RenderGraph::Compiler
{
    struct CompileResult
    {
        std::vector<std::string> logs;
        std::string failure_message;
        bool success = false;
        std::chrono::milliseconds compile_time;

        std::shared_ptr<RenderPath> render_path;
    };
}
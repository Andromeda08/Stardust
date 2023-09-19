#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "RenderPath.hpp"

namespace Nebula::Editor
{
    struct CompileResult
    {
        std::vector<std::string> logs;
        std::string failure_message;
        bool success = false;
        std::chrono::milliseconds compile_time;

        std::shared_ptr<RenderGraph::RenderPath> render_path;
    };
}
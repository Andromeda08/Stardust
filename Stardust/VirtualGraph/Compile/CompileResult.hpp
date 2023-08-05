#pragma once

#include <string>
#include <vector>

namespace Nebula::Editor
{
    struct CompileResult
    {
        std::vector<std::string> logs;
        std::string failure_message;
        bool success = false;
    };
}
#pragma once

#include <string>
#include <Window/WindowOptions.hpp>

namespace sd
{
    enum RenderAPI
    {
        eVulkan,
    };

    struct ApplicationOptions
    {
        std::string name = "Application";
        RenderAPI render_api { eVulkan };
        WindowOptions window_options {};
    };
}
#pragma once

#include <string>

namespace Nebula::RenderGraph
{
    enum class ResourceRole
    {
        eInput,
        eOutput,
        eUnknown,
    };

    std::string get_resource_role_str(ResourceRole role);
}
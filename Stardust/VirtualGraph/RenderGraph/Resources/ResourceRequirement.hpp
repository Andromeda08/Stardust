#pragma once

#include <string>
#include "ResourceRole.hpp"
#include "ResourceType.hpp"

namespace Nebula::RenderGraph
{
    struct ResourceRequirement
    {
        std::string  name { "Unknown Resource" };
        ResourceRole role { ResourceRole::eUnknown };
        ResourceType type { ResourceType::eUnknown };
    };
}
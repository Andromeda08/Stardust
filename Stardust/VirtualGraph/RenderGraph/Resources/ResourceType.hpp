#pragma once

#include <glm/fwd.hpp>

namespace Nebula::RenderGraph
{
    enum class ResourceType
    {
        eBuffer,
        eCamera,
        eDepthImage,
        eImage,
        eImageArray,
        eObjects,
        eTlas,
        eUnknown,
    };

    glm::ivec4 get_resource_type_color(ResourceType type);
}
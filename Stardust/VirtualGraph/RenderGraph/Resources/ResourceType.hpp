#pragma once

#include <ostream>
#include <string>
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

    std::string get_resource_type_str(ResourceType type);

    bool is_gpu_resource(ResourceType type);

    std::ostream& operator<<(std::ostream& os, const ResourceType& type);
}
#include "ResourceType.hpp"

#include <set>
#include <glm/vec4.hpp>

namespace Nebula::RenderGraph
{
    glm::ivec4 get_resource_type_color(ResourceType type)
    {
        switch (type)
        {
            case ResourceType::eBuffer:
                return { 210, 15, 57, 255 };
            case ResourceType::eCamera:
                return { 234, 118, 203, 255 };
            case ResourceType::eDepthImage:
                return { 210, 15, 57, 255 };
            case ResourceType::eImage:
                return { 30, 102, 245, 255 };
            case ResourceType::eImageArray:
                return { 136, 57, 239, 255 };
            case ResourceType::eObjects:
                return { 223, 142, 29, 255 };
            case ResourceType::eTlas:
                return { 64, 160, 43, 255 };
            case ResourceType::eUnknown:
            default:
                return { 128, 128, 128, 255 };
        }
    }
}
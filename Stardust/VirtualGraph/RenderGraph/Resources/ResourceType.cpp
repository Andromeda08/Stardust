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
                return { 251, 191, 36, 255 };
            case ResourceType::eCamera:
                return { 236, 72, 153, 255 };
            case ResourceType::eDepthImage:
                return { 20, 184, 166, 255 };
            case ResourceType::eImage:
                return { 59, 130, 246, 255 };
            case ResourceType::eImageArray:
                return { 14, 165, 233, 255 };
            case ResourceType::eObjects:
                return { 244, 63, 94, 255 };
            case ResourceType::eTlas:
                return { 217, 70, 239, 255 };
            case ResourceType::eUnknown:
            default:
                return { 128, 128, 128, 255 };
        }
    }

    std::string get_resource_type_str(ResourceType type)
    {
        switch (type)
        {
            case RenderGraph::ResourceType::eBuffer:
                return "Buffer";
            case RenderGraph::ResourceType::eCamera:
                return "Camera";
            case RenderGraph::ResourceType::eDepthImage:
                return "Depth Image";
            case RenderGraph::ResourceType::eImage:
                return "Image";
            case RenderGraph::ResourceType::eImageArray:
                return "Image Array";
            case RenderGraph::ResourceType::eObjects:
                return "Objects";
            case RenderGraph::ResourceType::eTlas:
                return "Tlas";
            case RenderGraph::ResourceType::eUnknown:
                // Falls through
            default:
                return "Unknown";
        }
    }

    bool is_gpu_resource(ResourceType type)
    {
        static std::set<ResourceType> non_gpu_types = {
            ResourceType::eCamera, ResourceType::eObjects, ResourceType::eUnknown
        };

        return !non_gpu_types.contains(type);
    }

    std::ostream& operator<<(std::ostream& os, const ResourceType& type)
    {
        os << get_resource_type_str(type);
        return os;
    }
}
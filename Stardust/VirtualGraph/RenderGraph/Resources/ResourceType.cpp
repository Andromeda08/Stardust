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
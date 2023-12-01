 #pragma once

#include <ostream>
#include <string>
#include <glm/vec4.hpp>

namespace Nebula::RenderGraph
{
    enum class ResourceType
    {
        eBuffer,
        eCamera,
        eDepthImage,
        eImage,
        eObjects,
        eScene,
        eTlas,
        eUnknown,
    };

    inline glm::ivec4 get_resource_type_color(const ResourceType type)
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
            case ResourceType::eObjects:
                return { 244, 63, 94, 255 };
            case ResourceType::eScene:
                return { 14, 165, 233, 255 };
            case ResourceType::eTlas:
                return { 217, 70, 239, 255 };
            case ResourceType::eUnknown:
                // Falls through
                    default:
                        return { 128, 128, 128, 255 };
        }
    }

    inline std::string get_resource_type_str(const ResourceType type)
    {
        switch (type)
        {
            case ResourceType::eBuffer:
                return "Buffer";
            case ResourceType::eCamera:
                return "Camera";
            case ResourceType::eDepthImage:
                return "Depth Image";
            case ResourceType::eImage:
                return "Image";
            case ResourceType::eObjects:
                return "Objects";
            case ResourceType::eScene:
                return "Scene";
            case ResourceType::eTlas:
                return "Tlas";
            case ResourceType::eUnknown:
                // Falls through
                    default:
                        return "Unknown";
        }
    }

    bool is_gpu_resource(ResourceType type);

    std::ostream& operator<<(std::ostream& os, const ResourceType& type);
}
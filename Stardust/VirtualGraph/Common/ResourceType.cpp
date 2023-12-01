#include "ResourceType.hpp"

#include <set>

namespace Nebula::RenderGraph
{
    bool is_gpu_resource(const ResourceType type)
    {
        static std::set non_gpu_types = {
            ResourceType::eCamera, ResourceType::eObjects, ResourceType::eScene, ResourceType::eUnknown
        };

        return !non_gpu_types.contains(type);
    }

    std::ostream& operator<<(std::ostream& os, const ResourceType& type)
    {
        os << get_resource_type_str(type);
        return os;
    }
}
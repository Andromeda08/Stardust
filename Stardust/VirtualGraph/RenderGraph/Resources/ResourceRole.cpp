#include "ResourceRole.hpp"

namespace Nebula::RenderGraph
{
    std::string get_resource_role_str(ResourceRole role)
    {
        switch (role)
        {
            case ResourceRole::eInput:
                return "Input";
            case ResourceRole::eOutput:
                return "Output";
            case ResourceRole::eUnknown:
            default:
                return "Unknown";
        }
    }
}
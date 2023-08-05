#pragma once

#include <vector>
#include <VirtualGraph/RenderGraph/Resources/ResourceRequirement.hpp>

namespace Nebula::RenderGraph
{
    struct RenderNodeConfiguration
    {
        const std::string name = "Render";
        const std::vector<ResourceRequirement> resource_requirements = {
                { "Objects", ResourceRole::eInput, ResourceType::eObjects },
                { "Active Camera", ResourceRole::eInput, ResourceType::eCamera },
                { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
                { "Render Image", ResourceRole::eOutput, ResourceType::eImage },
                { "G-Buffer", ResourceRole::eOutput, ResourceType::eImage },
                { "Depth Image", ResourceRole::eOutput, ResourceType::eImage },
        };

        bool with_shadows = true;
        bool raytraced_shadows = true;
    };
}
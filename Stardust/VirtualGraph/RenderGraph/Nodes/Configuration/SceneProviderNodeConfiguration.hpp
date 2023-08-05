#pragma once

#include <vector>
#include <VirtualGraph/RenderGraph/Resources/ResourceRequirement.hpp>

namespace Nebula::RenderGraph
{
    struct SceneProviderNodeConfiguration
    {
        const std::string name = "Scene Provider";
        const std::vector<ResourceRequirement> resource_requirements = {
                { "Objects", ResourceRole::eOutput, ResourceType::eObjects },
                { "Active Camera", ResourceRole::eOutput, ResourceType::eCamera },
                { "TLAS", ResourceRole::eOutput, ResourceType::eTlas },
        };
    };
}
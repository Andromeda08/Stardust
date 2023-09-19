#include "LightingPass.hpp"

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
    const std::vector<ResourceSpecification> LightingPass::s_resource_specs = {
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "G-Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Albedo Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "AO Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32Sfloat },
        { "AA Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR16G16B16A16Sfloat },
        { "Shadow Maps", ResourceRole::eInput, ResourceType::eImageArray, vk::Format::eR16G16B16A16Sfloat },
        { "Depth Image", ResourceRole::eInput, ResourceType::eDepthImage },
        { "Lighting Result", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
    #pragma endregion

    std::string to_string(LightingPassShadowMode shadow_mode)
    {
        {
            switch (shadow_mode)
            {

                case LightingPassShadowMode::eRayQuery:
                    return "Ray Query";
                case LightingPassShadowMode::eShadowMaps:
                    return "Shadow Maps";
                case LightingPassShadowMode::eNone:
                    // Falls through
                default:
                    return "Disabled";
            }
        }
    }
}
#include "PresentNode.hpp"

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
    const std::vector<ResourceSpecification> PresentNode::s_resource_specs = {
        { "Final Image", ResourceRole::eInput, ResourceType::eImage },
    };
    #pragma endregion
}
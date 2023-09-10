#include "RenderNode.hpp"
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>

namespace Nebula::RenderGraph
{
    std::vector<ResourceSpecification> RenderNode::s_resource_specs = {
        { "Objects", ResourceRole::eInput, ResourceType::eObjects },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "G-Buffer", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Render Image", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Image", ResourceRole::eOutput, ResourceType::eDepthImage },
    };

    RenderNode::RenderNode(const sdvk::Context& context)
    : m_context(context)
    {
    }

    void RenderNodeRenderer::initialize(const sdvk::Context& context)
    {
    }
}
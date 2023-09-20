#include "AmbientOcclusionNode.hpp"

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
    const std::vector<ResourceSpecification> AmbientOcclusionNode::s_resource_specs = {
        { "Position Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Normal Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "AO Image", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32Sfloat },
    };
    #pragma endregion

    AmbientOcclusionNode::AmbientOcclusionNode(const sdvk::Context& context)
    : Node("Ambient Occlusion", NodeType::eAmbientOcclusion)
    , m_context(context)
    {
    }

    void AmbientOcclusionNode::execute(const vk::CommandBuffer& command_buffer)
    {
        m_mode->execute(command_buffer);
    }

    void AmbientOcclusionNode::initialize()
    {
        auto* strategy = AmbientOcclusionStrategy::Factory(m_context, m_resources).create(m_options.mode);
        m_mode = std::shared_ptr<AmbientOcclusionStrategy>(strategy);
        m_mode->initialize(m_options);
    }
}
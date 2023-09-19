#include "SceneProviderNode.hpp"

namespace Nebula::RenderGraph
{
    std::vector<ResourceSpecification> SceneProviderNode::s_resource_specs = {
        { "Objects", ResourceRole::eOutput, ResourceType::eObjects },
        { "Camera", ResourceRole::eOutput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eOutput, ResourceType::eTlas },
    };

    SceneProviderNode::SceneProviderNode(const std::shared_ptr<sd::Scene>& scene)
    : Node("Scene Provider", NodeType::eSceneProvider), m_scene(scene)
    {
    }
}
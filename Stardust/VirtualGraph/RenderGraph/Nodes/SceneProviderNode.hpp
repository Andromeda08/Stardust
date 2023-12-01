#pragma once

#include <memory>
#include <vector>
#include <Scene/Scene.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include "Node.hpp"

namespace Nebula::RenderGraph
{
    class SceneProviderNode final : public Node
    {
    public:
        explicit SceneProviderNode(const std::shared_ptr<sd::Scene>& scene);

        ~SceneProviderNode() override = default;

    private:
        std::shared_ptr<sd::Scene> m_scene;

        DEF_RESOURCE_REQUIREMENTS();
    };
}
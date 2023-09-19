#pragma once

#include <memory>
#include <vector>
#include <Scene/Scene.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include "Node.hpp"

namespace Nebula::RenderGraph
{
    class SceneProviderNode : public Node
    {
    public:
        static std::vector<ResourceSpecification> s_resource_specs;

    public:
        explicit SceneProviderNode(const std::shared_ptr<sd::Scene>& scene);

        const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; }

        ~SceneProviderNode() override = default;

    private:
        std::shared_ptr<sd::Scene> m_scene;
    };
}
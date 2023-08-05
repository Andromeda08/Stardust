#pragma once

#include <Scene/Scene.hpp>
#include "Node.hpp"

namespace Nebula::RenderGraph
{
    class SceneProviderNode : public Node
    {
    private:
        std::shared_ptr<sd::Scene> m_scene;

    public:
        void execute() override { /* no-op */ }
    };
}
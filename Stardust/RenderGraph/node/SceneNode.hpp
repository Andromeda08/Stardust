#pragma once

#include <RenderGraph//Scene.hpp>
#include <RenderGraph/RenderGraph.hpp>

namespace sd::rg
{
    // This special node should be "auto-generated" by the system
    class SceneNode : public Node
    {
    public:
        explicit SceneNode(const std::shared_ptr<Scene>& scene);

        void execute(const vk::CommandBuffer&) override { /* no-op */ }

        void compile() override { /* no-op */ }

    public:
        // This special node has no inputs.
        std::shared_ptr<Scene> m_scene;

        // A SceneNode has 3 outputs:
        // - Camera
        // - Objects
        // - Tlas
        std::vector<std::unique_ptr<Output>> m_outputs;
    };
}
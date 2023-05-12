#pragma once

#include <RenderGraph/Input.hpp>
#include <RenderGraph/Node.hpp>
#include <RenderGraph/Output.hpp>
#include <RenderGraph/Scene.hpp>

namespace sd::rg
{
    // This special node should be "auto-generated" by the system
    class SceneNode : public Node
    {
    public:
        explicit SceneNode(const std::shared_ptr<Scene>& scene);

        void execute(const vk::CommandBuffer&) override { /* no-op */ }

        void compile() override { /* no-op */ }

        void draw() override;

    public:
        // This special node has no inputs.
        std::shared_ptr<Scene> m_scene;

        // A SceneNode has 3 outputs:
        // - Objects
        // - Camera
        // - Tlas
        std::vector<std::unique_ptr<Output>> m_outputs;
    };
}
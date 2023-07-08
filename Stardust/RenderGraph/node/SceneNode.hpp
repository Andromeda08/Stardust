#pragma once

#include <RenderGraph/Input.hpp>
#include <RenderGraph/Node.hpp>
#include <RenderGraph/Output.hpp>
#include <Scene/Scene.hpp>

namespace sd::rg
{
    /* Outputs:
     * [0] Objects
     * [1] Camera
     * [2] Top-level Acceleration Structure
     */
    class SceneNode : public Node
    {
    public:
        explicit SceneNode(const std::shared_ptr<sd::Scene>& scene);

        void execute(const vk::CommandBuffer&) override { /* no-op */ }

        void compile() override { /* no-op */ }

        void draw() override;

    public:
        std::shared_ptr<sd::Scene> m_scene;
    };
}
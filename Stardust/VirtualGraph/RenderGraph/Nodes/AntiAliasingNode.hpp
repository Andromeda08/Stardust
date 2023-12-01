#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <Vulkan/Buffer.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula::RenderGraph
{
    struct AntiAliasingNodePushConstant
    {
        glm::vec2 resolution_rcp;
    };

    struct AntiAliasingNodeOptions
    {
        bool enabled {true};
        bool debug_show_edges {false};
    };

    class AntiAliasingNode final : public Node
    {
    public:
        explicit AntiAliasingNode(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        void _update_descriptor(uint32_t current_frame);

        AntiAliasingNodeOptions m_options {};

        struct Renderer
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;
            vk::Sampler sampler;

            std::array<vk::ClearValue, 1> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            glm::vec2 render_resolution_rcp;
        } m_renderer;

        const sdvk::Context& m_context;

        DEF_RESOURCE_REQUIREMENTS();
    };
}
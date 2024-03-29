#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <Vulkan/Buffer.hpp>

namespace sdvk
{
    class Swapchain;
}

namespace Nebula::RenderGraph
{
    struct PresentNodeOptions
    {
        bool flip_image {false};
    };

    struct PresentNodePushConstant
    {
        glm::ivec4 options {};

        explicit PresentNodePushConstant(const PresentNodeOptions& _options)
        {
            options = glm::ivec4(0);
            options[0] = _options.flip_image ? 1 : 0;
        }
    };

    class PresentNode final : public Node
    {
    public:
        explicit PresentNode(const sdvk::Context& context, const sdvk::Swapchain& swapchain, const PresentNodeOptions& options);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        void _update_descriptor(uint32_t current_frame);

        PresentNodeOptions m_options;

        struct Renderer
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;
            std::array<vk::ClearValue, 1> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            vk::Sampler sampler;
        } m_renderer;

        const sdvk::Context& m_context;
        const sdvk::Swapchain& m_swapchain;

        DEF_RESOURCE_REQUIREMENTS();
    };
}
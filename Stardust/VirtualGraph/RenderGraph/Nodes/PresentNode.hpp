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
    class PresentNode : public Node
    {
    public:
        explicit PresentNode(const sdvk::Context& context, const sdvk::Swapchain& swapchain);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        void _update_descriptor(uint32_t current_frame);

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

    public:
        static const std::vector<ResourceSpecification> s_resource_specs;
        const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; }
    };
}
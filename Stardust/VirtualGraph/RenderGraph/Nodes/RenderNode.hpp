#pragma once

#include <array>
#include <map>
#include <memory>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Nebula/Descriptor.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include "Node.hpp"

namespace Nebula::RenderGraph
{
    class Resource;

    // Exposes various options to customise the rendering process.
    struct RenderNodeOptions
    {
        bool with_shadows = true;
    };

    // Contains all resources relevant for rendering
    struct RenderNodeRenderer
    {
        //static constexpr auto& s_frames_in_flight = sd::Application::s_max_frames_in_flight;

        static constexpr std::string_view s_default_vertex_shader = "basic.vert.spv";
        static constexpr std::string_view s_default_fragment_shader = "basic.frag.spv";

        std::array<float, 4> clear_color { 0.3f, 0.3f, 0.3f, 1.0f };

        std::array<vk::ClearValue, 3> clear_values;
        std::unique_ptr<Descriptor> descriptor;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipeline_layout;
        vk::RenderPass renderpass;
        vk::Extent2D render_resolution { 1920, 1080 };

        void initialize(const sdvk::Context& context);
    };

    // PushConstant block used while rendering, also contains options.
    struct RenderNodePushConstant : RenderNodeOptions
    {
        explicit RenderNodePushConstant(const RenderNodeOptions& options): RenderNodeOptions(options) {}
    };

    class RenderNode : public Node
    {
    public:
        explicit RenderNode(const sdvk::Context& context);

        ~RenderNode() override = default;

    public:
        static std::vector<ResourceSpecification> s_resource_specs;

    private:
        RenderNodeRenderer m_renderer {};
        RenderNodeOptions  m_options  {};

        const sdvk::Context& m_context;
    };
}
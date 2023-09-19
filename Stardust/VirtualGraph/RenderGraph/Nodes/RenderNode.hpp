#pragma once

#include <array>
#include <map>
#include <memory>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Buffer.hpp>
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
        std::shared_ptr<Descriptor> descriptor;
        std::vector<vk::Framebuffer> framebuffers;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipeline_layout;
        vk::RenderPass renderpass;
        vk::Extent2D render_resolution { 1920, 1080 };
        uint32_t frames_in_flight {2};

        std::vector<std::unique_ptr<sdvk::Buffer>> ub_camera;
    };

    // PushConstant block used while rendering, also contains options.
    struct RenderNodePushConstant
    {
        explicit RenderNodePushConstant(const RenderNodeOptions& options) {}

        // glm::ivec4
    };

    class RenderNode : public Node
    {
    public:
        explicit RenderNode(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        ~RenderNode() override = default;

        const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; }

    protected:
        bool _validate_resource(const std::string& key, const std::shared_ptr<Resource>& resource) override;

    private:
        void _initialize_renderer();

        void _update_descriptors(uint32_t current_frame);

    public:
        static std::vector<ResourceSpecification> s_resource_specs;

    private:
        RenderNodeRenderer m_renderer {};
        RenderNodeOptions  m_options  {};

        const sdvk::Context& m_context;
    };
}
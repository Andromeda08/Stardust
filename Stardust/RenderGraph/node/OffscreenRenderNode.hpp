#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Application/Application.hpp>
#include <RenderGraph/RenderGraph.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/v2/Descriptor.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>

namespace sd::rg
{
    class OffscreenRenderNode : public Node
    {
    public:
        OffscreenRenderNode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void compile() override;

    private:
        void _init_inputs();

        void _init_outputs(const sdvk::Context& context);

        void _init_renderer(const sdvk::Context& context);

        void _update_descriptors(uint32_t current_frame);

    private:
        static constexpr auto& n_frames_in_flight = sd::Application::s_max_frames_in_flight;

        static constexpr std::string_view s_vertex_shader   = "osr.vert.spv";
        static constexpr std::string_view s_fragment_shader = "osr.frag.spv";

        struct Parameters
        {
            std::array<float, 4> clear_color  { 0.3f, 0.3f, 0.3f, 1.0f };
            vk::Extent2D         resolution   { 1920, 1080 };
            vk::Bool32           with_shadows { true };
        } m_parameters;

        struct Renderer
        {
            std::array<vk::Framebuffer,               n_frames_in_flight> framebuffers;
            std::array<std::unique_ptr<sdvk::Buffer>, n_frames_in_flight> uniform_camera;

            std::unique_ptr<sdvk::Descriptor2<n_frames_in_flight>> descriptor2;

            std::unique_ptr<sdvk::Descriptor> descriptor;
            std::array<vk::ClearValue, 3>     clear_values;
            vk::RenderPass                    renderpass;
            vk::Pipeline                      pipeline;
            vk::PipelineLayout                pipeline_layout;

        } m_renderer;

        struct pcNodeParams
        {
            explicit pcNodeParams(Parameters params) : with_shadows(params.with_shadows) {}

            vk::Bool32 with_shadows { true };
        };

    public:
        // An OffscreenRenderNode expects 3 inputs
        // [1] List of objects from a Scene
        // [2] Camera from a Scene
        // [3] Top level acceleration structure
        std::vector<std::unique_ptr<Input>> m_inputs;

        // An OffscreenRenderNode has 3 outputs
        // [1] Render image
        // [2] G-Buffer image
        // [3] Depth image
        std::vector<std::unique_ptr<Output>> m_outputs;

        const sdvk::Context& m_context;
    };
}
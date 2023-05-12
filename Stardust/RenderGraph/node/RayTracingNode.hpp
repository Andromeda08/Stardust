#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Application/Application.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Node.hpp>
#include <RenderGraph/Output.hpp>
#include <RenderGraph/Scene.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/v2/Descriptor.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include <Vulkan/Raytracing/ShaderBindingTable.hpp>

namespace sd::rg
{
    class RayTracingNode : public Node
    {
    public:
        RayTracingNode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void compile() override;

        void draw(uint32_t id) override;

    private:
        void _init_inputs();

        void _init_outputs(const sdvk::Context& context);

        void _init_renderer(const sdvk::Context& context);

        void _update_descriptors(uint32_t current_frame);

    private:
        static constexpr auto& n_frames_in_flight = sd::Application::s_max_frames_in_flight;

        struct Parameters
        {
            vk::Extent2D resolution { 1920, 1080 };
        } m_parameters;

        struct Renderer
        {
            using descriptor_t = sdvk::Descriptor2<n_frames_in_flight>;

            std::array<vk::Framebuffer,               n_frames_in_flight> framebuffers;
            std::array<std::unique_ptr<sdvk::Buffer>, n_frames_in_flight> uniform_camera;

            std::unique_ptr<rt::ShaderBindingTable> sbt;
            std::unique_ptr<descriptor_t> descriptor2;
            vk::Pipeline                  pipeline;
            vk::PipelineLayout            pipeline_layout;
        } m_renderer;

        // A RayTracingNode expects 3 inputs
        // [0] List of objects from a Scene
        // [1] Camera from a Scene
        // [2] Top level acceleration structure
        std::vector<std::unique_ptr<Input>> m_inputs;

        // An OffscreenRenderNode has 1 output
        // [0] Render image
        std::vector<std::unique_ptr<Output>> m_outputs;
    };
}

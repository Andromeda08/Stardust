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
    class Context;
}

namespace Nebula::RenderGraph
{
    struct PrePassPushConstant
    {
        glm::mat4 model_matrix {1.0f};
        glm::vec4 color {0.5f};
    };

    struct PrePassUniform
    {
        sd::CameraUniformData current;
        sd::CameraUniformData previous;
    };

    class PrePass : public Node
    {
    public:
        explicit PrePass(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~PrePass() override = default;

    private:
        void _update_descriptor(uint32_t current_frame);

        struct Renderer
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;

            std::array<vk::ClearValue, 5> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform;

            sd::CameraUniformData previous_frame_camera_state;
        } m_renderer;

        const sdvk::Context& m_context;

    public:
        const std::vector<ResourceSpecification>& get_resource_specs() const override
        {
            return s_resource_specs;
        }

        static const std::vector<ResourceSpecification> s_resource_specs;
    };
}
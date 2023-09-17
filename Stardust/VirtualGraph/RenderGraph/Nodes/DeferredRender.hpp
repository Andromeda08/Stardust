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
    struct DeferredPassPushConstant
    {
        glm::mat4 model_matrix {1.0f};
        glm::vec4 color {0.5f};
    };

    // TODO: Rename to DeferredPass
    class DeferredRender : public Node
    {
    public:
        static const std::vector<ResourceSpecification> s_resource_specs;

    public:
        explicit DeferredRender(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~DeferredRender() override = default;

    private:
        struct Renderer
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;

            std::array<vk::ClearValue, 3> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform;
        } m_renderer;

        const sdvk::Context& m_context;
    };
}
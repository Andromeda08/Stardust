#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Raytracing/ShaderBindingTable.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula::RenderGraph
{
    class RayTracingNode : public Node
    {
    public:
        explicit RayTracingNode(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        Image& get_output();

        void update_descriptor(uint32_t index);

        struct Renderer {
            uint32_t                    frames_in_flight;
            vk::Extent2D                render_resolution;
            vk::Pipeline                pipeline;
            vk::PipelineLayout          pipeline_layout;
            vk::Sampler                 sampler;
            std::shared_ptr<Descriptor> descriptor;

            std::shared_ptr<sd::rt::ShaderBindingTable> sbt;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform;
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
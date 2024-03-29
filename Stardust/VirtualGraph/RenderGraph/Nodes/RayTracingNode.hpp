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
    struct RayTracingNodeOptions
    {
        int32_t reflection_count {0};

        const glm::ivec2 reflection_bounds { 0, 5 };
    };
    
    struct RayTracingPushConstant
    {
        glm::ivec4 options {};

        explicit RayTracingPushConstant(const RayTracingNodeOptions& _options)
        {
            options = glm::ivec4(0);
            options[0] = 1 + std::clamp(_options.reflection_count, _options.reflection_bounds.x, _options.reflection_bounds.y);
        }
    };

    class RayTracingNode final : public Node
    {
    public:
        explicit RayTracingNode(const sdvk::Context& context, const RayTracingNodeOptions& options);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        Image& get_output();

        void update_descriptor(uint32_t index);

        RayTracingNodeOptions m_options;
        
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

        DEF_RESOURCE_REQUIREMENTS();
    };
}
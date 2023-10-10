#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <Vulkan/Buffer.hpp>

namespace Nebula::RenderGraph
{
    struct BlurNodePushConstant
    {
        glm::ivec4 direction_vector;
    };

    class BlurNode : public Node
    {
    public:
        explicit BlurNode(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        void _update_descriptor(uint32_t current_frame, uint32_t pass);

        struct ComputeKernel
        {
            std::shared_ptr<Image> intermediate_image;
            std::shared_ptr<Descriptor> descriptor_pass_x;
            std::shared_ptr<Descriptor> descriptor_pass_y;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            std::vector<vk::Sampler> samplers;
            vk::Extent2D resolution;
            uint32_t frames_in_flight;
        } m_kernel;

        const sdvk::Context& m_context;

    public:
        const std::vector<ResourceSpecification>& get_resource_specs() const override
        {
            return s_resource_specs;
        }

        static const std::vector<ResourceSpecification> s_resource_specs;
    };
}
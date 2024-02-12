#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Nebula/Descriptor.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <Vulkan/Buffer.hpp>


namespace Nebula::RenderGraph
{
    DEF_BASIC_EDITOR_NODE(BloomEditorNode);

    struct BloomPushConstant
    {
        int32_t size       {5};
        float   separation {3};
        float   threshold  {0.4f};
        float   amount     {1};
    };

    class BloomNode final : public Node
    {
    public:
        explicit BloomNode(const sdvk::Context& context);

        void initialize() override;

        void execute(const vk::CommandBuffer& command_buffer) override;

    private:
        void update_descriptor(uint32_t current_frame);

        static constexpr uint32_t    s_group_size    = 8;
        static constexpr std::string s_shader_name   = "rg_bloom";
        static constexpr std::string id_input_image  = "Input Image";
        static constexpr std::string id_output_image = "Output Image";

        struct ComputeKernel
        {
            std::shared_ptr<Descriptor> descriptor;
            vk::Pipeline                pipeline;
            vk::PipelineLayout          pipeline_layout;
            std::vector<vk::Sampler>    samplers;
            vk::Extent2D                resolution;
            uint32_t                    frames_in_flight;
        } m_kernel;

        const sdvk::Context& m_context;

        DEF_RESOURCE_REQUIREMENTS();
    };
}

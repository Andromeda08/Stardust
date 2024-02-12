#include "BloomNode.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>

#include "Vulkan/Image/Sampler.hpp"

namespace Nebula::RenderGraph
{
#pragma region "Bloom Node Resource Specifications"
    const std::vector<ResourceSpecification> BloomNode::s_resource_specs = {
        { id_input_image, ResourceRole::eInput, ResourceType::eImage },
        { id_output_image, ResourceRole::eOutput, ResourceType::eImage },
    };
#pragma endregion

    namespace Editor
    {
        BloomEditorNode::BloomEditorNode(): Node("Bloom", glm::ivec4(124, 58, 237, 255), glm::ivec4(167, 139, 250, 255), NodeType::eBloom)
        {
            for (const auto& x: RenderGraph::BloomNode::s_resource_specs) {
                m_resource_descriptions.emplace_back(x.name, x.role, x.type);
                m_resource_descriptions.back().spec = x;
            }
        }
    }

    BloomNode::BloomNode(const sdvk::Context& context)
    : Node("Bloom Pass", NodeType::eBloom), m_context(context)
    {
    }

    void BloomNode::initialize()
    {
        m_kernel.resolution       = sd::Application::s_extent.vk_ext();
        m_kernel.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_kernel.samplers.resize(2);
        for (vk::Sampler& sampler : m_kernel.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }

        m_kernel.descriptor = Descriptor::Builder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .create(m_kernel.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(BloomPushConstant) })
            .add_descriptor_set_layout(m_kernel.descriptor->layout())
            .create_pipeline_layout()
            .add_shader("rg_bloom.comp.spv", vk::ShaderStageFlagBits::eCompute)
            .with_name("Bloom Node")
            .create_compute_pipeline();

        m_kernel.pipeline        = pipeline;
        m_kernel.pipeline_layout = pipeline_layout;
    }

    void BloomNode::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;

        const auto input  = m_resources[id_input_image]->as<ImageResource>().get_image();
        const auto output = m_resources[id_output_image]->as<ImageResource>().get_image();

        Sync::ImageBarrier(input, input->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Sync::ImageBarrier(output, output->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);

        const uint32_t group_x = (m_kernel.resolution.width + s_group_size - 1) / s_group_size;
        const uint32_t group_y = (m_kernel.resolution.height + s_group_size - 1) / s_group_size;

        update_descriptor(current_frame);

        constexpr BloomPushConstant push_constant {};
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_kernel.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_kernel.pipeline_layout, 0, 1, &m_kernel.descriptor->set(current_frame), 0, nullptr);
        command_buffer.pushConstants(m_kernel.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(BloomPushConstant), &push_constant);
        command_buffer.dispatch(group_x, group_y, 1);
    }

    void BloomNode::update_descriptor(const uint32_t current_frame)
    {
        const auto input  = m_resources[id_input_image]->as<ImageResource>().get_image();
        const auto output = m_resources[id_output_image]->as<ImageResource>().get_image();

        const vk::DescriptorImageInfo input_info { m_kernel.samplers[0], input->image_view(), input->state().layout };
        const vk::DescriptorImageInfo output_info { m_kernel.samplers[1], output->image_view(), output->state().layout };

        m_kernel.descriptor->begin_write(current_frame)
            .storage_image(0, input_info)
            .storage_image(1, output_info)
            .commit();
    }
}

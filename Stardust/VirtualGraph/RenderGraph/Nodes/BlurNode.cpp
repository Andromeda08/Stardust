#include "BlurNode.hpp"
#include <Vulkan/Image/Sampler.hpp>
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula::RenderGraph
{
#pragma region Resource Specifications
    const std::vector<ResourceSpecification> BlurNode::s_resource_specs = {
        { "Blur Input", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Blur Output", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
#pragma endregion

    BlurNode::BlurNode(const sdvk::Context& context)
    : Node("BlurNode", NodeType::eGaussianBlur)
    , m_context(context)
    {
    }

    void BlurNode::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;

        const auto blur_in = m_resources["Blur Input"]->as<ImageResource>().get_image();
        const auto blur_out = m_resources["Blur Output"]->as<ImageResource>().get_image();

        Sync::ImageBarrierBatch({
            Sync::ImageBarrier(blur_in, blur_in->state().layout, vk::ImageLayout::eGeneral),
            Sync::ImageBarrier(blur_out, blur_out->state().layout, vk::ImageLayout::eGeneral),
            Sync::ImageBarrier(m_kernel.intermediate_image, m_kernel.intermediate_image->state().layout, vk::ImageLayout::eGeneral),
        }).apply(command_buffer);

        BlurNodePushConstant pc {};
        constexpr uint32_t group_size = 16;
        const auto group_x = (m_kernel.resolution.width + group_size - 1) / group_size;
        const auto group_y = (m_kernel.resolution.height + group_size - 1) / group_size;

        _update_descriptor(current_frame, 0);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_kernel.pipeline);

        pc.direction_vector = glm::ivec4(1, 0, 0, 0);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_kernel.pipeline_layout, 0, 1, &m_kernel.descriptor_pass_x->set(current_frame), 0, nullptr);
        command_buffer.pushConstants(m_kernel.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(BlurNodePushConstant), &pc);
        command_buffer.dispatch(group_x, group_y, 1);

        _update_descriptor(current_frame, 1);
        pc.direction_vector = glm::ivec4(0, 1, 0, 0);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_kernel.pipeline_layout, 0, 1, &m_kernel.descriptor_pass_y->set(current_frame), 0, nullptr);
        command_buffer.pushConstants(m_kernel.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(BlurNodePushConstant), &pc);
        command_buffer.dispatch(group_x, group_y, 1);
    }

    void BlurNode::initialize()
    {
        const auto input = m_resources["Blur Input"]->as<ImageResource>().get_image();
        m_kernel.intermediate_image = std::make_shared<Image>(m_context, input->properties().format, input->properties().extent,
                                                              vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eStorage,
                                                              vk::ImageAspectFlagBits::eColor, vk::ImageTiling::eOptimal,
                                                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                              "GaussianBlurIntermediateImage");

        m_kernel.resolution = sd::Application::s_extent.vk_ext();
        m_kernel.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_kernel.samplers.resize(2);
        for (vk::Sampler& sampler : m_kernel.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }

        m_kernel.descriptor_pass_x = Descriptor::Builder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .create(m_kernel.frames_in_flight, m_context);

        m_kernel.descriptor_pass_y = Descriptor::Builder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .create(m_kernel.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(BlurNodePushConstant) })
            .add_descriptor_set_layout(m_kernel.descriptor_pass_x->layout())
            .create_pipeline_layout()
            .add_shader("rg_gaussian_blur.comp.spv", vk::ShaderStageFlagBits::eCompute)
            .with_name("Gaussian Blur Node")
            .create_compute_pipeline();

        m_kernel.pipeline = pipeline;
        m_kernel.pipeline_layout = pipeline_layout;
    }

    void BlurNode::_update_descriptor(const uint32_t current_frame, const uint32_t pass)
    {
        if (pass > 1)
        {
            throw std::runtime_error(std::format("[Error] BlurNode tried to update descriptors for invalid pass index \"{}\"", pass));
        }

        std::shared_ptr<Image> input;
        std::shared_ptr<Image> output;
        std::shared_ptr<Descriptor> descriptor;

        if (pass == 0)
        {
            input = m_resources["Blur Input"]->as<ImageResource>().get_image();
            output = m_kernel.intermediate_image;
            descriptor = m_kernel.descriptor_pass_x;
        }

        if (pass == 1)
        {
            input = m_kernel.intermediate_image;
            output = m_resources["Blur Output"]->as<ImageResource>().get_image();
            descriptor = m_kernel.descriptor_pass_y;
        }

        const vk::DescriptorImageInfo input_info { m_kernel.samplers[0], input->image_view(), input->state().layout };
        const vk::DescriptorImageInfo output_info { m_kernel.samplers[1], output->image_view(), output->state().layout };

        descriptor->begin_write(current_frame)
            .storage_image(0, input_info)
            .storage_image(1, output_info)
            .commit();
    }
}
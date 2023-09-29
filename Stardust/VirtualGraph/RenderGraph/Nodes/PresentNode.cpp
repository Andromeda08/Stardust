#include "PresentNode.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Sampler.hpp>

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
    const std::vector<ResourceSpecification> PresentNode::s_resource_specs = {
        { "Final Image", ResourceRole::eInput, ResourceType::eImage },
    };
    #pragma endregion

    PresentNode::PresentNode(const sdvk::Context& context, const sdvk::Swapchain& swapchain)
    : Node("Present Node", NodeType::ePresent)
    , m_context(context)
    , m_swapchain(swapchain)
    {
    }

    void PresentNode::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = sd::Application::s_current_frame;

        const auto& input = dynamic_cast<ImageResource&>(*m_resources["Final Image"]).get_image();
        auto input_barrier = Sync::ImageBarrier(input, input->state().layout, vk::ImageLayout::eGeneral);

        auto render_commands = [&](const vk::CommandBuffer& cmd){
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout, 0, 1,
                                   &m_renderer.descriptor->set(current_frame),
                                   0, nullptr);
            cmd.draw(3, 1, 0, 0);
        };

        input_barrier.apply(command_buffer);

        _update_descriptor(current_frame);
        auto framebuffer = m_renderer.framebuffers->get(current_frame);
        sdvk::RenderPass::Execute()
            .with_clear_values<1>(m_renderer.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, render_commands);
    }

    void PresentNode::initialize()
    {
        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_renderer.clear_values[0].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

        m_renderer.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_swapchain.format(), vk::SampleCountFlagBits::e1, vk::ImageLayout::ePresentSrcKHR)
            .make_subpass()
            .create(m_context);

        m_renderer.framebuffers = Framebuffer::Builder()
            .add_attachment_for_index(0, m_swapchain.view(0))
            .add_attachment_for_index(1, m_swapchain.view(1))
            .set_render_pass(m_renderer.render_pass)
            .set_size(m_renderer.render_resolution)
            .set_count(m_renderer.frames_in_flight)
            .set_name("Present Framebuffer")
            .create(m_context);

        m_renderer.descriptor = Descriptor::Builder()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(1)
            .add_shader("rg_passthrough.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_present.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("Present Pass")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline;
        m_renderer.pipeline_layout = pipeline_layout;

        m_renderer.sampler = sdvk::SamplerBuilder().create(m_context.device());
    }

    void PresentNode::_update_descriptor(uint32_t current_frame)
    {
        const auto& input = dynamic_cast<ImageResource&>(*m_resources["Final Image"]).get_image();
        vk::DescriptorImageInfo input_info { m_renderer.sampler, input->image_view(), input->state().layout };

        m_renderer.descriptor->begin_write(current_frame)
            .combined_image_sampler(0, input_info)
            .commit();
    }
}
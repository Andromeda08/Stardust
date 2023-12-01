#include "AntiAliasingNode.hpp"
#include <Vulkan/Image/Sampler.hpp>
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>


namespace Nebula::RenderGraph
{
#pragma region Resource Specifications
    const std::vector<ResourceSpecification> AntiAliasingNode::s_resource_specs = {
        { "Anti-Aliasing Input", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Anti-Aliasing Output", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
#pragma endregion

    AntiAliasingNode::AntiAliasingNode(const sdvk::Context& context)
    : Node("Anti-Aliasing Node", NodeType::eAntiAliasing)
    , m_context(context)
    {
    }

    void AntiAliasingNode::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;

        const auto aa_in = m_resources["Anti-Aliasing Input"]->as<ImageResource>().get_image();
        const auto aa_out = m_resources["Anti-Aliasing Output"]->as<ImageResource>().get_image();

        Sync::ImageBarrierBatch({
            Sync::ImageBarrier(aa_in, aa_in->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal),
            Sync::ImageBarrier(aa_out, aa_out->state().layout, vk::ImageLayout::eColorAttachmentOptimal),
        }).apply(command_buffer);

        _update_descriptor(current_frame);

        sdvk::RenderPass::Execute()
            .with_clear_values<1>(m_renderer.clear_values)
            .with_framebuffer(m_renderer.framebuffers->get(current_frame))
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, [&](const vk::CommandBuffer& cmd){
                AntiAliasingNodePushConstant pc {};
                pc.resolution_rcp = m_renderer.render_resolution_rcp;

                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
                cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(AntiAliasingNodePushConstant), &pc);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline_layout, 0, 1,
                                       &m_renderer.descriptor->set(current_frame), 0, nullptr);
                cmd.draw(3, 1, 0, 0);
            });
    }

    void AntiAliasingNode::initialize()
    {
        const auto aa = m_resources["Anti-Aliasing Output"]->as<ImageResource>().get_image();

        const auto extent = sd::Application::s_extent.vk_ext();

        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;
        m_renderer.clear_values[0].setColor(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.render_resolution = extent;
        m_renderer.render_resolution_rcp = glm::vec2 {
            1.0f / static_cast<float>(extent.width),
            1.0f / static_cast<float>(extent.height)
        };

        m_renderer.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(aa->properties().format)
            .make_subpass()
            .create(m_context);

        m_renderer.framebuffers = Framebuffer::Builder()
            .add_attachment(aa->image_view())
            .set_render_pass(m_renderer.render_pass)
            .set_size(m_renderer.render_resolution)
            .set_count(m_renderer.frames_in_flight)
            .set_name("Anti-Aliasing Framebuffer")
            .create(m_context);

        m_renderer.descriptor = Descriptor::Builder()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(AntiAliasingNodePushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(1)
            .add_shader("rg_fxaa.vert.hlsl.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_fxaa.frag.hlsl.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("Anti-Aliasing")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.sampler = sdvk::SamplerBuilder().create(m_context.device());

        m_renderer.pipeline = pipeline;
        m_renderer.pipeline_layout = pipeline_layout;
    }

    void AntiAliasingNode::_update_descriptor(uint32_t current_frame)
    {
        const auto aa_in = m_resources["Anti-Aliasing Input"]->as<ImageResource>().get_image();
        const vk::DescriptorImageInfo aa_in_info { m_renderer.sampler, aa_in->image_view(), aa_in->state().layout };

        m_renderer.descriptor->begin_write(current_frame)
            .combined_image_sampler(0, aa_in_info)
            .commit();
    }

}
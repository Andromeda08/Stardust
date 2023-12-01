#include "GBufferPass.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula::RenderGraph
{
#pragma region Resource Specifications
     const std::vector<ResourceSpecification> GBufferPass::s_resource_specs = {
        { id_scene_data, ResourceRole::eInput, ResourceType::eScene },
        { id_position_buffer, ResourceRole::eOutput, ResourceType::eImage },
        { id_normal_buffer, ResourceRole::eOutput, ResourceType::eImage },
        { id_albedo_buffer, ResourceRole::eOutput, ResourceType::eImage },
        { id_depth_buffer, ResourceRole::eOutput, ResourceType::eDepthImage },
        { id_motion_vectors, ResourceRole::eOutput, ResourceType::eImage },
    };
#pragma endregion

    GBufferPass::GBufferPass(const sdvk::Context& context)
    : Node("G-Buffer Pass", NodeType::eGBufferPass), m_context(context)
    {
    }

    void GBufferPass::initialize()
    {
        const auto position       = m_resources[id_position_buffer]->as<ImageResource>().get_image();
        const auto normal         = m_resources[id_normal_buffer]->as<ImageResource>().get_image();
        const auto albedo         = m_resources[id_albedo_buffer]->as<ImageResource>().get_image();
        const auto depth          = m_resources[id_depth_buffer]->as<DepthImageResource>().get_depth_image();
        const auto motion_vectors = m_resources[id_motion_vectors]->as<ImageResource>().get_image();

        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        for (int32_t i = 0; i < 4; i++)
        {
            m_renderer.clear_values[i].setColor(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });
        }
        m_renderer.clear_values[4].setDepthStencil({ 1.0f, 0 });

        m_renderer.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(position->properties().format)
            .add_color_attachment(normal->properties().format)
            .add_color_attachment(albedo->properties().format)
            .add_color_attachment(motion_vectors->properties().format)
            .set_depth_attachment(depth->properties().format)
            .make_subpass()
            .create(m_context);

        m_renderer.framebuffers = Framebuffer::Builder()
            .add_attachment(position->image_view())
            .add_attachment(normal->image_view())
            .add_attachment(albedo->image_view())
            .add_attachment(motion_vectors->image_view())
            .add_attachment(depth->image_view())
            .set_render_pass(m_renderer.render_pass)
            .set_size(m_renderer.render_resolution)
            .set_count(m_renderer.frames_in_flight)
            .set_name("G-Buffer Framebuffer")
            .create(m_context);

        m_renderer.descriptor = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PrePassPushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(4)
            .add_attribute_descriptions({ sd::VertexData::attribute_descriptions() })
            .add_binding_descriptions({ sd::VertexData::binding_description() })
            .add_shader("rg_deferred_pass.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_deferred_pass.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("G-Buffer")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline;
        m_renderer.pipeline_layout = pipeline_layout;

        m_renderer.uniform.resize(m_renderer.frames_in_flight);
        for (auto& ub : m_renderer.uniform)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(PrePassUniform))
                .as_uniform_buffer()
                .create(m_context);
        }

        const auto camera = m_resources[id_scene_data]->as<SceneResource>().get_scene()->camera();
        m_renderer.previous_frame_camera_state = camera->uniform_data();
    }

    void GBufferPass::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;

        _update_descriptor(current_frame);

        const auto& objects    = m_resources[id_scene_data]->as<SceneResource>().get_scene()->objects();
        const auto position       = m_resources[id_position_buffer]->as<ImageResource>().get_image();
        const auto normal         = m_resources[id_normal_buffer]->as<ImageResource>().get_image();
        const auto albedo         = m_resources[id_albedo_buffer]->as<ImageResource>().get_image();
        const auto depth          = m_resources[id_depth_buffer]->as<DepthImageResource>().get_depth_image();
        const auto motion_vectors = m_resources[id_motion_vectors]->as<ImageResource>().get_image();

        Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eDepthAttachmentOptimal).apply(command_buffer);
        Sync::ImageBarrier(motion_vectors, motion_vectors->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);

        sdvk::RenderPass::Execute()
            .with_clear_values<5>(m_renderer.clear_values)
            .with_framebuffer(m_renderer.framebuffers->get(current_frame))
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, [&](const vk::CommandBuffer& cmd){
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       m_renderer.pipeline_layout, 0, 1,
                                       &m_renderer.descriptor->set(current_frame),
                                       0, nullptr);

                for (const auto& object : objects)
                {
                    PrePassPushConstant pc {};
                    pc.model_matrix = object.transform.model();
                    pc.color = object.color;

                    cmd.pushConstants(m_renderer.pipeline_layout,
                                      vk::ShaderStageFlagBits::eVertex,
                                      0,
                                      sizeof(PrePassPushConstant),
                                      &pc);

                    object.mesh->draw(cmd);
                }
            });
    }

    void GBufferPass::_update_descriptor(const uint32_t current_frame)
    {
        const auto camera = m_resources[id_scene_data]->as<SceneResource>().get_scene()->camera();
        const auto camera_data = camera->uniform_data();

        PrePassUniform uniform {
            .current = camera_data,
            .previous = m_renderer.previous_frame_camera_state,
        };

        m_renderer.uniform[current_frame]->set_data(&uniform, m_context.device());

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(PrePassUniform))
            .commit();

        m_renderer.previous_frame_camera_state = camera_data;
    }
}
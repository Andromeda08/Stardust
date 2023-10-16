#include "PrePass.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
     const std::vector<ResourceSpecification> PrePass::s_resource_specs = {
        { "Objects", ResourceRole::eInput, ResourceType::eObjects },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "Position Buffer", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Normal Buffer", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Albedo Image", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Image", ResourceRole::eOutput, ResourceType::eDepthImage },
        { "Motion Vectors", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
    #pragma endregion

    PrePass::PrePass(const sdvk::Context& context)
    : Node("PrePass", NodeType::ePrePass), m_context(context)
    {
    }

    void PrePass::initialize()
    {
        const auto& r_position = m_resources["Position Buffer"];
        const auto& r_normal = m_resources["Normal Buffer"];
        const auto& r_albedo = m_resources["Albedo Image"];
        const auto& r_depth = m_resources["Depth Image"];
        const auto& r_mv = m_resources["Motion Vectors"];

        auto position = dynamic_cast<ImageResource&>(*r_position).get_image();
        auto normal = dynamic_cast<ImageResource&>(*r_normal).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*r_albedo).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*r_depth).get_depth_image();
        auto motion_vectors = dynamic_cast<ImageResource&>(*r_mv).get_image();

        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_renderer.clear_values[0].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.clear_values[1].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.clear_values[2].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.clear_values[3].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
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
            .set_name("PrePass Framebuffer")
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
            .with_name("PrePass")
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

        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        m_renderer.previous_frame_camera_state = camera.uniform_data();
    }

    void PrePass::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = sd::Application::s_current_frame;

        _update_descriptor(current_frame);
        auto& objects = (dynamic_cast<ObjectsResource&>(*m_resources["Objects"])).get_objects();

        auto render_commands = [&](const vk::CommandBuffer& cmd){
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout, 0, 1,
                                   &m_renderer.descriptor->set(current_frame),
                                   0, nullptr);

            for (const auto& obj : objects)
            {
                PrePassPushConstant pc {};
                pc.model_matrix = obj.transform.model();
                pc.color = obj.color;

                cmd.pushConstants(m_renderer.pipeline_layout,
                                  vk::ShaderStageFlagBits::eVertex,
                                  0,
                                  sizeof(PrePassPushConstant),
                                  &pc);

                obj.mesh->draw(cmd);
            }
        };


        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*m_resources["Albedo Image"]).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*m_resources["Depth Image"]).get_depth_image();
        auto mv = dynamic_cast<ImageResource&>(*m_resources["Motion Vectors"]).get_image();

        Nebula::Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eDepthAttachmentOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(mv, mv->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);

        auto framebuffer = m_renderer.framebuffers->get(current_frame);
        sdvk::RenderPass::Execute()
            .with_clear_values<5>(m_renderer.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, render_commands);
    }

    void PrePass::_update_descriptor(uint32_t current_frame)
    {
        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        auto camera_data = camera.uniform_data();

        PrePassUniform uniform {};
        uniform.current = camera_data;
        uniform.previous = m_renderer.previous_frame_camera_state;

        m_renderer.uniform[current_frame]->set_data(&uniform, m_context.device());

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(PrePassUniform))
            .commit();

        m_renderer.previous_frame_camera_state = camera_data;
    }
}
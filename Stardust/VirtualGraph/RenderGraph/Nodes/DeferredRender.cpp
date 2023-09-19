#include "DeferredRender.hpp"
#include <Application/Application.hpp>
#include <Nebula/Image.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
     const std::vector<ResourceSpecification> DeferredRender::s_resource_specs = {
        { "Objects", ResourceRole::eInput, ResourceType::eObjects },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "G-Buffer", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Albedo Image", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Image", ResourceRole::eOutput, ResourceType::eDepthImage },
    };
    #pragma endregion

    DeferredRender::DeferredRender(const sdvk::Context& context)
    : Node("Deferred Pass", NodeType::eDeferredRender), m_context(context)
    {
    }

    void DeferredRender::initialize()
    {
        const auto& r_gbuffer = m_resources["G-Buffer"];
        const auto& r_albedo = m_resources["Albedo Image"];
        const auto& r_depth = m_resources["Depth Image"];

        auto gbuffer = dynamic_cast<ImageResource&>(*r_gbuffer).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*r_albedo).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*r_depth).get_depth_image();

        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_renderer.clear_values[0].setColor(std::array<float, 4>{ 0.3f, 0.3f, 0.3f, 1.0f });
        m_renderer.clear_values[1].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.clear_values[2].setDepthStencil({ 1.0f, 0 });

        m_renderer.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(gbuffer->properties().format)
            .add_color_attachment(albedo->properties().format)
            .set_depth_attachment(depth->properties().format)
            .make_subpass()
            .create(m_context);

        m_renderer.framebuffers = Framebuffer::Builder()
            .add_attachment(gbuffer->image_view())
            .add_attachment(albedo->image_view())
            .add_attachment(depth->image_view())
            .set_render_pass(m_renderer.render_pass)
            .set_size(m_renderer.render_resolution)
            .set_count(m_renderer.frames_in_flight)
            .create(m_context);

        m_renderer.descriptor = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .acceleration_structure(1, vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto pipeline = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(DeferredPassPushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(2)
            .add_attribute_descriptions({ sd::VertexData::attribute_descriptions() })
            .add_binding_descriptions({ sd::VertexData::binding_description() })
            .add_shader("rg_deferred_pass.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_deferred_pass.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("DeferredPass")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline.pipeline;
        m_renderer.pipeline_layout = pipeline.pipeline_layout;

        m_renderer.uniform.resize(m_renderer.frames_in_flight);
        for (auto& ub : m_renderer.uniform)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(sd::CameraUniformData))
                .as_uniform_buffer()
                .create(m_context);
        }
    }

    void DeferredRender::execute(const vk::CommandBuffer& command_buffer)
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
                DeferredPassPushConstant pc {};
                pc.model_matrix = obj.transform.model();
                pc.color = obj.color;

                cmd.pushConstants(m_renderer.pipeline_layout,
                                  vk::ShaderStageFlagBits::eVertex,
                                  0,
                                  sizeof(DeferredPassPushConstant),
                                  &pc);

                obj.mesh->draw(cmd);
            }
        };

        auto framebuffer = m_renderer.framebuffers->get(current_frame);
        sdvk::RenderPass::Execute()
            .with_clear_values(m_renderer.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, render_commands);
    }

    void DeferredRender::_update_descriptor(uint32_t current_frame)
    {
        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        auto camera_data = camera.uniform_data();
        m_renderer.uniform[current_frame]->set_data(&camera_data, m_context.device());

        auto& tlas = dynamic_cast<TlasResource&>(*m_resources["TLAS"]).get_tlas();

        vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &tlas->tlas() };
        vk::DescriptorBufferInfo un_info { m_renderer.uniform[current_frame]->buffer(), 0, sizeof(sd::CameraUniformData) };

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(sd::CameraUniformData))
            .acceleration_structure(1, 1, &tlas->tlas())
            .commit();
    }
}
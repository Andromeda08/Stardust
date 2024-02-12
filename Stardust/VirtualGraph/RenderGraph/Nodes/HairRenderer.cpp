#include "HairRenderer.hpp"
#include <Application/Application.hpp>
#include <Nebula/Image.hpp>
#include <Scene/Camera.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>

namespace Nebula::RenderGraph
{
    #pragma region HairRenderer: Resource Specification and Editor Node

    const std::vector<ResourceSpecification> HairRenderer::s_resource_specs = {
        { id_scene_data, ResourceRole::eInput, ResourceType::eScene },
        { id_output, ResourceRole::eOutput, ResourceType::eImage },
        { id_depth, ResourceRole::eOutput, ResourceType::eDepthImage },
    };

    Editor::HairRendererEditorNode::HairRendererEditorNode()
    : Node("Hair Renderer", { 124, 58, 237, 255 }, { 167, 139, 250, 255 }, NodeType::eHairRenderer)
    {
        for (const auto& spec : HairRenderer::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    #pragma endregion

    HairRenderer::HairRenderer(const sdvk::Context& context)
    : Node("Hair Renderer", NodeType::eHairRenderer), m_context(context)
    {
    }

    void HairRenderer::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;
        update_descriptor(current_frame);

        const auto scene = m_resources[id_scene_data]->as<SceneResource>().get_scene();
        const auto output = m_resources[id_output]->as<ImageResource>().get_image();
        const auto depth = m_resources[id_depth]->as<DepthImageResource>().get_depth_image();

        Sync::ImageBarrierBatch({
            Sync::ImageBarrier(output, output->state().layout, vk::ImageLayout::eColorAttachmentOptimal),
            Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eDepthAttachmentOptimal),
        }).apply(command_buffer);

        sdvk::RenderPass::Execute()
            .with_clear_values<2>(m_clear_values)
            .with_framebuffer(m_framebuffers->get(current_frame))
            .with_render_area({{ 0, 0 }, m_render_resolution})
            .with_render_pass(m_render_pass)
            .execute(command_buffer, [&](const vk::CommandBuffer& cmd)
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, 1, &m_descriptor->set(current_frame), 0, nullptr);

                auto& meshes = scene->meshes();
                for (const auto& object : scene->objects())
                {
                    const std::string mesh_name = object.mesh->name();
                    const HairRendererPushConstant push_constant {
                        meshes.at(mesh_name)->vertex_buffer().address(),
                    };

                    cmd.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eMeshEXT, 0, sizeof(HairRendererPushConstant), &push_constant);
                    cmd.drawMeshTasksEXT(1, 1, 1);
                }
            });
    }

    void HairRenderer::initialize()
    {
        m_render_resolution = sd::Application::s_extent.vk_ext();
        m_frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_descriptor = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment)
            .create(m_frames_in_flight, m_context, "HairRendererCamera");

        m_clear_values[0].setColor(std::array{ 0.0f, 0.0f, 0.0f, 1.0f});
        m_clear_values[1].setDepthStencil({ 1.0f, 0 });

        const auto output = m_resources[id_output]->as<ImageResource>().get_image();
        const auto depth = m_resources[id_depth]->as<DepthImageResource>().get_depth_image();

        m_render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(output->properties().format)
            .set_depth_attachment(depth->properties().format)
            .make_subpass()
            .create(m_context);

        m_framebuffers = Framebuffer::Builder()
            .add_attachment(output->image_view())
            .add_attachment(depth->image_view())
            .set_render_pass(m_render_pass)
            .set_size(m_render_resolution)
            .set_count(m_frames_in_flight)
            .set_name("HairRenderer Framebuffer")
            .create(m_context);

        m_camera_uniform.resize(m_frames_in_flight);
        for (auto& ub : m_camera_uniform)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(sd::CameraUniformData))
                .as_uniform_buffer()
                .create(m_context);
        }

        const auto [a, b] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eMeshEXT, 0, sizeof(HairRendererPushConstant) })
            .add_descriptor_set_layout(m_descriptor->layout())
            .create_pipeline_layout()
            .add_shader("hair_strand.mesh.spv", vk::ShaderStageFlagBits::eMeshEXT)
            .add_shader("hair_strand.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_attachment_count(1)
            .set_sample_count(vk::SampleCountFlagBits::e1)
            //.set_cull_mode(vk::CullModeFlagBits::eNone)
            .create_graphics_pipeline(m_render_pass);

        m_pipeline = a;
        m_pipeline_layout = b;
    }

    void HairRenderer::init_single_strand_pipeline()
    {
        const auto [a, b] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eMeshEXT, 0, sizeof(HairRendererPushConstant) })
            .add_descriptor_set_layout(m_descriptor->layout())
            .create_pipeline_layout()
            .add_shader("hair_strand.mesh.spv", vk::ShaderStageFlagBits::eMeshEXT)
            .add_shader("hair_strand.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_attachment_count(4)
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .create_graphics_pipeline(m_render_pass);

        m_pipeline = a;
        m_pipeline_layout = b;
    }

    void HairRenderer::update_descriptor(const uint32_t current_frame)
    {
        const auto camera = m_resources[id_scene_data]->as<SceneResource>().get_scene()->camera();
        const auto camera_data = camera->uniform_data();

        m_camera_uniform[current_frame]->set_data(&camera_data, m_context.device());

        m_descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_camera_uniform[current_frame]->buffer(), 0, sizeof(sd::CameraUniformData))
            .commit();
    }
}
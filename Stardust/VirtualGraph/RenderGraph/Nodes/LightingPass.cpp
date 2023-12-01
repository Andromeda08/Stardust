#include "LightingPass.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Sampler.hpp>

namespace Nebula::RenderGraph
{
#pragma region Resource Specifications
    const std::vector<ResourceSpecification> LightingPass::s_resource_specs = {
        { "Position Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Normal Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Albedo Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Buffer", ResourceRole::eInput, ResourceType::eDepthImage },
        { "AO Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32Sfloat },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "Lighting Result", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
#pragma endregion

    std::string to_string(const LightingPassShadowMode shadow_mode)
    {
        {
            switch (shadow_mode)
            {
                case LightingPassShadowMode::eRayQuery:
                    return "Ray Query";
                case LightingPassShadowMode::eShadowMaps:
                    return "Shadow Maps";
                case LightingPassShadowMode::eNone:
                    // Falls through
                default:
                    return "Disabled";
            }
        }
    }

    LightingPass::LightingPass(const sdvk::Context& context, const LightingPassOptions& params)
    : Node("Lighting Pass", NodeType::eLightingPass)
    , m_params(params), m_context(context)
    {
    }

    void LightingPass::execute(const vk::CommandBuffer& command_buffer)
    {
        const uint32_t current_frame = sd::Application::s_current_frame;

        const auto position = m_resources["Position Buffer"]->as<ImageResource>().get_image();
        const auto normal = m_resources["Normal Buffer"]->as<ImageResource>().get_image();
        const auto albedo = m_resources["Albedo Buffer"]->as<ImageResource>().get_image();
        const auto depth = m_resources["Depth Buffer"]->as<DepthImageResource>().get_depth_image();
        const auto lr = m_resources["Lighting Result"]->as<ImageResource>().get_image();

        if (m_params.ambient_occlusion)
        {
            const auto ao = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();
            Sync::ImageBarrier(ao, ao->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        }

        Sync::ImageBarrierBatch({
            Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal),
            Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal),
            Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal),
            Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal),
            Sync::ImageBarrier(lr, lr->state().layout, vk::ImageLayout::eColorAttachmentOptimal),
        }).apply(command_buffer);

        _update_descriptor(current_frame);

        sdvk::RenderPass::Execute()
            .with_clear_value(m_renderer.clear_values)
            .with_framebuffer(m_renderer.framebuffers->get(current_frame))
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, [&](const vk::CommandBuffer& cmd){
                LightingPassPushConstant push_constant(m_params);
                push_constant.light_pos = { -12, 10, 5, 1 };

                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
                cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstant), &push_constant);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline_layout, 0, 1,
                                       &m_renderer.descriptor->set(current_frame), 0, nullptr);

                cmd.draw(3, 1, 0, 0);
            });
    }

    void LightingPass::initialize()
    {
        if (const auto ao_image = std::dynamic_pointer_cast<ImageResource>(m_resources["AO Image"]);
            ao_image == nullptr)
        {
            m_params.ambient_occlusion = false;
        }

        const auto lighting_result = m_resources["Lighting Result"]->as<ImageResource>().get_image();

        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_renderer.clear_values[0].setColor(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });

        m_renderer.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(lighting_result->properties().format)
            .make_subpass()
            .create(m_context);

        m_renderer.framebuffers = Framebuffer::Builder()
            .add_attachment(lighting_result->image_view())
            .set_render_pass(m_renderer.render_pass)
            .set_size(m_renderer.render_resolution)
            .set_count(m_renderer.frames_in_flight)
            .set_name("LightingPass Framebuffer")
            .create(m_context);

        const auto tlas_binding = m_params.ambient_occlusion ? 6 : 5;
        auto builder = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(2, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(3, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(4, vk::ShaderStageFlagBits::eFragment)
            .acceleration_structure(tlas_binding, vk::ShaderStageFlagBits::eFragment);

        if (m_params.ambient_occlusion)
        {
            builder.combined_image_sampler(5, vk::ShaderStageFlagBits::eFragment);
        }

        m_renderer.descriptor = builder.create(m_renderer.frames_in_flight, m_context);

        const auto fragment_shader = _select_fragment_shader();
        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(1)
            .add_shader("rg_lighting_pass.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader(fragment_shader, vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("LightingPass")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline;
        m_renderer.pipeline_layout = pipeline_layout;

        m_renderer.uniform.resize(m_renderer.frames_in_flight);
        for (auto& ub : m_renderer.uniform)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(LightingPassUniform))
                .as_uniform_buffer()
                .create(m_context);
        }

        m_renderer.samplers.resize(5);
        for (vk::Sampler& sampler : m_renderer.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }
    }

    void LightingPass::_update_descriptor(uint32_t current_frame)
    {
        const auto position = m_resources["Position Buffer"]->as<ImageResource>().get_image();
        const auto normal = m_resources["Normal Buffer"]->as<ImageResource>().get_image();
        const auto albedo = m_resources["Albedo Buffer"]->as<ImageResource>().get_image();
        const auto depth = m_resources["Depth Buffer"]->as<DepthImageResource>().get_depth_image();

        auto camera = *m_resources["Camera"]->as<CameraResource>().get_camera();
        auto camera_data = camera.uniform_data();
        m_renderer.uniform[current_frame]->set_data(&camera_data, m_context.device());

        auto& tlas = m_resources["TLAS"]->as<TlasResource>().get_tlas();

        LightingPassUniform uniform_data {};
        uniform_data.view = camera_data.view;
        uniform_data.proj = camera_data.proj;
        uniform_data.view_inverse = camera_data.view_inverse;
        uniform_data.proj_inverse = camera_data.proj_inverse;
        uniform_data.eye = camera_data.eye;

        vk::DescriptorImageInfo position_info { m_renderer.samplers[0],position->image_view(),position->state().layout };
        vk::DescriptorImageInfo normal_info { m_renderer.samplers[1],normal->image_view(),normal->state().layout };
        vk::DescriptorImageInfo albedo_info { m_renderer.samplers[2], albedo->image_view(), albedo->state().layout };
        vk::DescriptorImageInfo depth_info { m_renderer.samplers[3], depth->image_view(), depth->state().layout };

        if (m_params.ambient_occlusion)
        {
            auto ao = m_resources["AO Image"]->as<ImageResource>().get_image();
            vk::DescriptorImageInfo ao_info { m_renderer.samplers[3], ao->image_view(), ao->state().layout };

            m_renderer.descriptor->begin_write(current_frame)
                .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform))
                .combined_image_sampler(1, position_info)
                .combined_image_sampler(2, normal_info)
                .combined_image_sampler(3, albedo_info)
                .combined_image_sampler(4, depth_info)
                .combined_image_sampler(5, ao_info)
                .acceleration_structure(6, 1, &tlas->tlas())
                .commit();

            return;
        }

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform))
            .combined_image_sampler(1, position_info)
            .combined_image_sampler(2, normal_info)
            .combined_image_sampler(3, albedo_info)
            .combined_image_sampler(4, depth_info)
            .acceleration_structure(5, 1, &tlas->tlas())
            .commit();

    }

    std::string LightingPass::_select_fragment_shader() const
    {
        return m_params.ambient_occlusion
            ? "rg_lighting_pass_ao.frag.spv"
            : "rg_lighting_pass.frag.spv";
    }
}
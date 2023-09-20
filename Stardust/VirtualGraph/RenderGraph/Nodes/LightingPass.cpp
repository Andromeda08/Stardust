#include "LightingPass.hpp"
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
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
        { "Albedo Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Image", ResourceRole::eInput, ResourceType::eDepthImage },
        { "AO Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32Sfloat },
        { "AA Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR16G16B16A16Sfloat },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "Lighting Result", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
    #pragma endregion

    std::string to_string(LightingPassShadowMode shadow_mode)
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

    LightingPass::LightingPass(const sdvk::Context& context)
    : Node("Lighting Pass", NodeType::eLightingPass)
    , m_context(context)
    {
    }

    void LightingPass::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = sd::Application::s_current_frame;

        auto render_commands = [&](const vk::CommandBuffer& cmd){
            LightingPassPushConstant push_constant(m_options);
            push_constant.light_pos = { -12, 10, 5, 1 };

            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstant), &push_constant);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout, 0, 1,
                                   &m_renderer.descriptor->set(current_frame),
                                   0, nullptr);

            cmd.draw(3, 1, 0, 0);
        };

        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*m_resources["Albedo Image"]).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*m_resources["Depth Image"]).get_depth_image();
        auto ao = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();

        Nebula::Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao, ao->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);

        _update_descriptor(current_frame);

        auto framebuffer = m_renderer.framebuffers->get(current_frame);
        sdvk::RenderPass::Execute()
            .with_clear_value(m_renderer.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, render_commands);

        Nebula::Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao, ao->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
    }

    void LightingPass::initialize()
    {
        const auto& r_lighting_result = m_resources["Lighting Result"];
        auto lighting_result = dynamic_cast<ImageResource&>(*r_lighting_result).get_image();

        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_renderer.clear_values[0].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

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

        m_renderer.descriptor = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(2, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(3, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(4, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(5, vk::ShaderStageFlagBits::eFragment)
            .acceleration_structure(6, vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto pipeline = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(1)
            .add_shader("rg_lighting_pass.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_lighting_pass.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("LightingPass")
            .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline.pipeline;
        m_renderer.pipeline_layout = pipeline.pipeline_layout;

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
        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*m_resources["Albedo Image"]).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*m_resources["Depth Image"]).get_depth_image();
        auto ao = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();

        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        auto camera_data = camera.uniform_data();
        m_renderer.uniform[current_frame]->set_data(&camera_data, m_context.device());

        auto& tlas = dynamic_cast<TlasResource&>(*m_resources["TLAS"]).get_tlas();

        LightingPassUniform uniform_data {};
        uniform_data.view = camera_data.view;
        uniform_data.proj = camera_data.proj;
        uniform_data.view_inverse = camera_data.view_inverse;
        uniform_data.proj_inverse = camera_data.proj_inverse;
        uniform_data.eye = camera_data.eye;

        vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &tlas->tlas() };
        vk::DescriptorImageInfo position_info { m_renderer.samplers[0],position->image_view(),position->state().layout };
        vk::DescriptorImageInfo normal_info { m_renderer.samplers[1],normal->image_view(),normal->state().layout };
        vk::DescriptorImageInfo albedo_info { m_renderer.samplers[2], albedo->image_view(), albedo->state().layout };
        vk::DescriptorImageInfo depth_info { m_renderer.samplers[3], depth->image_view(), depth->state().layout };
        vk::DescriptorImageInfo ao_info { m_renderer.samplers[4], ao->image_view(), ao->state().layout };
        vk::DescriptorBufferInfo un_info { m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform) };

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform))
            .combined_image_sampler(1, position_info)
            .combined_image_sampler(2, normal_info)
            .combined_image_sampler(3, albedo_info)
            .combined_image_sampler(4, depth_info)
            .combined_image_sampler(5, ao_info)
            .acceleration_structure(6, 1, &tlas->tlas())
            .commit();
    }
}
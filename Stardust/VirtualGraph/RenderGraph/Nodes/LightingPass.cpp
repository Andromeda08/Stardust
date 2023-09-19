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
        { "G-Buffer", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Albedo Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "AO Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR32Sfloat },
        { "AA Image", ResourceRole::eInput, ResourceType::eImage, vk::Format::eR16G16B16A16Sfloat },
        { "Depth Image", ResourceRole::eInput, ResourceType::eDepthImage },
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

            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstant), &push_constant);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout, 0, 1,
                                   &m_renderer.descriptor->set(current_frame),
                                   0, nullptr);

            cmd.draw(3, 1, 0, 0);
        };

        auto gbuffer = dynamic_cast<ImageResource&>(*m_resources["G-Buffer"]).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*m_resources["Albedo Image"]).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*m_resources["Depth Image"]).get_depth_image();

        Nebula::Sync::ImageBarrier(gbuffer, gbuffer->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);

        _update_descriptor(current_frame);

        auto framebuffer = m_renderer.framebuffers->get(current_frame);
        sdvk::RenderPass::Execute()
            .with_clear_value(m_renderer.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_renderer.render_resolution})
            .with_render_pass(m_renderer.render_pass)
            .execute(command_buffer, render_commands);

        Nebula::Sync::ImageBarrier(gbuffer, gbuffer->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(albedo, albedo->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(depth, depth->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
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
            .acceleration_structure(4, vk::ShaderStageFlagBits::eFragment)
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

        m_renderer.samplers.resize(3);
        for (vk::Sampler& sampler : m_renderer.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }
    }

    void LightingPass::_update_descriptor(uint32_t current_frame)
    {
        auto gbuffer = dynamic_cast<ImageResource&>(*m_resources["G-Buffer"]).get_image();
        auto albedo = dynamic_cast<ImageResource&>(*m_resources["Albedo Image"]).get_image();
        auto depth = dynamic_cast<DepthImageResource&>(*m_resources["Depth Image"]).get_depth_image();

        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        auto camera_data = camera.uniform_data();
        m_renderer.uniform[current_frame]->set_data(&camera_data, m_context.device());

        auto& tlas = dynamic_cast<TlasResource&>(*m_resources["TLAS"]).get_tlas();

        LightingPassUniform uniform_data {};
        uniform_data.camera_data = camera_data;

        vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &tlas->tlas() };
        vk::DescriptorImageInfo gbuffer_info { m_renderer.samplers[0],gbuffer->image_view(),gbuffer->state().layout };
        vk::DescriptorImageInfo albedo_info { m_renderer.samplers[0], albedo->image_view(), albedo->state().layout };
        vk::DescriptorImageInfo depth_info { m_renderer.samplers[0], depth->image_view(), depth->state().layout };
        vk::DescriptorBufferInfo un_info { m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform) };

        m_renderer.descriptor->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform[current_frame]->buffer(), 0, sizeof(LightingPassUniform))
            .combined_image_sampler(1, gbuffer_info)
            .combined_image_sampler(2, albedo_info)
            .combined_image_sampler(3, depth_info)
            .acceleration_structure(4, 1, &tlas->tlas())
            .commit();
    }
}
#include "ScreenSpaceAO.hpp"

#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Sampler.hpp>

namespace Nebula::RenderGraph
{
    ScreenSpaceAO::ScreenSpaceAO(const sdvk::Context& context,
                                 std::map<std::string, std::shared_ptr<Resource>>& resources)
    : AmbientOcclusionStrategy(context, resources)
    {
        m_random_floats = std::uniform_real_distribution<float>(0.0, 1.0);
    }

    void ScreenSpaceAO::execute(const vk::CommandBuffer& command_buffer)
    {
        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto ao_buffer = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();

        Nebula::Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eShaderReadOnlyOptimal).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao_buffer, ao_buffer->state().layout, vk::ImageLayout::eColorAttachmentOptimal).apply(command_buffer);

        uint32_t current_frame = sd::Application::s_current_frame;
        _update_descriptor(current_frame);
        auto framebuffer = m_kernel.framebuffers->get(current_frame);

        sdvk::RenderPass::Execute()
            .with_clear_values<1>(m_kernel.clear_values)
            .with_framebuffer(framebuffer)
            .with_render_area({{ 0, 0 }, m_kernel.render_resolution})
            .with_render_pass(m_kernel.render_pass)
            .execute(command_buffer, [&](const vk::CommandBuffer& cmd){
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_kernel.pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       m_kernel.pipeline_layout, 0, 1,
                                       &m_kernel.descriptor->set(current_frame),
                                       0, nullptr);
                cmd.draw(3, 1, 0, 0);
            });

        Nebula::Sync::ImageBarrier(position, position->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao_buffer, ao_buffer->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
    }

    void ScreenSpaceAO::initialize(const AmbientOcclusionOptions& options)
    {
        m_options = ScreenSpaceAOOptions(options);

        for (int32_t i = 0; i < m_options.sample_count; i++)
        {
            glm::vec4 sample = {
                m_random_floats(m_random) * 2.0f - 1.0f,
                m_random_floats(m_random) * 2.0f - 1.0f,
                m_random_floats(m_random),
                1.0f
            };

            float scale = (float) i / (float) m_options.sample_count;
            scale = std::lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            m_kernel.samples.push_back(sample);
        }

        for (int i = 0; i < m_options.noise_count; ++i) {
            glm::vec4 noise = {
                m_random_floats(m_random) * 2.0f - 1.0f,
                m_random_floats(m_random) * 2.0f - 1.0f,
                0.0f,
                1.0f
            };

            m_kernel.noise.push_back(noise);
        }

        const auto& r_ao_buffer = m_resources["AO Image"];
        auto ao_buffer = dynamic_cast<ImageResource&>(*r_ao_buffer).get_image();

        m_kernel.render_resolution = sd::Application::s_extent.vk_ext();
        m_kernel.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_kernel.clear_values[0].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

        m_kernel.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(ao_buffer->properties().format)
            .make_subpass()
            .create(m_context);

        m_kernel.framebuffers = Framebuffer::Builder()
            .add_attachment(ao_buffer->image_view())
            .set_render_pass(m_kernel.render_pass)
            .set_size(m_kernel.render_resolution)
            .set_count(m_kernel.frames_in_flight)
            .set_name("ScreenSpace AO Framebuffer")
            .create(m_context);

        m_kernel.descriptor = Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eFragment)
            .uniform_buffer(1, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(2, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(3, vk::ShaderStageFlagBits::eFragment)
            .create(m_kernel.frames_in_flight, m_context);

        auto pipeline = sdvk::PipelineBuilder(m_context)
            .add_descriptor_set_layout(m_kernel.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(1)
            .add_shader("rg_ssao.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rg_ssao.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("ScreenSpace AO")
            .create_graphics_pipeline(m_kernel.render_pass);

        m_kernel.pipeline = pipeline.pipeline;
        m_kernel.pipeline_layout = pipeline.pipeline_layout;

        m_kernel.uniform_camera.resize(m_kernel.frames_in_flight);
        for (auto& ub : m_kernel.uniform_camera)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(sd::CameraUniformData))
                .as_uniform_buffer()
                .create(m_context);
        }

        m_kernel.uniform_ssao.resize(m_kernel.frames_in_flight);
        for (auto& ub : m_kernel.uniform_ssao)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(ScreenSpaceAOUniform))
                .as_uniform_buffer()
                .create(m_context);
        }

        m_kernel.samplers.resize(2);
        for (vk::Sampler& sampler : m_kernel.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }
    }

    void ScreenSpaceAO::_update_descriptor(uint32_t current_frame)
    {
        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();

        auto camera = *(dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera());
        auto camera_data = camera.uniform_data();
        m_kernel.uniform_camera[current_frame]->set_data(&camera_data, m_context.device());

        ScreenSpaceAOUniform ssao_data(m_options);
        int32_t sample_count = (m_options.sample_count > 64) ? 64 : m_options.sample_count;
        for (int32_t i = 0; i < sample_count; i++)
        {
            ssao_data.samples[i] = m_kernel.samples[i];
        }
        for (int32_t i = 0; i < m_options.noise_count; i++)
        {
            ssao_data.noise[i] = m_kernel.noise[i];
        }
        m_kernel.uniform_ssao[current_frame]->set_data(&ssao_data, m_context.device());

        vk::DescriptorImageInfo position_info { m_kernel.samplers[0],position->image_view(),position->state().layout };
        vk::DescriptorImageInfo normal_info { m_kernel.samplers[1],normal->image_view(),normal->state().layout };
        vk::DescriptorBufferInfo camera_info { m_kernel.uniform_camera[current_frame]->buffer(), 0, sizeof(sd::CameraUniformData) };
        vk::DescriptorBufferInfo ssao_info { m_kernel.uniform_ssao[current_frame]->buffer(), 0, sizeof(ScreenSpaceAOUniform) };

        m_kernel.descriptor->begin_write(current_frame)
            .uniform_buffer(0, camera_info)
            .uniform_buffer(1, ssao_info)
            .combined_image_sampler(2, position_info)
            .combined_image_sampler(3, normal_info)
            .commit();
    }
}
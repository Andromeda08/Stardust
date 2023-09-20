#include "RayTracedAO.hpp"

#include <glm/ext/matrix_relational.hpp>
#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Sampler.hpp>

namespace Nebula::RenderGraph
{
    RayTracedAO::RayTracedAO(const sdvk::Context& context, std::map<std::string, std::shared_ptr<Resource>>& resources)
    : AmbientOcclusionStrategy(context, resources)
    {
    }

    void RayTracedAO::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = sd::Application::s_current_frame;

        auto camera = *dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera();

        // AO accumulation while camera is stationary.
        auto view_mat = camera.view();
        auto eq = glm::equal(glm::mat4(m_ref_mat), view_mat, 0.001f);
        if (!(eq.x && eq.y && eq.z && eq.w))
        {
            m_ref_mat = view_mat;
            m_kernel.frame = -1;
        }
        m_kernel.frame++;

        m_options.cur_samples = m_kernel.frame;

        if (m_kernel.frame * m_options.cur_samples > m_options.max_samples)
        {
            return;
        }

        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto ao = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();

        // Calculate grou sizes from AO buffer extent.
        auto size = ao->properties().extent;
        auto group_x = (size.width + (Kernel::s_group_size - 1)) / Kernel::s_group_size;
        auto group_y = (size.height + (Kernel::s_group_size - 1)) / Kernel::s_group_size;

        RayTracedAOPushConsant pc(m_options);
        pc.cur_samples = m_options.cur_samples;

        Nebula::Sync::ImageBarrier(position, position->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao, ao->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);

        _update_descriptor(current_frame);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_kernel.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_kernel.pipeline_layout, 0, 1, &m_kernel.descriptor->set(current_frame), 0, nullptr);
        command_buffer.pushConstants(m_kernel.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(RayTracedAOPushConsant), &pc);
        command_buffer.dispatch(group_x, group_y, 1);

        Nebula::Sync::ImageBarrier(position, position->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(normal, normal->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);
        Nebula::Sync::ImageBarrier(ao, ao->state().layout,vk::ImageLayout::eGeneral).apply(command_buffer);
    }

    void RayTracedAO::initialize(const AmbientOcclusionOptions& options)
    {
        m_kernel.render_resolution = sd::Application::s_extent.vk_ext();
        m_kernel.frames_in_flight = sd::Application::s_max_frames_in_flight;

        m_kernel.samplers.resize(3);
        for (vk::Sampler& sampler : m_kernel.samplers)
        {
            sampler = sdvk::SamplerBuilder().create(m_context.device());
        }

        m_kernel.descriptor = Descriptor::Builder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .storage_image(2, vk::ShaderStageFlagBits::eCompute)
            .acceleration_structure(3, vk::ShaderStageFlagBits::eCompute)
            .create(m_kernel.frames_in_flight, m_context);

        auto pl = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(RayTracedAOOptions) })
            .add_descriptor_set_layout(m_kernel.descriptor->layout())
            .create_pipeline_layout()
            .add_shader("rg_rtao.comp.spv", vk::ShaderStageFlagBits::eCompute)
            .with_name("RayTracing AO")
            .create_compute_pipeline();

        m_kernel.pipeline = pl.pipeline;
        m_kernel.pipeline_layout = pl.pipeline_layout;
    }

    void RayTracedAO::_update_descriptor(uint32_t current_frame)
    {
        auto position = dynamic_cast<ImageResource&>(*m_resources["Position Buffer"]).get_image();
        auto normal = dynamic_cast<ImageResource&>(*m_resources["Normal Buffer"]).get_image();
        auto ao = dynamic_cast<ImageResource&>(*m_resources["AO Image"]).get_image();
        auto tlas = dynamic_cast<TlasResource&>(*m_resources["TLAS"]).get_tlas();

        vk::DescriptorImageInfo position_info { m_kernel.samplers[0], position->image_view(), position->state().layout };
        vk::DescriptorImageInfo normal_info { m_kernel.samplers[1], normal->image_view(), normal->state().layout };
        vk::DescriptorImageInfo ao_info { m_kernel.samplers[2], ao->image_view(), ao->state().layout };

        m_kernel.descriptor->begin_write(current_frame)
            .storage_image(0, position_info)
            .storage_image(1, normal_info)
            .storage_image(2, ao_info)
            .acceleration_structure(3, 1, &tlas->tlas())
            .commit();
    }
}
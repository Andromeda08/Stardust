#include "RayTracingNode.hpp"

#include <Application/Application.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Sampler.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>

namespace Nebula::RenderGraph
{
    #pragma region Resource Specifications
    const std::vector<ResourceSpecification> RayTracingNode::s_resource_specs = {
        { "Object Descriptions", ResourceRole::eInput, ResourceType::eBuffer },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "Output", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
    };
    #pragma endregion

    RayTracingNode::RayTracingNode(const sdvk::Context& context)
    : Node("RayTracingNode", NodeType::eRayTracing)
    , m_context(context)
    {
    }

    void RayTracingNode::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = sd::Application::s_current_frame;

        auto output_image = dynamic_cast<ImageResource&>(*m_resources["Output"]).get_image();
        Nebula::Sync::ImageBarrier(output_image, output_image->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);

        update_descriptor(current_frame);

        auto& sbt = *m_renderer.sbt;

        command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_renderer.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_renderer.pipeline_layout, 0, 1, &m_renderer.descriptor->set(current_frame), 0, nullptr);
        command_buffer.traceRaysKHR(
            sbt.rgen_region(),
            sbt.miss_region(),
            sbt.hit_region(),
            sbt.call_region(),
            m_renderer.render_resolution.width,
            m_renderer.render_resolution.height,
            2);
    }

    void RayTracingNode::initialize()
    {
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;
        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();

        m_renderer.descriptor = Descriptor::Builder()
            .acceleration_structure(0, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
            .storage_image(1, vk::ShaderStageFlagBits::eRaygenKHR)
            .uniform_buffer(2, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
            .storage_buffer(3, vk::ShaderStageFlagBits::eClosestHitKHR)
            .create(m_renderer.frames_in_flight, m_context);

        auto [pipeline, pipeline_layout] = sdvk::PipelineBuilder(m_context)
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .add_shader("rt_test.rgen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
            .add_shader("rt_test.rmiss.spv", vk::ShaderStageFlagBits::eMissKHR)
            .add_shader("rt_test_shadow.rmiss.spv", vk::ShaderStageFlagBits::eMissKHR)
            .add_shader("rt_test.rchit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
            .make_rt_shader_groups()
            .create_ray_tracing_pipeline(1);

        m_renderer.pipeline = pipeline;
        m_renderer.pipeline_layout = pipeline_layout;

        m_renderer.sbt = std::make_shared<sd::rt::ShaderBindingTable>(2, 1, m_renderer.pipeline, m_context);

        m_renderer.uniform.resize(m_renderer.frames_in_flight);
        for (auto& ub : m_renderer.uniform)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(sd::CameraUniformData))
                .as_uniform_buffer()
                .create(m_context);
        }

        m_renderer.sampler = sdvk::SamplerBuilder().create(m_context.device());
    }

    Image& RayTracingNode::get_output()
    {
        return *(dynamic_cast<ImageResource&>(*m_resources["Output"]).get_image());
    }

    void RayTracingNode::update_descriptor(uint32_t index)
    {
        auto& output_image = get_output();
        auto& camera = dynamic_cast<CameraResource&>(*m_resources["Camera"]).get_camera();
        auto& tlas = dynamic_cast<TlasResource&>(*m_resources["TLAS"]).get_tlas();
        auto& obj_buffer = dynamic_cast<BufferResource&>(*m_resources["Object Descriptions"]).get_buffer();

        auto camera_data = camera->uniform_data();
        m_renderer.uniform[index]->set_data(&camera_data, m_context.device());

        vk::DescriptorImageInfo output_image_info { m_renderer.sampler, output_image.image_view(), output_image.state().layout };

        m_renderer.descriptor->begin_write(index)
            .acceleration_structure(0, 1, &tlas->tlas())
            .storage_image(1, output_image_info)
            .uniform_buffer(2, m_renderer.uniform[index]->buffer(), 0, sizeof(sd::CameraUniformData))
            .storage_buffer(3, obj_buffer->buffer(), 0, obj_buffer->size())
            .commit();
    }
}
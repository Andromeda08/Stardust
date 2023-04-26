#include "OffscreenRenderNode.hpp"

#include <RenderGraph/res/pcObject.hpp>
#include <RenderGraph/res/CameraResource.hpp>
#include <RenderGraph/res/ObjectsResource.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Image/DepthBuffer.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>

namespace sd::rg
{

    OffscreenRenderNode::OffscreenRenderNode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers)
    : m_context(context)
    {
        _init_inputs();
        _init_outputs(context);
    }

    void OffscreenRenderNode::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = Application::s_current_frame;

        _update_descriptors(current_frame);
        auto& objects = (dynamic_cast<ObjectsResource&>(*m_inputs[0])).get_objects();

        pcNodeParams node_params(m_parameters);
        pcObject     object_params {};

        auto render_commands = [&](const vk::CommandBuffer& cmd){
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout, 0, 1,
                                   &m_renderer.descriptor2->set(current_frame),
                                   0, nullptr);

            for (const auto& obj : objects)
            {
                // Update per object push constant
                object_params.model_matrix = obj.transform.model();
                object_params.color = obj.color;

                cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pcObject), &object_params);

                obj.mesh->draw(cmd);
            }
        };

        sdvk::RenderPass::Execute()
            .with_clear_values(m_renderer.clear_values)
            .with_framebuffer(m_renderer.framebuffers[current_frame])
            .with_render_area({{ 0, 0 }, m_parameters.resolution})
            .with_render_pass(m_renderer.renderpass)
            .execute(command_buffer, render_commands);
    }

    void OffscreenRenderNode::compile()
    {
        _init_renderer(m_context);
        for (int32_t i = 0; i < Application::s_max_frames_in_flight; i++)
        {
            _update_descriptors(i);
        }
    }

    void OffscreenRenderNode::_init_inputs()
    {
        m_inputs.resize(3);

        m_inputs[0] = std::make_unique<ObjectsResource>();
        m_inputs[1] = std::make_unique<CameraResource>();
        m_inputs[2] = std::make_unique<AccelerationStructureResource>();
    }

    void OffscreenRenderNode::_init_outputs(const sdvk::Context& context)
    {
        const auto format = vk::Format::eR32G32B32A32Sfloat;
        const auto usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
        const auto samples = vk::SampleCountFlagBits::e1;
        const auto memory = vk::MemoryPropertyFlagBits::eDeviceLocal;

        m_outputs.resize(3);

        #pragma region create images
        std::shared_ptr<sdvk::Image> render_img = sdvk::Image::Builder()
                .with_extent(m_parameters.resolution)
                .with_format(format)
                .with_sample_count(samples)
                .with_usage_flags(usage)
                .with_memory_property_flags(memory)
                .create(context);
        std::shared_ptr<sdvk::Image> g_buffer = sdvk::Image::Builder()
                .with_extent(m_parameters.resolution)
                .with_format(format)
                .with_sample_count(samples)
                .with_usage_flags(usage)
                .with_memory_property_flags(memory)
                .create(context);

        std::shared_ptr<sdvk::Image> depth_img = std::make_unique<sdvk::DepthBuffer>(m_parameters.resolution, vk::SampleCountFlagBits::e1, m_context);
        #pragma endregion

        m_outputs[0] = ImageResource::Builder().create_from_resource(render_img);
        m_outputs[1] = ImageResource::Builder().create_from_resource(g_buffer);
        m_outputs[2] = ImageResource::Builder().create_from_resource(depth_img);
    }

    void OffscreenRenderNode::_init_renderer(const sdvk::Context& context)
    {
        auto& render_buffer = *(dynamic_cast<ImageResource&>(*m_outputs[0])).m_resource;
        auto& g_buffer = *(dynamic_cast<ImageResource&>(*m_outputs[1])).m_resource;
        auto& depth_buffer = *(dynamic_cast<ImageResource&>(*m_outputs[2])).m_resource;

        auto& r = m_renderer;

        r.renderpass = sdvk::RenderPass::Builder()
                .add_color_attachment(render_buffer.format())
                .add_color_attachment(g_buffer.format())
                .set_depth_attachment(depth_buffer.format())
                .make_subpass(vk::PipelineBindPoint::eGraphics)
                .create(context);

        std::array<vk::ImageView, 3> attachments = { render_buffer.view(), g_buffer.view(), depth_buffer.view() };

        vk::FramebufferCreateInfo framebuffer_create_info;
        #pragma region structure data
        framebuffer_create_info.setRenderPass(r.renderpass);
        framebuffer_create_info.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
        framebuffer_create_info.setPAttachments(attachments.data());
        // TODO: static resolution size from app or whatever
        framebuffer_create_info.setWidth(m_parameters.resolution.width);
        framebuffer_create_info.setHeight(m_parameters.resolution.height);
        framebuffer_create_info.setLayers(1);
        #pragma endregion

        for (auto& framebuffer : m_renderer.framebuffers)
        {
            auto result = context.device().createFramebuffer(&framebuffer_create_info, nullptr, &framebuffer);
        }

        r.descriptor = sdvk::DescriptorBuilder()
                .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .accelerator(1, vk::ShaderStageFlagBits::eFragment)
                .with_name("OSR Node")
                .create(context.device(), 2);

        r.descriptor2 = sdvk::Descriptor2<n_frames_in_flight>::Builder()
                .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .acceleration_structure(1, vk::ShaderStageFlagBits::eFragment)
                .with_name("OSR Node v2")
                .create(context);

        auto pipeline_result = sdvk::PipelineBuilder(context)
                .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(pcObject) })
                .add_descriptor_set_layout(r.descriptor2->layout())
                .create_pipeline_layout()
                .set_sample_count(vk::SampleCountFlagBits::e1)
                .set_attachment_count(2)
                .add_attribute_descriptions({ VertexData::attribute_descriptions() })
                .add_binding_descriptions({ VertexData::binding_description() })
                .add_shader(s_vertex_shader.data(), vk::ShaderStageFlagBits::eVertex)
                .add_shader(s_fragment_shader.data(), vk::ShaderStageFlagBits::eFragment)
                .with_name("OSR Node")
                .create_graphics_pipeline(r.renderpass);

        r.pipeline = pipeline_result.pipeline;
        r.pipeline_layout = pipeline_result.pipeline_layout;

        for (auto& u : r.uniform_camera)
        {
            u = sdvk::Buffer::Builder()
                    .with_size(sizeof(CameraUniformData))
                    .as_uniform_buffer()
                    .create(context);
        }

        r.clear_values[0].setColor(m_parameters.clear_color);
        r.clear_values[1].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        r.clear_values[2].setDepthStencil({ 1.0f, 0 });
    }

    void OffscreenRenderNode::_update_descriptors(uint32_t current_frame)
    {
        auto resource = dynamic_cast<CameraResource&>(*m_inputs[1]);
        auto camera = *resource.m_resource;
        auto camera_data = camera.uniform_data();

        m_renderer.uniform_camera[current_frame]->set_data(&camera_data, m_context.device());

        auto& tlas = dynamic_cast<AccelerationStructureResource&>(*m_inputs[2]);

        vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &tlas.m_resource->tlas() };
        vk::DescriptorBufferInfo un_info { m_renderer.uniform_camera[current_frame]->buffer(), 0, sizeof(CameraUniformData) };

        m_renderer.descriptor2->begin_write(current_frame)
            .uniform_buffer(0, m_renderer.uniform_camera[current_frame]->buffer(), 0, sizeof(CameraUniformData))
            .acceleration_structure(1, 1, &tlas.m_resource->tlas())
            .commit();

        sdvk::DescriptorWrites(m_context.device(), *m_renderer.descriptor)
                .uniform_buffer(current_frame, 0, un_info)
                .acceleration_structure(current_frame, 1, as_info)
                .commit();
    }

}
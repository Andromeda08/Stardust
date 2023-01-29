#include "Scene.hpp"

#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>

namespace sd
{

    Scene::Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context, const sdvk::Swapchain& swapchain)
    : m_command_buffers(command_buffers), m_context(context), m_swapchain(swapchain)
    {
        auto view = glm::lookAt(glm::vec3{5.0f, 5.0f, 5.0f}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
        auto proj = glm::perspective(glm::radians(45.0f), m_swapchain.aspect_ratio(), 0.1f, 1000.0f);
        CameraUniformData cud
        {
            .view = view,
            .proj = proj,
            .view_inverse = glm::inverse(view),
            .proj_inverse = glm::inverse(proj),
            .eye = {5.0f, 5.0f, 5.0f, 1.0f}
        };

        m_descriptor = sdvk::DescriptorBuilder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .create(context.device(), m_swapchain.image_count());

        m_uniform_camera.resize(m_swapchain.image_count());
        for (int32_t i = 0; i < m_swapchain.image_count(); i++)
        {
            m_uniform_camera[i] = sdvk::Buffer::Builder()
                .with_size(sizeof(CameraUniformData))
                .as_uniform_buffer()
                .create(context);

            m_uniform_camera[i]->set_data(&cud, context.device());

            vk::DescriptorBufferInfo info = { m_uniform_camera[i]->buffer(), 0, sizeof(CameraUniformData) };
            sdvk::DescriptorWrites(context.device(), *m_descriptor)
                .uniform_buffer(i, 0, info)
                .commit();
        }

        m_rendering.depth_buffer = std::make_unique<sdvk::DepthBuffer>(m_swapchain.extent(), m_context);
        m_rendering.render_pass = std::make_unique<sdvk::RenderPass>(m_swapchain.format(), m_rendering.depth_buffer->format(), m_context);
        m_rendering.clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        m_rendering.clear_values[1].setDepthStencil({ 1.0f, 0 });

        std::array<vk::ImageView, 2> attachments = { m_swapchain.view(0), m_rendering.depth_buffer->view() };
        vk::FramebufferCreateInfo create_info;
        create_info.setRenderPass(m_rendering.render_pass->handle());
        create_info.setAttachmentCount(attachments.size());
        create_info.setPAttachments(attachments.data());
        create_info.setWidth(m_swapchain.extent().width);
        create_info.setHeight(m_swapchain.extent().height);
        create_info.setLayers(1);

        for (size_t i = 0; i < 2; i++)
        {
            attachments[0] = m_swapchain.view(i);
            auto result = m_context.device().createFramebuffer(&create_info, nullptr, &m_rendering.framebuffers[i]);
        }

        m_pipelines["default"] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData) })
            .add_descriptor_set_layout(m_descriptor->layout())
            .create_pipeline_layout()
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader("base.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("base.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*m_rendering.render_pass);

        std::string cube = "cube", sphere = "sphere";
        m_meshes[cube] = std::make_shared<sdvk::Mesh>(new primitives::Cube(), m_command_buffers, m_context, cube);
        m_meshes[sphere] = std::make_shared<sdvk::Mesh>(new primitives::Sphere(), m_command_buffers, m_context, sphere);

        Object obj_cube;
        obj_cube.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[cube]);
        obj_cube.transform.position = {2, 0, 0};
        obj_cube.color = { 0.8f, 0.1f, 0.9f, 1.0f };

        Object obj_sphere;
        obj_sphere.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[sphere]);
        obj_sphere.transform.position = {-2, 0, 0};
        obj_sphere.color = { 0.0f, 0.7f, 0.9f, 1.0f };

        m_objects.push_back(obj_cube);
        m_objects.push_back(obj_sphere);

        if (m_context.raytracing())
        {
            m_tlas = sdvk::Tlas::Builder().with_name("[TLAS] Scene").create(m_objects, m_command_buffers, m_context);
        }
    }

    void Scene::rasterize(uint32_t current_frame, const vk::CommandBuffer& cmd)
    {
        vk::RenderPassBeginInfo begin_info;
        begin_info.setRenderPass(m_rendering.render_pass->handle());
        begin_info.setRenderArea({{0,0}, m_swapchain.extent()});
        begin_info.setClearValueCount(m_rendering.clear_values.size());
        begin_info.setPClearValues(m_rendering.clear_values.data());
        begin_info.setFramebuffer(m_rendering.framebuffers[current_frame]);

        auto viewport = m_swapchain.make_viewport();
        auto scissor = m_swapchain.make_scissor();
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        cmd.beginRenderPass(&begin_info, vk::SubpassContents::eInline);

        for (const auto& obj : m_objects)
        {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipelines[obj.pipeline].pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelines[obj.pipeline].pipeline_layout, 0, 1, &m_descriptor->set(current_frame), 0, nullptr);

            Object::PushConstantData obj_pcd;
            obj_pcd.model = obj.transform.model();
            obj_pcd.color = obj.color;

            cmd.pushConstants(m_pipelines[obj.pipeline].pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData), &obj_pcd);

            obj.mesh->draw(cmd);
        }

        cmd.endRenderPass();
    }
}
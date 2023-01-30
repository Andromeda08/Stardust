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
        {
            auto scext = swapchain.extent();
            m_camera = std::make_unique<Camera>(glm::ivec2(scext.width, scext.height), glm::vec3(5.0f));
        }

        m_descriptor = sdvk::DescriptorBuilder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .accelerator(1, vk::ShaderStageFlagBits::eFragment)
            .create(context.device(), m_swapchain.image_count());

        auto cud = m_camera->uniform_data();
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
            .add_shader("rq_light.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rq_light.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*m_rendering.render_pass);

        std::string cube = "cube", sphere = "sphere";
        m_meshes[cube] = std::make_shared<sdvk::Mesh>(new primitives::Cube(), m_command_buffers, m_context, cube);
        m_meshes[sphere] = std::make_shared<sdvk::Mesh>(new primitives::Sphere(), m_command_buffers, m_context, sphere);

        Object obj_cube;
        obj_cube.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[cube]);
        obj_cube.transform.position = {2, 0.5f, 0};
        obj_cube.color = { 0.8f, 0.1f, 0.9f, 1.0f };

        Object obj_cube2;
        obj_cube2.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[cube]);
        obj_cube2.transform.position = {-5, 0.5f, -5};
        obj_cube2.color = { 0.4f, 0.1f, 0.9f, 1.0f };

        Object obj_cube3;
        obj_cube3.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[cube]);
        obj_cube3.transform.position = {4, 0.5f, 2};
        obj_cube3.transform.scale = {2, 2, 2};
        obj_cube3.color = { 0.1f, 1.0f, 0.2f, 1.0f };

        Object obj_plane;
        obj_plane.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[cube]);
        obj_plane.transform.scale = { 25, 0.05f, 25 };
        obj_plane.color = { 0.5f, 0.5f, 0.5f, 1.0f };

        Object obj_sphere;
        obj_sphere.mesh = std::shared_ptr<sdvk::Mesh>(m_meshes[sphere]);
        obj_sphere.transform.position = {-2, 1.0f, 0};
        obj_sphere.color = { 0.0f, 0.7f, 0.9f, 1.0f };

        m_objects.push_back(obj_plane);
        m_objects.push_back(obj_cube);
        m_objects.push_back(obj_cube2);
        m_objects.push_back(obj_cube3);
        m_objects.push_back(obj_sphere);

        if (m_context.raytracing())
        {
            m_tlas = sdvk::Tlas::Builder().with_name("[TLAS] Scene").create(m_objects, m_command_buffers, m_context);
            vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &m_tlas->tlas() };
            for (int32_t i = 0; i < m_swapchain.image_count(); i++)
            {
                sdvk::DescriptorWrites(m_context.device(), *m_descriptor).acceleration_structure(i, 1, as_info).commit();
            }
        }
    }

    void Scene::rasterize(uint32_t current_frame, const vk::CommandBuffer& cmd)
    {
        auto cud = m_camera->uniform_data();
        m_uniform_camera[current_frame]->set_data(&cud, m_context.device());

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

    void Scene::register_keybinds(GLFWwindow* p_window)
    {
        m_camera->register_keys(p_window);
    }
}
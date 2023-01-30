#include "Scene.hpp"

namespace sd
{

    Scene::Scene(Swapchain &swapchain, const CommandBuffers &command_buffers)
    : m_swapchain(swapchain), m_command_buffers(command_buffers), m_device(command_buffers.device())
    {
        gen_defaults();
    }

    void Scene::rasterize(uint32_t frame, vk::CommandBuffer cmd)
    {
        update_camera(frame);
        auto viewport = m_swapchain.make_viewport();
        auto scissor = m_swapchain.make_scissor();
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        render_pass(m_render_pass->handle(), frame, cmd, [&](){
            auto pc_data = PushConstantData {
                .time = { m_clock->ms(), 0, 0, 0 },
                .model = glm::mat4(1.0f),
                .color = glm::vec4(0.5f)
            };

            for (const auto& obj : m_objects)
            {
                auto mat = obj.material;
                pc_data.model = obj.transform;
                pc_data.color = obj.color;

                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mat.pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mat.pipeline_layout, 0, 1, &m_descriptors.sets->operator[](frame), 0, nullptr);
                cmd.pushConstants(mat.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &pc_data);

                obj.mesh.draw(cmd);
            }
        });
    }

    void Scene::trace_rays(uint32_t frame, vk::CommandBuffer cmd)
    {
        throw std::runtime_error("This scene doesn't support ray tracing.");
    }

    void Scene::register_keybinds(GLFWwindow *window)
    {
        m_cameras[m_active_camera]->use_inputs(window);

        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        {
            std::cout << "Camera eye: " << glm::to_string(m_cameras[m_active_camera]->eye()) << std::endl;
        }
    }

    void Scene::add_object(const std::string &mesh_id, const std::string &mat_id, const std::string &name)
    {
        add_object(mesh_id, mat_id, glm::mat4(1.0f), glm::vec4(0.5f), name);
    }

    void Scene::add_object(const std::string &mesh_id, const std::string &mat_id, const glm::mat4 &model,
                           const glm::vec4 &color, const std::string &name)
    {
        Object obj = {
            .mesh = *m_meshes[mesh_id],
            .material = m_materials[mat_id],
            .transform = model,
            .color = color,
            .name = name
        };
        m_objects.push_back(obj);
    }

    void Scene::render_pass(vk::RenderPass render_pass, uint32_t frame, vk::CommandBuffer cmd,
                            const std::function<void()> &fun)
    {
        vk::RenderPassBeginInfo begin_info;
        begin_info.setRenderPass(render_pass);
        begin_info.setRenderArea({{ 0, 0 }, m_swapchain.extent()});
        begin_info.setClearValueCount(m_clear_values.size());
        begin_info.setPClearValues(m_clear_values.data());
        begin_info.setFramebuffer(m_swapchain.framebuffer(frame));
        cmd.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
        fun();
        cmd.endRenderPass();
    }

    void Scene::update_camera(uint32_t frame)
    {
        auto eye = m_cameras[m_active_camera]->eye();
        CameraUniformData data = {
            .view = m_cameras[m_active_camera]->view(),
            .proj = m_cameras[m_active_camera]->projection(),
            .view_inverse = glm::inverse(m_cameras[m_active_camera]->view()),
            .proj_inverse = glm::inverse(m_cameras[m_active_camera]->projection()),
            .eye = glm::vec4(eye.x, eye.y, eye.z, 0)
        };
        m_uniform_camera[frame]->update(&data);
    }

    void Scene::gen_defaults()
    {
        const uint32_t frames = m_swapchain.image_count();
        const auto vtx_frg_stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        m_clock = std::make_unique<Clock>();
        m_depth_buffer = std::make_unique<re::DepthBuffer>(m_swapchain.extent(), m_command_buffers);
        m_render_pass = std::make_shared<RenderPass>(m_device, m_swapchain.format(), m_depth_buffer->format());
        m_clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        m_clear_values[1].setDepthStencil({ 1.0f, 0 });
        m_swapchain.create_frame_buffers(*m_render_pass, *m_depth_buffer);

        m_cameras.resize(1);
        m_cameras[0] = std::make_unique<Camera>(m_swapchain.extent(), glm::vec3(5, 0, 5));

        m_descriptors.layout = DescriptorSetLayout()
            .uniform_buffer(Bindings::eCamera, vtx_frg_stages)
            .get_bindings(m_descriptors.bindings)
            .create(m_device);

        m_descriptors.sets = std::make_unique<DescriptorSets>(m_descriptors.bindings, m_descriptors.layout, m_device);

        m_uniform_camera.resize(frames);
        for (uint32_t i = 0; i < frames; i++)
        {
            m_uniform_camera[i] = std::make_unique<CameraUB>(m_command_buffers);
            update_camera(i);

            vk::DescriptorBufferInfo info = { m_uniform_camera[i]->buffer(), 0, sizeof(CameraUniformData) };
            DescriptorWrites(m_device, *m_descriptors.sets).uniform_buffer(i, Bindings::eCamera, info).commit();
        }

        auto pipeline = PipelineBuilder(m_device)
            .add_descriptor_set_layout(m_descriptors.layout)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData) })
            .create_pipeline_layout()
            .add_attribute_descriptions({ re::VertexData::attribute_descriptions() })
            .add_binding_descriptions({ re::VertexData::binding_description() })
            .add_shader("scene_default.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("scene_default.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*m_render_pass);

        m_materials.insert({ "default", pipeline });

        m_meshes["sphere"] = std::make_shared<re::Mesh>(new SphereGeometry(0.25f, glm::vec3(0.5f), 120), m_command_buffers);
        m_meshes["sphereL"] = std::make_shared<re::Mesh>(new SphereGeometry(1.0f, glm::vec3(0.5f), 120), m_command_buffers);
        m_meshes["cube"] = std::make_shared<re::Mesh>(new CubeGeometry(), m_command_buffers);
    }
}
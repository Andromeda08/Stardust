#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc51-cpp"
#include "Scene.hpp"

namespace re
{
    Scene::Scene(Swapchain &swapchain, const CommandBuffers& command_buffer)
    : m_command_buffers(command_buffer), m_device(command_buffer.device()), m_swapchain(swapchain)
    {
        m_rand     = std::default_random_engine(static_cast<unsigned>(time(nullptr)));
        m_camera3d = std::make_unique<Camera>(m_swapchain.extent(), glm::vec3 {10.0f, 0.0f, 10.0f});

        create_objects();
        build_default_rendering_pipeline({ "default.vert.spv", "default.frag.spv" });

        m_clear_values[0].setColor(std::array<float, 4>{ 30.0f / 255.0f, 30.0f / 255.0f, 46.0f / 255.0f, 1.0f });
        m_clear_values[1].setDepthStencil({ 1.0f, 0 });
        m_render_area = vk::Rect2D({ 0, 0 }, m_swapchain.extent());

        m_swapchain.create_frame_buffers(*m_render_pass, *m_depth_buffer);
    }

    void Scene::rasterize(size_t current_frame, vk::CommandBuffer cmd)
    {
        update_camera(current_frame);

        vk::RenderPassBeginInfo begin_info;
        begin_info.setRenderPass(m_render_pass->handle());
        begin_info.setRenderArea(m_render_area);
        begin_info.setClearValueCount(m_clear_values.size());
        begin_info.setPClearValues(m_clear_values.data());
        begin_info.setFramebuffer(m_swapchain.framebuffer(current_frame));

        auto viewport = m_swapchain.make_viewport();
        auto scissor = m_swapchain.make_scissor();

        cmd.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline_layout, 0, 1, &m_descriptors->get_set(current_frame), 0, nullptr);

        for (const auto& mesh : m_objects)
        {
            mesh->draw(cmd);
        }

        cmd.endRenderPass();
    }

    void Scene::create_objects()
    {
        m_objects.push_back(std::make_unique<re::Mesh>(new CubeGeometry(2.0f, { 0, 1, 1 }), m_command_buffers));

        std::vector<re::InstanceData> instance_data;

        std::uniform_int_distribution<int>    uniform_dist(-1 * 128, 128);
        std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
        std::uniform_real_distribution<float> scale_mod(1.0f, 4.0f);

        for (int i = 0; i < 128; i++)
        {
            auto x = (float) uniform_dist(m_rand);
            auto y = (float) uniform_dist(m_rand);
            auto z = (float) uniform_dist(m_rand);

            instance_data.push_back({
                .translate = glm::vec3{ x, y, z },
                .scale = glm::vec3{ scale_mod(m_rand) },
                .r_axis = glm::vec3{ 1 },
                .r_angle = 0.0f,
                .color = glm::vec4{ uniform_float(m_rand), uniform_float(m_rand), uniform_float(m_rand), 1.0f }
            });
        }

        m_instanced_objects.push_back(std::make_unique<InstancedGeometry>(new CubeGeometry(2.0f), instance_data, m_command_buffers));
    }

    void Scene::build_default_rendering_pipeline(const std::vector<std::string> &default_shaders)
    {
        const uint32_t frames = m_swapchain.image_count();

        m_depth_buffer = std::make_unique<re::DepthBuffer>(m_swapchain.extent(), m_command_buffers);
        m_render_pass  = std::make_unique<RenderPass>(m_device, m_swapchain.format(), m_depth_buffer->format());

        m_textures.push_back(std::make_unique<re::Texture>("stone.jpg", m_command_buffers));
        m_samplers.push_back(std::make_unique<re::Sampler>(m_device));

        m_uniform_camera.resize(frames);
        for (uint32_t i = 0; i < frames; i++)
        {
            m_uniform_camera[i] = std::make_unique<re::UniformBuffer<CameraUniform>>(m_command_buffers);
            update_camera(i);
        }

        m_dsl = DescriptorSetLayout()
            .uniform_buffer(SceneBindings::eCamera, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(SceneBindings::eDefaultTexture, vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_dslb)
            .create(m_device);

        m_descriptors = std::make_unique<DescriptorSets>(m_dslb, m_dsl, m_device);
        for (uint32_t i = 0; i < frames; i++)
        {
            vk::DescriptorBufferInfo camera { m_uniform_camera[i]->buffer(), 0, sizeof(CameraUniform) };
            vk::DescriptorImageInfo texture { m_samplers[0]->sampler(), m_textures[0]->view(), vk::ImageLayout::eShaderReadOnlyOptimal };

            DescriptorWrites(m_device, *m_descriptors)
                .uniform_buffer(i, SceneBindings::eCamera, camera)
                .combined_image_sampler(i, SceneBindings::eDefaultTexture, texture)
                .commit();
        }

        std::string src_vert, src_frag;
        std::for_each(
            std::begin(default_shaders),
            std::end(default_shaders),
            [&src_vert, &src_frag] (const auto& str) {
                if (str.find(".vert") != std::string::npos) src_vert = str;
                if (str.find(".frag") != std::string::npos) src_frag = str;
            }
        );

        m_pipeline = PipelineBuilder(m_device)
            .add_descriptor_set_layout(m_dsl)
            .create_pipeline_layout()
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader(src_vert, vk::ShaderStageFlagBits::eVertex)
            .add_shader(src_frag, vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*m_render_pass);
    }

    void Scene::update_camera(uint32_t index) const
    {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        float radius = 10.0f;
        float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f; // NOLINT(cppcoreguidelines-narrowing-conversions)
        auto eye = glm::vec3{sinf(time_since_start) * radius, 5, cosf(time_since_start) * radius};

        auto view = glm::lookAt(eye, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, -1, 0 });
        auto proj = glm::perspective(glm::radians(45.0f), m_swapchain.aspect_ratio(), 0.1f, 5000.0f);

        CameraUniform cu {
            .view = m_camera3d->view(),
            .projection = m_camera3d->projection(),
            .view_inverse = glm::inverse(m_camera3d->view()),
            .proj_inverse = glm::inverse(m_camera3d->projection()),
            .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1, 0, 1))
        };

        m_uniform_camera[index]->update(&cu);
    }
}

#pragma clang diagnostic pop
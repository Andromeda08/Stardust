#include "TerrainScene.hpp"

#include <glm/gtc/noise.hpp>
#include <Vulkan/Descriptor/DescriptorSetLayout.hpp>
#include <Vulkan/Descriptor/DescriptorWrites.hpp>

TerrainScene::TerrainScene(glm::ivec2 dim, Swapchain& swapchain, const CommandBuffer& command_buffer)
: m_command_buffers(command_buffer), m_device(command_buffer.device())
, m_swapchain(swapchain), m_dimensions(dim)
{
    m_camera = std::make_unique<Camera>(m_swapchain.extent(), glm::vec3(10.0f, 0, 0));
    m_depth_buffer = std::make_unique<re::DepthBuffer>(m_swapchain.extent(), m_command_buffers);
    m_render_pass = std::make_unique<RenderPass>(m_device, m_swapchain.format(), m_depth_buffer->format());

    generate_height_map();
    generate_terrain();
    build_pipeline();

    m_clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
    m_clear_values[1].setDepthStencil({ 1.0f, 0 });
    m_render_area = vk::Rect2D({ 0, 0 }, m_swapchain.extent());

    m_swapchain.createFrameBuffers(*m_render_pass, *m_depth_buffer);
}

void TerrainScene::generate_height_map()
{
    HeightMap result;
    for (size_t z = 0; z < m_dimensions.y; z++)
    {
        std::vector<int> vec1d;
        for (size_t x = 0; x < m_dimensions.x; x++)
        {
            float value = glm::simplex(glm::vec2((float) x / 128.0f, (float) z / 128.0f));
            value = ((value + 1.0f) / 2.0f) * 45.0f;
            vec1d.push_back(static_cast<int>(value));
        }
        result.push_back(vec1d);
    }
    m_height_map = result;
}

void TerrainScene::generate_terrain()
{
    for (size_t z = 0; z < m_dimensions.y; z++)
    {
        for (size_t x = 0; x < m_dimensions.x; x++)
        {
            auto height = std::clamp((float) m_height_map[z][x] * 0.2f, 0.1f, 1.0f);
            float fx = (1.0f / (float) m_dimensions.x) * (float) x;
            float fz = 1.0f - (1.0f / (float) m_dimensions.y) * (float) z;
            //auto color = glm::vec3(std::clamp(fx, 0.1f, 1.0f), 0.0f, std::clamp(fz, 0.1f, 1.0f));
            auto color = glm::vec3(0, 0.9f, std::clamp(fz, 0.1f, 1.0f));

            glm::vec3 pos = {
                (float) x - (float) m_dimensions.x / 2.0f,
                m_height_map[z][x],
                (float) z - (float) m_dimensions.y / 2.0f
            };

            re::InstanceData data = {
                .translate = pos,
                .scale     = glm::vec3(1.0f),
                .r_axis    = glm::vec3(1.0f),
                .r_angle   = 0.0f,
                .color     = color,
            };

            m_instance_data.push_back(data);
        }
    }

    m_objects = std::make_unique<re::InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
}

void TerrainScene::build_pipeline()
{
    const uint32_t frames = m_swapchain.image_count();

    m_dsl = DescriptorSetLayout()
        .uniform_buffer(SceneBindings::eCamera, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
        .get_bindings(m_dslb)
        .create(m_device);

    m_descriptors = std::make_unique<DescriptorSets>(m_dslb, m_dsl, m_device);

    m_uniform.resize(frames);
    for (size_t i = 0; i < frames; i++)
    {
        m_uniform[i] = std::make_unique<CameraUB>(m_command_buffers);
        update_camera(i);

        vk::DescriptorBufferInfo ubi { m_uniform[i]->buffer(), 0, sizeof(UniformCamera) };
        DescriptorWrites(m_device, *m_descriptors)
            .uniform_buffer(i, SceneBindings::eCamera, ubi)
            .commit();
    }

    auto attribs = re::VertexData::attribute_descriptions();
    attribs.emplace_back(4, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, translate));
    attribs.emplace_back(5, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, color));

    std::vector<vk::VertexInputBindingDescription> binds = {
        re::VertexData::binding_description(), re::InstanceData::binding_description()
    };

    m_pipeline = PipelineBuilder(m_device)
        .add_descriptor_set_layout(m_dsl)
        .create_pipeline_layout()
        .add_attribute_descriptions(attribs)
        .add_binding_descriptions(binds)
        .add_shader("terrain.vert.spv", vk::ShaderStageFlagBits::eVertex)
        .add_shader("terrain.frag.spv", vk::ShaderStageFlagBits::eFragment)
        .create_graphics_pipeline(*m_render_pass);
}

void TerrainScene::update_camera(uint32_t index) const
{
    auto e = m_camera->eye();
    UniformCamera camera = {
        .view         = m_camera->view(),
        .projection   = m_camera->projection(),
        .view_inverse = glm::inverse(m_camera->view()),
        .proj_inverse = glm::inverse(m_camera->projection()),
        .camera_pos   = glm::vec4(e.x, e.y, e.z, 1.0)
    };

    m_uniform[index]->update(&camera);
}

void TerrainScene::rasterize(uint32_t current_frame, vk::CommandBuffer cmd)
{
    update_camera(current_frame);

    vk::RenderPassBeginInfo begin_info;
    begin_info.setRenderPass(m_render_pass->handle());
    begin_info.setRenderArea(m_render_area);
    begin_info.setClearValueCount(m_clear_values.size());
    begin_info.setPClearValues(m_clear_values.data());
    begin_info.setFramebuffer(m_swapchain.framebuffer(current_frame));

    auto viewport = m_swapchain.make_negative_viewport();
    auto scissor = m_swapchain.make_scissor();

    cmd.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
    cmd.setViewport(0, 1, &viewport);
    cmd.setScissor(0, 1, &scissor);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline_layout, 0, 1, &m_descriptors->get_set(current_frame), 0, nullptr);

    m_objects->draw_instanced(cmd);

    cmd.endRenderPass();
}

void TerrainScene::scene_key_bindings(GLFWwindow* window)
{
    m_camera->use_inputs(window);
}

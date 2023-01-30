#include "Voxels.hpp"

namespace sd
{

    Voxels::Voxels(glm::ivec2 dim, Swapchain &swapchain, const CommandBuffers &command_buffers)
    : Scene(swapchain, command_buffers), m_dimensions(dim)
    {
        auto attribs = re::VertexData::attribute_descriptions();
        attribs.emplace_back(4, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, translate));
        attribs.emplace_back(5, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, color));

        std::vector<vk::VertexInputBindingDescription> binds = {
            re::VertexData::binding_description(), re::InstanceData::binding_description()
        };

        auto pipeline = PipelineBuilder(m_device)
            .add_descriptor_set_layout(m_descriptors.layout)
            .create_pipeline_layout()
            .add_attribute_descriptions(attribs)
            .add_binding_descriptions(binds)
            .add_shader("terrain.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("scene_default.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*m_render_pass);

        m_materials.insert({ "instanced", pipeline });

        generate_height_map();
        generate_terrain();

        m_cameras[m_active_camera] = std::make_unique<Camera>(m_swapchain.extent(), glm::vec3(0, 15, 0));
    }

    void Voxels::rasterize(uint32_t frame, vk::CommandBuffer cmd)
    {
        update_camera(frame);
        auto viewport = m_swapchain.make_viewport();
        auto scissor = m_swapchain.make_scissor();
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        render_pass(m_render_pass->handle(), frame, cmd, [&](){
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_materials["instanced"].pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_materials["instanced"].pipeline_layout, 0, 1, &m_descriptors.sets->operator[](frame), 0, nullptr);
            m_voxels->draw_instanced(cmd);
        });
    }

    void Voxels::register_keybinds(GLFWwindow *window)
    {
        m_cameras[m_active_camera]->use_inputs(window);
    }

    void Voxels::generate_height_map()
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

        int lo = 0, hi = 0;
        for (auto i : result) {
            for (auto j : i) {
                if (j <= lo) lo = j;
                if (j >= hi) hi = j;
            }
        }
//        std::cout << "Voxel height range : [" << lo << ", " << hi << "]" << std::endl;
//        std::cout << "Voxel grid size    : [" << m_dimensions.x << "x" << m_dimensions.y << "]" << std::endl;
    }

    void Voxels::generate_terrain()
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

        m_voxels = std::make_unique<re::InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
    }
}
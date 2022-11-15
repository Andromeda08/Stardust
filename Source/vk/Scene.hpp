#pragma once

#include <random>
#include <glm/glm.hpp>
#include <Resources/Geometry.hpp>
#include <Utility/Math.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <vk/Buffer.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Mesh.hpp>
#include <vk/UniformData.hpp>

namespace re
{
    class Scene
    {
    public:
        explicit Scene(const CommandBuffer& command_buffer)
        : m_command_buffers(command_buffer)
        {
            create_objects();
        }

        void create_objects()
        {
            std::default_random_engine rand(static_cast<unsigned>(time(nullptr)));
            std::uniform_int_distribution<int> uniform_dist(-1 * 128, 128);
            std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
            std::uniform_real_distribution<float> scale_mod(1.0f, 4.0f);

            for (int i = 0; i < 128; i++)
            {
                auto x = (float) uniform_dist(rand);
                auto y = (float) uniform_dist(rand);
                auto z = (float) uniform_dist(rand);

                m_instance_data.push_back({
                    .translate = glm::vec3{ x, y, z },
                    .scale = glm::vec3{ scale_mod(rand) },
                    .r_axis = glm::vec3{ 1 },
                    .r_angle = 0.0f,
                    .color = glm::vec4{ uniform_float(rand), uniform_float(rand), uniform_float(rand), 1.0f }
                });
            }

            m_objects.push_back(std::make_unique<InstancedGeometry>(new CubeGeometry(2.0f), m_instance_data, m_command_buffers));
        }

        virtual void draw(size_t current_frame, vk::CommandBuffer cmd) const
        {

            for (const auto& obj : m_objects)
            {
                obj->draw_instanced(cmd);
            }
        }

    private:
        const CommandBuffer& m_command_buffers;

        std::vector<InstanceData> m_instance_data;
        std::vector<std::unique_ptr<InstancedGeometry>> m_objects;
    };
}
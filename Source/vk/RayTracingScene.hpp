#pragma once

#include <memory>
#include <random>
#include <vulkan/vulkan.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <rt/AccelerationStructure.hpp>
#include <rt/RtAccelerator.hpp>

namespace re
{
    class RayTracingScene
    {
    public:
        explicit RayTracingScene(const CommandBuffer& command_buffers) : m_command_buffers(command_buffers)
        {
            build_objects();
            build_acceleration_structures();
        }

        void build_objects()
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

            m_objects = std::make_unique<InstancedGeometry>(new SphereGeometry(2.0f), m_instance_data, m_command_buffers);
        }

        void build_acceleration_structures()
        {
            m_accelerator = RtAccelerator::create_accelerator(*m_objects, m_command_buffers);
        }

        void trace_rays(vk::CommandBuffer cmd)
        {

        }

    private:
        const CommandBuffer& m_command_buffers;

        RtAccelerator m_accelerator;

        std::vector<InstanceData> m_instance_data;
        std::unique_ptr<InstancedGeometry> m_objects;
    };
}
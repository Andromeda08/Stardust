#pragma once

#include <random>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Scenes/Scene.hpp>
#include <rt/AccelerationStructure.hpp>
#include <rt/RtAccelerator.hpp>
#include <vk/Image.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Sampler.hpp>
#include <vk/Material.hpp>

namespace sd
{
    /**
     * @brief Ray tracing demo
     */
    class Raytracing : public Scene
    {
    public:
        struct RtUniformData
        {
            glm::mat4 view_proj;
            glm::mat4 view_inverse;
            glm::mat4 proj_inverse;
        };
        struct RtObjBufferAddresses
        {
            uint64_t vertex_addr;
            uint64_t index_addr;
        };
        struct RtPushConstantData
        {
            int material_index;
        };
        using CameraUB = re::UniformBuffer<RtUniformData>;
        using MaterialUB = re::UniformBuffer<Material>;

        Raytracing(Swapchain& swapchain, CommandBuffers const& command_buffers);

        void trace_rays(uint32_t current_frame, vk::CommandBuffer cmd) override;

        void blit(uint32_t current_frame, vk::CommandBuffer cmd);

        void register_keybinds(GLFWwindow* window) override;

    private:
        void generate_objects(uint32_t entities = 1024);

        void build_acceleration_structures();

        void build_descriptors();

        void build_sbt();

        template <typename T>
        inline T round_up(T k, T alignment) {
            return (k + alignment - 1) & ~(alignment - 1);
        }

    private:
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rt_props;

        RtAccelerator m_accelerator;
        std::unique_ptr<re::Image> m_output_image;
        std::vector<std::unique_ptr<CameraUB>> m_uniform_camera;
        std::vector<std::unique_ptr<MaterialUB>> m_uniform_material;
        std::unique_ptr<re::Buffer> m_shader_binding_table;
        std::unique_ptr<re::Buffer> m_obj_buffer_addresses;

        Descriptors m_rt_descriptors;

        int m_current_material {0};
        std::vector<Material> m_mats;

        std::vector<re::InstanceData>          m_instance_data;
        std::unique_ptr<re::InstancedGeometry> m_cubes;
    };
}
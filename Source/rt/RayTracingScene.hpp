#pragma once

#include <memory>
#include <random>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <rt/RtAccelerator.hpp>
#include <vk/Image.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Sampler.hpp>
#include <vk/Material.hpp>
#include <vk/Shader.hpp>
#include <vk/Descriptors/DescriptorSetLayout.hpp>
#include <vk/Descriptors/DescriptorSets.hpp>
#include <vk/Presentation/Swapchain.hpp>

namespace re
{
    class RayTracingScene
    {
    public:
        struct RtUniformData
        {
            glm::mat4 viewProjection;
            glm::mat4 viewInverse;
            glm::mat4 projInverse;
        };
        struct RtObjDesc
        {
            uint64_t vertex_addr;
            uint64_t index_addr;
        };
        struct RtPushConstants
        {
            int material_index;
        };

        RayTracingScene(const Swapchain& swap_chain, const CommandBuffers& command_buffers);

        void trace_rays(uint32_t current_frame, vk::CommandBuffer cmd);

        void rasterize(uint32_t current_frame, vk::CommandBuffer cmd);

        /**
         * @brief Blit output image to swapchain image.
         */
        void blit(uint32_t current_frame, vk::CommandBuffer cmd);

        const vk::Image& output() const { return m_output->image(); }

        void rt_keybinds(GLFWwindow* window);

    private:
        /**
         * @brief Build descriptors used for ray tracing.
         */
        void build_descriptors();

        /**
         * @brief Generate scene with "entities" number of flying cubes.
         */
        void build_objects(uint32_t entities = 1024);

        /**
         * @brief Build acceleration structures from scene objects.
         */
        void build_acceleration_structures();

        void build_rt_pipeline();

        /**
         * @brief Create the shader binding table buffer.
         */
        void build_sbt();

        template <typename T>
        inline T round_up(T k, T alignment) {
            return (k + alignment - 1) & ~(alignment - 1);
        }

    private:
        // Cached device raytracing properties
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rt_props;

        // Raytracing: AS, UB, output image, SBT.
        RtAccelerator m_accelerator;
        std::unique_ptr<re::Image> m_output;
        std::vector<std::unique_ptr<re::UniformBuffer<RtUniformData>>> m_ubs;
        std::vector<std::unique_ptr<re::UniformBuffer<Material>>> m_material_data;
        std::unique_ptr<re::Buffer> m_sbt;
        std::unique_ptr<re::Buffer> m_obj_desc;

        // Raytracing pipeline descriptors
        std::vector<vk::DescriptorSetLayoutBinding> m_dslb;
        vk::DescriptorSetLayout m_dsl;
        std::unique_ptr<DescriptorSets> m_descriptors;

        // Raytracing pipeline & layout
        vk::Pipeline m_pipeline;
        vk::PipelineLayout m_pipeline_layout;

        // Scene objects
        std::vector<Material> m_materials;
        std::vector<InstanceData> m_instance_data;
        std::unique_ptr<InstancedGeometry> m_objects;

        int m_current_material = 0;

        // Vulkan dependencies
        const CommandBuffers& m_command_buffers;
        const Device&         m_device;
        const Swapchain&      m_swap_chain;
    };
}
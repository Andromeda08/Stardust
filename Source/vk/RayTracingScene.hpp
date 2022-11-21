#pragma once

#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <rt/Material.hpp>
#include <rt/RtAccelerator.hpp>
#include <vk/ComputePipeline.hpp>
#include <vk/Image.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Sampler.hpp>
#include <Vulkan/Swapchain.hpp>
#include <Vulkan/Descriptor/DescriptorSetLayout.hpp>
#include <Vulkan/Descriptor/DescriptorSets.hpp>
#include <Vulkan/GraphicsPipeline/ShaderModule.hpp>

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
        struct RtMaterialData
        {
            glm::vec4 ambient;
            glm::vec4 diffuse;
            glm::vec4 specular;
            glm::vec4 shininess;
        };

        RayTracingScene(const Swapchain& swap_chain, const CommandBuffer& command_buffers);

        void trace_rays(uint32_t current_frame, vk::CommandBuffer cmd);

        void rasterize(uint32_t current_frame, vk::CommandBuffer cmd);

        /**
         * @brief Blit output image to swapchain image.
         */
        void blit(uint32_t current_frame, vk::CommandBuffer cmd);

        const vk::Image& output() const { return m_output->image(); }

    private:
        /**
         * @brief Build descriptors used for ray tracing.
         */
        void build_descriptors();

        /**
         * @brief Generate random scene.
         */
        void build_objects();
        void build_reflection_scene();

        /**
         * @brief Build acceleration structures from scene objects.
         */
        void build_acceleration_structures();

        void build_rt_pipeline();

        /**
         * @brief Create the shader binding table buffer.
         */
        void build_sbt();

        void build_compute_pipeline();

        void compute_copy_to_swapchain(uint32_t current_frame, vk::CommandBuffer cmd)
        {
            auto vps = m_swap_chain.extent();
            const uint32_t group_size_x = 32, group_size_y = 32;
            uint32_t group_count_x = (vps.width + group_size_x - 1) / group_size_x;
            uint32_t group_count_y = (vps.height + group_size_y - 1) / group_size_y;
            uint32_t push_constants[] = { vps.width, vps.height };

            cmd.pushConstants(m_compute_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push_constants), push_constants, m_device.dispatch());
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_compute_layout, 0, 1, &m_compute_descriptors->get_set(current_frame), 0, nullptr, m_device.dispatch());
            cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_compute_pipeline, m_device.dispatch());
            cmd.dispatch(group_count_x, group_count_y, 1, m_device.dispatch());
        }

        template <typename T>
        inline T round_up(T k, T alignment) {
            return (k + alignment - 1) & ~(alignment - 1);
        }

    private:
        // Cached device raytracing properties
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rt_props;

        // Raytracing: AS, UB, output image, SBT.
        RtAccelerator m_accelerator;
        std::unique_ptr<re::vkImage> m_output;
        std::vector<std::unique_ptr<re::UniformBuffer<RtUniformData>>> m_ubs;
        std::vector<std::unique_ptr<re::UniformBuffer<RtMaterialData>>> m_material_data;
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
        std::vector<InstanceData> m_instance_data;
        std::unique_ptr<InstancedGeometry> m_objects;

        // Compute pipeline objects.
        vk::Pipeline m_compute_pipeline;
        vk::PipelineLayout m_compute_layout;
        std::unique_ptr<re::Sampler> m_sampler;
        std::vector<vk::DescriptorSetLayoutBinding> m_compute_dslb;
        vk::DescriptorSetLayout m_compute_dsl;
        std::unique_ptr<DescriptorSets> m_compute_descriptors;

        // Vulkan dependencies
        const CommandBuffer& m_command_buffers;
        const Device&        m_device;
        const Swapchain&     m_swap_chain;
    };
}
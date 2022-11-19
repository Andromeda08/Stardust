#pragma once

#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <rt/RtAccelerator.hpp>
#include <vk/ComputePipeline.hpp>
#include <vk/Image.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Sampler.hpp>
#include <Vulkan/Swapchain.hpp>
#include <Vulkan/Descriptor/DescriptorSetLayout.hpp>
#include <Vulkan/GraphicsPipeline/ShaderModule.hpp>

namespace re
{
    class RayTracingScene
    {
    public:
        struct RtUniformData
        {
            glm::mat4 viewInverse;
            glm::mat4 projInverse;
        };

        explicit RayTracingScene(const Swapchain& swap_chain,
                                 const CommandBuffer& command_buffers)
        : m_command_buffers(command_buffers)
        , m_device(command_buffers.device())
        , m_swap_chain(swap_chain)
        {
            vk::PhysicalDeviceProperties2 pdp;
            pdp.pNext = &m_rt_props;
            m_command_buffers.device().physicalDevice()
                .getProperties2(&pdp, m_command_buffers.device().dispatch());

            // Build ray tracing scene and pipeline
            build_objects();
            build_acceleration_structures();
            build_descriptors();
            build_pipeline();
            build_sbt();

            // Build compute pipeline for copying output to swapchain
            build_compute_pipeline();
        }

        void rasterize(vk::CommandBuffer cmd)
        {
            m_objects->draw_instanced(cmd);
        }

        void trace_rays(uint32_t current_frame, vk::CommandBuffer cmd)
        {
            cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline_layout, 0, 1,
                                          &m_descriptors->get_set(current_frame), 0, nullptr);

            auto sbt_slot_size = m_rt_props.shaderGroupHandleSize;
            auto miss_offset = round_up(sbt_slot_size, m_rt_props.shaderGroupBaseAlignment);
            auto hit_offset = round_up(miss_offset + sbt_slot_size, m_rt_props.shaderGroupBaseAlignment);

            vk::StridedDeviceAddressRegionKHR raygen_sbt;
            raygen_sbt.setDeviceAddress(m_sbt->address() + 0);
            raygen_sbt.setStride(sbt_slot_size);
            raygen_sbt.setSize(sbt_slot_size);

            vk::StridedDeviceAddressRegionKHR miss_sbt;
            miss_sbt.setDeviceAddress(m_sbt->address() + miss_offset);
            miss_sbt.setStride(sbt_slot_size);
            miss_sbt.setSize(sbt_slot_size);

            vk::StridedDeviceAddressRegionKHR chit_sbt;
            chit_sbt.setDeviceAddress(m_sbt->address() + hit_offset);
            chit_sbt.setStride(sbt_slot_size);
            chit_sbt.setSize(sbt_slot_size);

            vk::StridedDeviceAddressRegionKHR callable_sbt {};

            auto e = m_swap_chain.extent();
            cmd.traceRaysKHR(&raygen_sbt, &miss_sbt, &chit_sbt, &callable_sbt,
                             e.width, e.height, 1, m_device.dispatch());
        }

        void copy_to_swapchain(uint32_t current_frame, vk::CommandBuffer cmd)
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

        const vk::Image& output() const { return m_output->image(); }

    private:
        void build_compute_pipeline()
        {
            m_compute_dsl = DescriptorSetLayout()
                .sampler(0, vk::ShaderStageFlagBits::eCompute)
                .sampled_image(1, vk::ShaderStageFlagBits::eCompute)
                .storage_image(2, vk::ShaderStageFlagBits::eCompute)
                .get_bindings(m_compute_dslb)
                .create(m_device);

            m_compute_descriptors = std::make_unique<DescriptorSets>(m_compute_dslb, m_compute_dsl, m_device);

            m_sampler = std::make_unique<re::Sampler>(m_device);

            for (size_t i = 0; i < 2; i++)
            {
                vk::DescriptorImageInfo sampler;
                sampler.setSampler(m_sampler->sampler());
                vk::WriteDescriptorSet write1;
                write1.setDstSet(m_compute_descriptors->get_set(i));
                write1.setDstBinding(0);
                write1.setDescriptorCount(1);
                write1.setDescriptorType(vk::DescriptorType::eSampler);
                write1.setPImageInfo(&sampler);
                m_device.handle().updateDescriptorSets(1, &write1, 0, nullptr);

                vk::DescriptorImageInfo sampled_info;
                sampled_info.setImageLayout(vk::ImageLayout::eGeneral);
                sampled_info.setImageView(m_output->view());
                m_compute_descriptors->update_descriptor_set(i, 1, sampled_info);

                vk::DescriptorImageInfo image_info;
                image_info.setImageView(m_swap_chain.view(i));
                image_info.setImageLayout(vk::ImageLayout::eGeneral);
                vk::WriteDescriptorSet write;
                write.setDstSet(m_compute_descriptors->get_set(i));
                write.setDstBinding(2);
                write.setDescriptorCount(1);
                write.setDescriptorType(vk::DescriptorType::eStorageImage);
                write.setPImageInfo(&image_info);
                m_device.handle().updateDescriptorSets(1, &write, 0, nullptr);
            }

            vk::PushConstantRange push_constant_range;
            push_constant_range.setStageFlags(vk::ShaderStageFlagBits::eCompute);
            push_constant_range.setOffset(0);
            push_constant_range.setSize(8);

            vk::PipelineLayoutCreateInfo create_info;
            create_info.setSetLayoutCount(1);
            create_info.setPSetLayouts(&m_compute_dsl);
            create_info.setPushConstantRangeCount(1);
            create_info.setPPushConstantRanges(&push_constant_range);

            auto result = m_device.handle().createPipelineLayout(&create_info, nullptr, &m_compute_layout, m_device.dispatch());

            m_compute_pipeline = ComputePipeline::make_compute_pipeline("swapchain.comp.spv", m_compute_layout, m_device);
        }

        void build_descriptors()
        {
            m_dsl = DescriptorSetLayout()
                .accelerator(0, vk::ShaderStageFlagBits::eRaygenKHR)
                .storage_image(1, vk::ShaderStageFlagBits::eRaygenKHR)
                .uniform_buffer(2, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eVertex)
                .get_bindings(m_dslb)
                .create(m_command_buffers.device());

            m_descriptors = std::make_unique<DescriptorSets>(m_dslb, m_dsl, m_device);

            m_output = std::make_unique<re::vkImage>(m_swap_chain.extent(),
                                                     vk::Format::eR16G16B16A16Sfloat,
                                                     vk::ImageTiling::eOptimal,
                                                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                     m_command_buffers);

            m_output->transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

            m_ubs.resize(2);
            for (auto i = 0; i < 2; i++)
            {
                m_ubs[i] = std::make_unique<re::UniformBuffer<RtUniformData>>(m_command_buffers);
                auto view = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
                auto proj = glm::perspective(glm::radians(45.0f), m_swap_chain.aspectRatio(), 0.1f, 2000.0f);
                RtUniformData data = {
                    .viewInverse = glm::inverse(view),
                    .projInverse = glm::inverse(proj)
                };
                m_ubs[i]->update(data);
            }

            vk::DescriptorImageInfo image_info;
            image_info.setImageView(m_output->view());
            image_info.setImageLayout(vk::ImageLayout::eGeneral);

            for (auto i = 0 ; i < 2; i++)
            {
                vk::WriteDescriptorSetAccelerationStructureKHR accel;
                accel.setAccelerationStructureCount(1);
                accel.setPAccelerationStructures(&m_accelerator.tlas.tlas);
                vk::WriteDescriptorSet accelw;
                accelw.setDstBinding(0);
                accelw.setDstSet(m_descriptors->get_set(i));
                accelw.setDescriptorCount(1);
                accelw.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
                accelw.setDstArrayElement(0);
                accelw.setPNext(&accel);
                m_device.handle().updateDescriptorSets(1, &accelw, 0, nullptr, m_device.dispatch());

                vk::WriteDescriptorSet write;
                write.setDstBinding(1);
                write.setDstSet(m_descriptors->get_set(i));
                write.setDescriptorCount(1);
                write.setDescriptorType(vk::DescriptorType::eStorageImage);
                write.setPImageInfo(&image_info);
                write.setDstArrayElement(0);
                m_device.handle().updateDescriptorSets(1, &write, 0, nullptr, m_device.dispatch());

                vk::DescriptorBufferInfo ubo_info;
                ubo_info.setBuffer(m_ubs[i]->buffer());
                ubo_info.setOffset(0);
                ubo_info.setRange(sizeof(RtUniformData));
                m_descriptors->update_descriptor_set(i, 2, ubo_info);
            }
        }

        void premade_objects()
        {
            m_instance_data.push_back({});
            m_objects = std::make_unique<InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
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

            m_objects = std::make_unique<InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
        }

        void build_acceleration_structures()
        {
            m_accelerator = RtAccelerator::create_accelerator(*m_objects, m_command_buffers);
        }

        void build_pipeline()
        {
            vk::Result result;

            vk::PipelineLayoutCreateInfo pl_create_info;
            pl_create_info.setSetLayoutCount(1);
            pl_create_info.setPSetLayouts(&m_dsl);

            result = m_device.handle().createPipelineLayout(&pl_create_info, nullptr, &m_pipeline_layout, m_device.dispatch());

#pragma region Shaders
            ShaderModule sh_rgen(vk::ShaderStageFlagBits::eRaygenKHR, "raytrace.rgen.spv", m_device);
            ShaderModule sh_miss(vk::ShaderStageFlagBits::eMissKHR, "raytrace.rmiss.spv", m_device);
            ShaderModule sh_chit(vk::ShaderStageFlagBits::eClosestHitKHR, "raytrace.rchit.spv", m_device);

            std::vector<vk::PipelineShaderStageCreateInfo> stage_infos = {
                sh_rgen.stage_info(), sh_miss.stage_info(), sh_chit.stage_info()
            };

            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups(3);
            for (auto i = 0; i < 2; i++)
            {
                auto& g = shader_groups[i];
                g.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
                g.setGeneralShader(i);
                g.setClosestHitShader(VK_SHADER_UNUSED_KHR);
                g.setAnyHitShader(VK_SHADER_UNUSED_KHR);
                g.setIntersectionShader(VK_SHADER_UNUSED_KHR);
            }

            auto& g3 = shader_groups[2];
            g3.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
            g3.setGeneralShader(VK_SHADER_UNUSED_KHR);
            g3.setClosestHitShader(2);
            g3.setAnyHitShader(VK_SHADER_UNUSED_KHR);
            g3.setIntersectionShader(VK_SHADER_UNUSED_KHR);
#pragma endregion

            vk::RayTracingPipelineCreateInfoKHR create_info;
            create_info.setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR |
                                 vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR);
            create_info.setStageCount(stage_infos.size());
            create_info.setPStages(stage_infos.data());
            create_info.setGroupCount(shader_groups.size());
            create_info.setPGroups(shader_groups.data());
            create_info.setMaxPipelineRayRecursionDepth(1);
            create_info.setLayout(m_pipeline_layout);

            result = m_device.handle().createRayTracingPipelinesKHR(nullptr, nullptr,
                                                                    1, &create_info,
                                                                    nullptr, &m_pipeline,
                                                                    m_device.dispatch());
        }

        void build_sbt()
        {
            vk::Result r;
            auto device = m_command_buffers.device().handle();
            auto d = m_command_buffers.device().dispatch();

            auto sghs = m_rt_props.shaderGroupHandleSize;
            auto sgba = m_rt_props.shaderGroupBaseAlignment;
            uint32_t miss = round_up(sghs, sgba);
            uint32_t hit = round_up(miss + sghs, sgba);
            uint32_t sbt_size = hit + sghs;

            std::vector<uint8_t> data(sbt_size);
            r = device.getRayTracingShaderGroupHandlesKHR(m_pipeline, 0, 1, sghs, data.data() + 0, d);
            r = device.getRayTracingShaderGroupHandlesKHR(m_pipeline, 1, 1, sghs, data.data() + miss, d);
            r = device.getRayTracingShaderGroupHandlesKHR(m_pipeline, 2, 1, sghs, data.data() + hit, d);
            m_sbt = std::make_unique<re::Buffer>(sbt_size,
                                                 vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eTransferDst |
                                                 vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
                                                 m_command_buffers);

            re::Buffer::set_data(data.data(), *m_sbt, m_command_buffers);
        }

        template <typename T>
        inline T round_up(T k, T alignment) {
            return (k + alignment - 1) & ~(alignment - 1);
        }

    private:
        const CommandBuffer& m_command_buffers;
        const Device& m_device;
        const Swapchain& m_swap_chain;

        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rt_props;

        RtAccelerator m_accelerator;
        std::unique_ptr<re::vkImage> m_output;
        std::vector<std::unique_ptr<re::UniformBuffer<RtUniformData>>> m_ubs;
        std::unique_ptr<re::Buffer> m_sbt;

        std::vector<vk::DescriptorSetLayoutBinding> m_dslb;
        vk::DescriptorSetLayout m_dsl;
        std::unique_ptr<DescriptorSets> m_descriptors;

        vk::Pipeline m_pipeline;
        vk::PipelineLayout m_pipeline_layout;

        std::vector<InstanceData> m_instance_data;
        std::unique_ptr<InstancedGeometry> m_objects;

        vk::Pipeline m_compute_pipeline;
        vk::PipelineLayout m_compute_layout;
        std::unique_ptr<re::Sampler> m_sampler;
        std::vector<vk::DescriptorSetLayoutBinding> m_compute_dslb;
        vk::DescriptorSetLayout m_compute_dsl;
        std::unique_ptr<DescriptorSets> m_compute_descriptors;
    };
}
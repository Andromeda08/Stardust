#include "RayTracingScene.hpp"

namespace re
{
    RayTracingScene::RayTracingScene(const Swapchain &swap_chain, const CommandBuffer &command_buffers)
    : m_command_buffers(command_buffers), m_device(command_buffers.device()), m_swap_chain(swap_chain)
    {
        vk::PhysicalDeviceProperties2 pdp;
        pdp.pNext = &m_rt_props;
        m_command_buffers.device().physicalDevice()
            .getProperties2(&pdp, m_command_buffers.device().dispatch());

        // Build ray tracing scene and pipeline
        build_objects();
        //build_reflection_scene();
        build_acceleration_structures();
        build_descriptors();
        build_rt_pipeline();
        build_sbt();

        // Build compute pipeline for copying output to swapchain
        // build_compute_pipeline();
    }

    void RayTracingScene::rasterize(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        // TODO: Build raster pipeline in scene
        m_objects->draw_instanced(cmd);
    }

    void RayTracingScene::trace_rays(uint32_t current_frame, vk::CommandBuffer cmd)
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

    void RayTracingScene::blit(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        vk::ImageSubresourceRange subresource_range { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        re::vkImage::set_layout(cmd, m_swap_chain.image(current_frame),
                                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                subresource_range);

        re::vkImage::set_layout(cmd, m_output->image(),
                                vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal,
                                subresource_range);

        vk::Offset3D blit_size {
            static_cast<int32_t>(m_swap_chain.extent().width),
            static_cast<int32_t>(m_swap_chain.extent().height),
            1
        };

        vk::ImageSubresourceLayers subresource_layers;
        subresource_layers.setLayerCount(1);
        subresource_layers.setAspectMask(vk::ImageAspectFlagBits::eColor);

        vk::ImageBlit blit_info;
        blit_info.srcOffsets[1] = blit_size;
        blit_info.dstOffsets[1] = blit_size;
        blit_info.setSrcSubresource(subresource_layers);
        blit_info.setDstSubresource(subresource_layers);

        cmd.blitImage(m_output->image(), vk::ImageLayout::eTransferSrcOptimal,
                      m_swap_chain.image(current_frame), vk::ImageLayout::eTransferDstOptimal,
                      1, &blit_info, vk::Filter::eNearest);

        re::vkImage::set_layout(cmd, m_swap_chain.image(current_frame),
                                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
                                subresource_range);

        re::vkImage::set_layout(cmd, m_output->image(),
                                vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral,
                                subresource_range);
    }

    void RayTracingScene::build_objects()
    {
        auto seed = static_cast<unsigned>(time(nullptr));
        std::cout << "Seed: " << seed << std::endl;

        std::default_random_engine rand(1669071706);
        std::uniform_int_distribution<int> uniform_dist(-32,32);
        std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
        std::uniform_real_distribution<float> scale_mod(1.0f, 5.0f);

        for (int i = 0; i < 192; i++)
        {
            auto x = (float) uniform_dist(rand);
            auto y = (float) uniform_dist(rand);
            auto z = (float) uniform_dist(rand);

            m_instance_data.push_back({
                .translate = glm::vec3{ x, y, z },
                .scale = glm::vec3{ scale_mod(rand) },
                .r_axis = glm::vec3{ 1 },
                .r_angle = uniform_float(rand) * 360.0f,
                .color = glm::vec4{ uniform_float(rand), uniform_float(rand), uniform_float(rand), 1.0f }
            });
        }

        m_objects = std::make_unique<InstancedGeometry>(new SphereGeometry(1.0f), m_instance_data, m_command_buffers);
    }

    void RayTracingScene::build_reflection_scene()
    {
        m_instance_data.push_back(re::InstanceData {
            .translate = glm::vec3(2, 0, 0),
            .scale = glm::vec3(5.0f),
            .r_axis = glm::vec3{ 1 },
            .r_angle = 60.0f,
            .color = glm::vec4(0.5f)
        });

        m_instance_data.push_back(re::InstanceData {
            .translate = glm::vec3(0, -5, 0),
            .scale = glm::vec3(2.5f),
            .r_axis = glm::vec3{ 1 },
            .r_angle = 0.0f,
            .color = glm::vec4(0.5f)
        });

        m_objects = std::make_unique<InstancedGeometry>(new SphereGeometry(1.0f), m_instance_data, m_command_buffers);
    }

    void RayTracingScene::build_acceleration_structures()
    {
        m_accelerator = RtAccelerator::create_accelerator(*m_objects, m_command_buffers);
    }

    void RayTracingScene::build_descriptors()
    {
        m_dsl = DescriptorSetLayout()
            .accelerator(0, vk::ShaderStageFlagBits::eRaygenKHR)
            .storage_image(1, vk::ShaderStageFlagBits::eRaygenKHR)
            .uniform_buffer(2, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
            .storage_buffer(3, vk::ShaderStageFlagBits::eClosestHitKHR)
            .uniform_buffer(4, vk::ShaderStageFlagBits::eClosestHitKHR)
            .get_bindings(m_dslb)
            .create(m_command_buffers.device());

        m_descriptors = std::make_unique<DescriptorSets>(m_dslb, m_dsl, m_device);

        m_output = std::make_unique<re::vkImage>(m_swap_chain.extent(),
                                                 vk::Format::eR16G16B16A16Sfloat,
                                                 vk::ImageTiling::eOptimal,
                                                 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                 m_command_buffers);

        m_output->transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);


        auto view = glm::lookAt(glm::vec3(15, 4, 15), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
        auto proj = glm::perspective(glm::radians(45.0f), m_swap_chain.aspectRatio(), 0.1f, 1000.0f);

        /*
        RtMaterialData material_data = {
            .ambient = glm::vec4(0.0215f, 0.1745f, 0.0215f, 0.95f),
            .diffuse = glm::vec4(0.54f, 0.89f, 0.63f, 0.95f),
            .specular = glm::vec4(0.3162f, 0.3162f, 0.3162f, 0.95f),
            .shininess = glm::vec4(12.8f)
        };
        */

        RtMaterialData material_data = {
            .ambient = glm::vec4(0.105882f, 0.058824f, 0.113725f, 1.0f),
            .diffuse = glm::vec4(0.427451f, 0.470588f, 0.541176f, 1.0f),
            .specular = glm::vec4(0.333333f, 0.333333f, 0.521569f, 1.0f),
            .shininess = glm::vec4(9.84615f)
        };

        RtUniformData data = {
            .viewProjection = view * proj,
            .viewInverse = glm::inverse(view),
            .projInverse = glm::inverse(proj),
        };

        m_ubs.resize(2);
        m_material_data.resize(2);
        for (auto i = 0; i < 2; i++)
        {
            m_ubs[i] = std::make_unique<re::UniformBuffer<RtUniformData>>(m_command_buffers);
            m_ubs[i]->update(data);

            m_material_data[i] = std::make_unique<re::UniformBuffer<RtMaterialData>>(m_command_buffers);
            m_material_data[i]->update(material_data);
        }

        RtObjDesc obj_desc {
            .vertex_addr = m_objects->mesh().vertex_buffer().address(),
            .index_addr = m_objects->mesh().index_buffer().address(),
        };
        vk::DeviceSize size = sizeof(RtObjDesc);
        m_obj_desc = std::make_unique<re::Buffer>(size,
                                                  vk::BufferUsageFlagBits::eStorageBuffer,
                                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                  m_command_buffers);
        re::Buffer::set_data(&obj_desc, *m_obj_desc, m_command_buffers);

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

            vk::DescriptorImageInfo image_info;
            image_info.setImageView(m_output->view());
            image_info.setImageLayout(vk::ImageLayout::eGeneral);

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

            vk::DescriptorBufferInfo obj_info;
            obj_info.setBuffer(m_obj_desc->buffer());
            obj_info.setOffset(0);
            obj_info.setRange(sizeof(RtObjDesc));
            vk::WriteDescriptorSet write2;
            write2.setDstBinding(3);
            write2.setDstSet(m_descriptors->get_set(i));
            write2.setDescriptorCount(1);
            write2.setDescriptorType(vk::DescriptorType::eStorageBuffer);
            write2.setPBufferInfo(&obj_info);
            write2.setDstArrayElement(0);
            m_device.handle().updateDescriptorSets(1, &write2, 0, nullptr, m_device.dispatch());

            vk::DescriptorBufferInfo material_info;
            material_info.setBuffer(m_material_data[i]->buffer());
            material_info.setOffset(0);
            material_info.setRange(sizeof(RtMaterialData));
            m_descriptors->update_descriptor_set(i, 4, material_info);
        }
    }

    void RayTracingScene::build_rt_pipeline()
    {
        vk::Result result;

        vk::PipelineLayoutCreateInfo pl_create_info;
        pl_create_info.setSetLayoutCount(1);
        pl_create_info.setPSetLayouts(&m_dsl);

        result = m_device.handle().createPipelineLayout(&pl_create_info, nullptr, &m_pipeline_layout, m_device.dispatch());

        ShaderModule sh_rgen(vk::ShaderStageFlagBits::eRaygenKHR, "reflection.rgen.spv", m_device);
        ShaderModule sh_miss(vk::ShaderStageFlagBits::eMissKHR, "reflection.rmiss.spv", m_device);
        ShaderModule sh_chit(vk::ShaderStageFlagBits::eClosestHitKHR, "reflection.rchit.spv", m_device);

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

        vk::RayTracingPipelineCreateInfoKHR create_info;
        create_info.setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR |
                             vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR);
        create_info.setStageCount(stage_infos.size());
        create_info.setPStages(stage_infos.data());
        create_info.setGroupCount(shader_groups.size());
        create_info.setPGroups(shader_groups.data());
        create_info.setMaxPipelineRayRecursionDepth(2);
        create_info.setLayout(m_pipeline_layout);

        result = m_device.handle().createRayTracingPipelinesKHR(nullptr, nullptr,
                                                                1, &create_info,
                                                                nullptr, &m_pipeline,
                                                                m_device.dispatch());
    }

    void RayTracingScene::build_sbt()
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

    void RayTracingScene::build_compute_pipeline()
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
}
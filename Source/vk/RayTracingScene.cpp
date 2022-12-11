#include "RayTracingScene.hpp"

#include <Vulkan/Descriptor/DescriptorWrites.hpp>

namespace re
{
    RayTracingScene::RayTracingScene(const Swapchain &swap_chain, const CommandBuffer &command_buffers)
    : m_command_buffers(command_buffers), m_device(command_buffers.device()), m_swap_chain(swap_chain)
    {
        vk::PhysicalDeviceProperties2 pdp;
        pdp.pNext = &m_rt_props;
        m_command_buffers.device().physicalDevice()
            .getProperties2(&pdp, m_command_buffers.device().dispatch());

        build_objects();
        build_acceleration_structures();
        build_descriptors();
        build_rt_pipeline();
        build_sbt();
    }

    void RayTracingScene::rasterize(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        // TODO: Build raster pipeline in scene
        m_objects->draw_instanced(cmd);
    }

    void RayTracingScene::trace_rays(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        RtAccelerator::rebuild_top_level(*m_objects, m_accelerator, m_command_buffers);

        vk::WriteDescriptorSetAccelerationStructureKHR as_info;
        as_info.setAccelerationStructureCount(1);
        as_info.setPAccelerationStructures(&m_accelerator.tlas.tlas);

        DescriptorWrites(m_device, *m_descriptors)
            .acceleration_structure(current_frame, 0, as_info)
            .commit();

        std::vector<RtPushConstants> pc = {{ m_current_material }};

        cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline_layout, 0, 1,
                               &m_descriptors->get_set(current_frame), 0, nullptr);
        cmd.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(RtPushConstants), pc.data());

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
                         e.width, e.height, 2, m_device.dispatch());
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

    void RayTracingScene::build_objects(uint32_t entities)
    {
        m_materials = { Materials::purple, Materials::obsidian, Materials::ruby, Materials::pearl,
                        Materials::gold, Materials::emerald, Materials::cyan };

        auto seed = static_cast<unsigned>(time(nullptr));
        std::cout << "Seed: " << seed << std::endl;

        std::default_random_engine rand(seed);
        std::uniform_int_distribution<int> uniform_dist(-48,48);
        std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
        std::uniform_real_distribution<float> scale_mod(1.0f, 5.0f);

        for (int i = 0; i < entities; i++)
        {
            int mat = rand() % m_materials.size();

            auto x = (float) uniform_dist(rand);
            auto y = (float) uniform_dist(rand);
            auto z = (float) uniform_dist(rand);

            m_instance_data.push_back({
                .translate = glm::vec3{ x, y, z },
                .scale = glm::vec3{ scale_mod(rand) },
                .r_axis = glm::vec3{ uniform_dist(rand), uniform_dist(rand), uniform_dist(rand) },
                .r_angle = uniform_float(rand) * 360.0f,
                .color = glm::vec4{ uniform_float(rand), uniform_float(rand), uniform_float(rand), 1.0f },
                .material_idx = mat,
                .hit_group = 0
            });
        }

        m_objects = std::make_unique<InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
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
                                                 vk::ImageAspectFlagBits::eColor,
                                                 m_command_buffers);

        m_output->transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);


        auto view = glm::lookAt(glm::vec3(10, 4, 10), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
        auto proj = glm::perspective(glm::radians(45.0f), m_swap_chain.aspectRatio(), 0.1f, 1000.0f);

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

            m_material_data[i] = std::make_unique<re::UniformBuffer<Material>>(m_materials.size(), m_command_buffers);
            m_material_data[i]->update(m_materials.data());
        }

        RtObjDesc obj_desc {
            .vertex_addr = m_objects->mesh().vertex_buffer().address(),
            .index_addr = m_objects->mesh().index_buffer().address(),
        };

        m_obj_desc = std::make_unique<re::Buffer>(sizeof(RtObjDesc),
                                                  vk::BufferUsageFlagBits::eStorageBuffer,
                                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                  m_command_buffers);
        re::Buffer::set_data(&obj_desc, *m_obj_desc, m_command_buffers);

        for (auto i = 0; i < 2; i++)
        {
            vk::WriteDescriptorSetAccelerationStructureKHR as_info;
            as_info.setAccelerationStructureCount(1);
            as_info.setPAccelerationStructures(&m_accelerator.tlas.tlas);

            vk::DescriptorImageInfo image_info;
            image_info.setImageView(m_output->view());
            image_info.setImageLayout(vk::ImageLayout::eGeneral);

            auto ubo_info = DescriptorWrites::buffer_info<RtUniformData>(m_ubs[i]->buffer());
            auto obj_info = DescriptorWrites::buffer_info<RtObjDesc>(m_obj_desc->buffer());
            vk::DescriptorBufferInfo mat_info;
            mat_info.setBuffer(m_material_data[i]->buffer());
            mat_info.setRange(sizeof(Material) * m_materials.size());
            mat_info.setOffset(0);

            DescriptorWrites(m_device, *m_descriptors)
                .acceleration_structure(i, 0, as_info)
                .storage_image(i, 1, image_info)
                .uniform_buffer(i, 2, ubo_info)
                .storage_buffer(i, 3, obj_info)
                .uniform_buffer(i, 4, mat_info)
                .commit();
        }
    }

    void RayTracingScene::build_rt_pipeline()
    {
        vk::Result result;

        vk::PushConstantRange push_constant;
        push_constant.setOffset(0);
        push_constant.setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
        push_constant.setSize(sizeof(RtPushConstants));

        vk::PipelineLayoutCreateInfo pl_create_info;
        pl_create_info.setSetLayoutCount(1);
        pl_create_info.setPSetLayouts(&m_dsl);
        pl_create_info.setPushConstantRangeCount(1);
        pl_create_info.setPPushConstantRanges(&push_constant);

        result = m_device.handle().createPipelineLayout(&pl_create_info, nullptr, &m_pipeline_layout, m_device.dispatch());

        Shader sh_rgen("reflection.rgen.spv", vk::ShaderStageFlagBits::eRaygenKHR, m_device);
        Shader sh_miss("reflection.rmiss.spv", vk::ShaderStageFlagBits::eMissKHR,  m_device);
        Shader sh_chit("reflection.rchit.spv", vk::ShaderStageFlagBits::eClosestHitKHR, m_device);

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

    void RayTracingScene::rt_keybinds(GLFWwindow *window)
    {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) m_current_material = 0;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) m_current_material = 1;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) m_current_material = 2;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) m_current_material = 3;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) m_current_material = 4;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) m_current_material = 5;
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) m_current_material = 6;
    }
}
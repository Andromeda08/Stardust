#include "Raytracing.hpp"

namespace sd
{

    Raytracing::Raytracing(Swapchain &swapchain, const CommandBuffers &command_buffers)
    : Scene(swapchain, command_buffers)
    {
        vk::PhysicalDeviceProperties2 pdp;
        pdp.pNext = &m_rt_props;
        m_command_buffers.device().physicalDevice().getProperties2(&pdp, m_device.dispatch());

        generate_objects(1024);
        build_acceleration_structures();
        build_descriptors();

        auto reflection = PipelineBuilder(m_device)
            .add_descriptor_set_layout(m_rt_descriptors.layout)
            .add_push_constant({vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(RtPushConstantData)})
            .create_pipeline_layout()
            .add_shader("reflection.rgen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
            .add_shader("reflection.rmiss.spv", vk::ShaderStageFlagBits::eMissKHR)
            .add_shader("reflection.rchit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
            .make_rt_shader_groups()
            .create_ray_tracing_pipeline(2);

        m_materials.insert({ "reflection", reflection });

        build_sbt();
    }

    void Raytracing::trace_rays(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        update_camera(current_frame);
        RtAccelerator::rebuild_top_level(*m_cubes, m_accelerator, m_command_buffers);

        vk::WriteDescriptorSetAccelerationStructureKHR as_info;
        as_info.setAccelerationStructureCount(1);
        as_info.setPAccelerationStructures(&m_accelerator.tlas.tlas);

        DescriptorWrites(m_device, *m_rt_descriptors.sets)
            .acceleration_structure(current_frame, 0, as_info)
            .commit();

        std::vector<RtPushConstantData> pc = {{ m_current_material }};

        cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_materials["reflection"].pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_materials["reflection"].pipeline_layout, 0, 1,
                               &m_rt_descriptors.sets->get_set(current_frame), 0, nullptr);
        cmd.pushConstants(m_materials["reflection"].pipeline_layout, vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(RtPushConstantData), pc.data());

        auto sbt_slot_size = m_rt_props.shaderGroupHandleSize;
        auto miss_offset = round_up(sbt_slot_size, m_rt_props.shaderGroupBaseAlignment);
        auto hit_offset = round_up(miss_offset + sbt_slot_size, m_rt_props.shaderGroupBaseAlignment);

        vk::StridedDeviceAddressRegionKHR raygen_sbt;
        raygen_sbt.setDeviceAddress(m_shader_binding_table->address() + 0);
        raygen_sbt.setStride(sbt_slot_size);
        raygen_sbt.setSize(sbt_slot_size);

        vk::StridedDeviceAddressRegionKHR miss_sbt;
        miss_sbt.setDeviceAddress(m_shader_binding_table->address() + miss_offset);
        miss_sbt.setStride(sbt_slot_size);
        miss_sbt.setSize(sbt_slot_size);

        vk::StridedDeviceAddressRegionKHR chit_sbt;
        chit_sbt.setDeviceAddress(m_shader_binding_table->address() + hit_offset);
        chit_sbt.setStride(sbt_slot_size);
        chit_sbt.setSize(sbt_slot_size);

        vk::StridedDeviceAddressRegionKHR callable_sbt {};

        auto e = m_swapchain.extent();
        cmd.traceRaysKHR(&raygen_sbt, &miss_sbt, &chit_sbt, &callable_sbt,
                         e.width, e.height, 2, m_device.dispatch());

        blit(current_frame, cmd);
    }

    void Raytracing::blit(uint32_t current_frame, vk::CommandBuffer cmd)
    {
        vk::ImageSubresourceRange subresource_range { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        re::Image::set_layout(cmd, m_swapchain[current_frame],
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              subresource_range);

        re::Image::set_layout(cmd, m_output_image->image(),
                              vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal,
                              subresource_range);

        vk::Offset3D blit_size {
            static_cast<int32_t>(m_swapchain.extent().width),
            static_cast<int32_t>(m_swapchain.extent().height),
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

        cmd.blitImage(m_output_image->image(), vk::ImageLayout::eTransferSrcOptimal,
                      m_swapchain[current_frame], vk::ImageLayout::eTransferDstOptimal,
                      1, &blit_info, vk::Filter::eNearest);

        re::Image::set_layout(cmd, m_swapchain[current_frame],
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
                              subresource_range);

        re::Image::set_layout(cmd, m_output_image->image(),
                              vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral,
                              subresource_range);
    }

    void Raytracing::register_keybinds(GLFWwindow *window)
    {
        m_cameras[m_active_camera]->use_inputs(window);

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) m_current_material = 0;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) m_current_material = 1;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) m_current_material = 2;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) m_current_material = 3;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) m_current_material = 4;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) m_current_material = 5;
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) m_current_material = 6;
    }

    void Raytracing::generate_objects(uint32_t entities)
    {
        m_mats = { Materials::purple, Materials::obsidian, Materials::ruby, Materials::pearl,
                   Materials::gold, Materials::emerald, Materials::cyan };

        auto seed = static_cast<unsigned>(time(nullptr));
//        std::cout << "Seed: " << seed << std::endl;

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

        m_cubes = std::make_unique<re::InstancedGeometry>(new CubeGeometry(1.0f), m_instance_data, m_command_buffers);
    }

    void Raytracing::build_acceleration_structures()
    {
        m_accelerator = RtAccelerator::create_accelerator(*m_cubes, m_command_buffers);
    }

    void Raytracing::build_descriptors()
    {
        m_rt_descriptors.layout = DescriptorSetLayout()
            .accelerator(0, vk::ShaderStageFlagBits::eRaygenKHR)
            .storage_image(1, vk::ShaderStageFlagBits::eRaygenKHR)
            .uniform_buffer(2, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
            .storage_buffer(3, vk::ShaderStageFlagBits::eClosestHitKHR)
            .uniform_buffer(4, vk::ShaderStageFlagBits::eClosestHitKHR)
            .get_bindings(m_rt_descriptors.bindings)
            .create(m_device);

        m_rt_descriptors.sets = std::make_unique<DescriptorSets>(m_rt_descriptors.bindings, m_rt_descriptors.layout, m_device);

        m_output_image = std::make_unique<re::Image>(m_swapchain.extent(),
                                                     vk::Format::eR16G16B16A16Sfloat,
                                                     vk::ImageTiling::eOptimal,
                                                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                     vk::ImageAspectFlagBits::eColor,
                                                     m_command_buffers);

        m_output_image->transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        auto view = m_cameras[m_active_camera]->view();
        auto proj = m_cameras[m_active_camera]->projection();

        RtUniformData data = {
            .view_proj = view * proj,
            .view_inverse = glm::inverse(view),
            .proj_inverse = glm::inverse(proj),
        };

        m_uniform_camera.resize(2);
        m_uniform_material.resize(2);
        for (auto i = 0; i < 2; i++)
        {
            m_uniform_camera[i] = std::make_unique<CameraUB>(m_command_buffers);
            m_uniform_camera[i]->update(data);

            m_uniform_material[i] = std::make_unique<MaterialUB>(m_mats.size(), m_command_buffers);
            m_uniform_material[i]->update(m_mats.data());
        }

        RtObjBufferAddresses obj_desc {
            .vertex_addr = m_cubes->mesh().vertex_buffer().address(),
            .index_addr = m_cubes->mesh().index_buffer().address(),
        };

        m_obj_buffer_addresses = std::make_unique<re::Buffer>(sizeof(RtObjBufferAddresses),
                                                              vk::BufferUsageFlagBits::eStorageBuffer,
                                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                              m_command_buffers);
        re::Buffer::set_data(&obj_desc, *m_obj_buffer_addresses, m_command_buffers);

        for (auto i = 0; i < 2; i++)
        {
            vk::WriteDescriptorSetAccelerationStructureKHR as_info;
            as_info.setAccelerationStructureCount(1);
            as_info.setPAccelerationStructures(&m_accelerator.tlas.tlas);

            vk::DescriptorImageInfo image_info;
            image_info.setImageView(m_output_image->view());
            image_info.setImageLayout(vk::ImageLayout::eGeneral);

            auto ubo_info = DescriptorWrites::buffer_info<RtUniformData>(m_uniform_camera[i]->buffer());
            auto obj_info = DescriptorWrites::buffer_info<RtObjBufferAddresses>(m_obj_buffer_addresses->buffer());

            vk::DescriptorBufferInfo mat_info;
            mat_info.setBuffer(m_uniform_material[i]->buffer());
            mat_info.setRange(sizeof(Material) * m_mats.size());
            mat_info.setOffset(0);

            DescriptorWrites(m_device, *m_rt_descriptors.sets)
                .acceleration_structure(i, 0, as_info)
                .storage_image(i, 1, image_info)
                .uniform_buffer(i, 2, ubo_info)
                .storage_buffer(i, 3, obj_info)
                .uniform_buffer(i, 4, mat_info)
                .commit();
        }
    }

    void Raytracing::build_sbt()
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
        r = device.getRayTracingShaderGroupHandlesKHR(m_materials["reflection"].pipeline, 0, 1, sghs, data.data() + 0, d);
        r = device.getRayTracingShaderGroupHandlesKHR(m_materials["reflection"].pipeline, 1, 1, sghs, data.data() + miss, d);
        r = device.getRayTracingShaderGroupHandlesKHR(m_materials["reflection"].pipeline, 2, 1, sghs, data.data() + hit, d);
        m_shader_binding_table = std::make_unique<re::Buffer>(sbt_size,
                                                              vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eTransferDst |
                                                              vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
                                                              vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
                                                              m_command_buffers);

        re::Buffer::set_data(data.data(), *m_shader_binding_table, m_command_buffers);
    }

}
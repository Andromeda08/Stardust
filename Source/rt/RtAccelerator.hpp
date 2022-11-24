#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <Utility/Math.hpp>
#include <vk/Buffer.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Mesh.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>

struct RtAccelerator
{
    BlasInfo blas;
    TlasInfo tlas;
    std::unique_ptr<re::Buffer> instance_buffer;
    std::vector<vk::AccelerationStructureInstanceKHR> mapped_instance_buffer;

    static RtAccelerator create_accelerator(const re::InstancedGeometry& objects,
                                            const CommandBuffer& command_buffers)
    {
        auto& device = command_buffers.device();
        auto& d = device.dispatch();
        RtAccelerator accelerator;

        /* Create BLAS */
        accelerator.blas = BlasInfo::create_blas(objects.mesh(), command_buffers);

        /* Create instance data */
        auto& instance_data = objects.instance_data();
        auto instance_count = objects.instance_count();

        std::vector<vk::AccelerationStructureInstanceKHR> instances(instance_count);
        for (auto i = 0; i < instance_count; i++)
        {
            auto t = instance_data[i];
            //auto model = Math::model(t.translate, t.scale, t.r_axis, t.r_angle);
            auto model = glm::mat4(1.0f);
            model = glm::translate(model, t.translate);

            auto transform = Math::glmToKhr(model);

            //if (i == 0) std::cout << model << transform << std::endl;

            instances[i].setTransform(transform);

            instances[i].setInstanceCustomIndex(i);
            instances[i].setMask(0xff);
            instances[i].setInstanceShaderBindingTableRecordOffset(instance_data[i].hit_group);
            instances[i].setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instances[i].setAccelerationStructureReference(accelerator.blas.blas_address);
        }

        /* vk::AccelerationStructureInstanceKHR -> Instance buffer thingy */
#pragma region staging_to_instance_buffer
        vk::DeviceSize ibs = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        auto staging = re::Buffer::make_staging_buffer(ibs, command_buffers);

        re::Buffer::set_data(instances.data(), *staging, command_buffers);

        accelerator.instance_buffer = std::make_unique<re::Buffer>(ibs,
                                                                   vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                                   | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                                                   vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                                   command_buffers);

        re::Buffer::copy_buffer(*staging, *accelerator.instance_buffer, command_buffers);
#pragma endregion

        /* Build TLAS */
        accelerator.tlas = TlasInfo::create_tlas(instance_count, accelerator.instance_buffer->address(), command_buffers);

        return accelerator;
    }

    static void rebuild_top_level(re::InstancedGeometry& objects,
                                  RtAccelerator& accelerator,
                                  const CommandBuffer& command_buffers)
    {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();


        /* Create instance data */
        auto& instance_data = objects.instance_data();
        auto instance_count = objects.instance_count();

        std::vector<vk::AccelerationStructureInstanceKHR> instances(instance_count);
        for (auto i = 0; i < instance_count; i++)
        {
            auto t = instance_data[i];
            auto model = Math::model(t.translate, t.scale, t.r_axis, time * 30.0f);

            auto transform = Math::glmToKhr(model);

            //if (i == 0) std::cout << model << transform << std::endl;

            instances[i].setTransform(transform);

            instances[i].setInstanceCustomIndex(i);
            instances[i].setMask(0xff);
            instances[i].setInstanceShaderBindingTableRecordOffset(instance_data[i].hit_group);
            instances[i].setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instances[i].setAccelerationStructureReference(accelerator.blas.blas_address);
        }

        /* vk::AccelerationStructureInstanceKHR -> Instance buffer thingy */
#pragma region staging_to_instance_buffer
        vk::DeviceSize ibs = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        auto staging = re::Buffer::make_staging_buffer(ibs, command_buffers);

        re::Buffer::set_data(instances.data(), *staging, command_buffers);

        accelerator.instance_buffer = std::make_unique<re::Buffer>(ibs,
                                                                   vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                                   | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                                                   vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                                   command_buffers);

        re::Buffer::copy_buffer(*staging, *accelerator.instance_buffer, command_buffers);
#pragma endregion

        /* Build TLAS */
        accelerator.tlas = TlasInfo::create_tlas(instance_count, accelerator.instance_buffer->address(), command_buffers);
    }
};

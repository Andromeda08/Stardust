#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <vk/Buffer.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Utility/Math.hpp>
#include <vk/Mesh.hpp>

struct RtAccelerator
{
    BlasInfo blas;
    TlasInfo tlas;
    std::unique_ptr<re::Buffer> instance_buffer;
    std::vector<vk::AccelerationStructureInstanceKHR> mapped_instance_buffer;

    static RtAccelerator create_accelerator(const re::InstancedGeometry& objects,
                                            const CommandBuffer& command_buffers);
};

RtAccelerator RtAccelerator::create_accelerator(const re::InstancedGeometry& objects,
                                                const CommandBuffer &command_buffers)
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
    std::vector<vk::TransformMatrixKHR> transfroms(instance_count);

    for (auto i = 0; i < instance_count; i++)
    {
        transfroms.push_back(Math::glmToKhr(Math::model(instance_data[i].translate, instance_data[i].scale)));
        instances[i].setTransform(transfroms[i]);
        instances[i].setMask(0xff);
        instances[i].setInstanceShaderBindingTableRecordOffset(0);
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

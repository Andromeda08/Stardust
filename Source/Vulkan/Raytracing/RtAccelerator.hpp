#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "AccelerationStructure.hpp"
#include "../Buffer/Buffer.hpp"
#include "../Command/CommandBuffer.hpp"
#include "../../Math.hpp"
#include "../../Scene/Mesh.hpp"

struct RtAccelerator
{
    BlasInfo blas;
    TlasInfo tlas;
    std::unique_ptr<Buffer> instance_buffer;
    std::vector<vk::AccelerationStructureInstanceKHR> mapped_instance_buffer;

    static RtAccelerator create_accelerator(const Mesh& mesh,
                                            const CommandBuffer& command_buffers);
};

RtAccelerator RtAccelerator::create_accelerator(const Mesh& mesh,
                                                const CommandBuffer &command_buffers)
{
    auto& device = command_buffers.device();
    auto& d = device.dispatch();
    RtAccelerator accelerator;

    auto rt_start = std::chrono::high_resolution_clock::now();

    /* Create BLAS */
    accelerator.blas = BlasInfo::create_blas(mesh, command_buffers, d);

    /* Create instance data */
    auto& instance_data = mesh.instance_data();
    auto instance_count = mesh.instance_count();

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
    auto staging = Buffer::make_staging_buffer(ibs, device);

    void *data;
    vkMapMemory(device.handle(), staging.memory(), 0, ibs, 0, &data);
    memcpy(data, instances.data(), (size_t) ibs);
    vkUnmapMemory(device.handle(), staging.memory());

    accelerator.instance_buffer = std::make_unique<Buffer>(ibs,
                                                           vk::BufferUsageFlagBits::eTransferDst
                                                           | vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                           | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                                           vk::MemoryPropertyFlagBits::eDeviceLocal
                                                           | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                           device);

    Buffer::copy_buffer(command_buffers,
                        staging.handle(),
                        accelerator.instance_buffer->handle(),
                        ibs);
#pragma endregion

    /* Build TLAS */
    accelerator.tlas = TlasInfo::create_tlas(instance_count,
                                             accelerator.instance_buffer->address(),
                                             command_buffers);

    auto rt_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(rt_end - rt_start);
    std::cout << "Acceleration Structure build time: " << duration.count() << "ms\n"
              << "\tBottom Level : " << mesh.m_vertex_buffer->vertex_count() << " Vertices\n"
              << "\tTop Level    : " << instances.size() << " Instances" << std::endl;

    return accelerator;
}

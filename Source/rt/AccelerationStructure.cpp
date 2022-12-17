#include "AccelerationStructure.hpp"
#include <iostream>

BlasInfo BlasInfo::create_blas(const re::Mesh& mesh, const CommandBuffers& command_buffers)
{
    auto& d = command_buffers.device();
    auto device = command_buffers.device().handle();
    auto dispatch = command_buffers.device().dispatch();

    // Setup geometry data using the vertex and index buffers of the input Mesh
    vk::AccelerationStructureGeometryTrianglesDataKHR triangle_data;
    triangle_data.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangle_data.setVertexData(mesh.vertex_buffer().address());
    triangle_data.setVertexStride(sizeof(re::VertexData));
    triangle_data.setMaxVertex(mesh.vertex_buffer().count());
    triangle_data.setIndexType(mesh.index_buffer().type());
    triangle_data.setIndexData(mesh.index_buffer().address());

    vk::AccelerationStructureGeometryKHR geometry;
    // Build triangle geometry on blas
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry(triangle_data);

    // BLAS build information
    vk::AccelerationStructureBuildGeometryInfoKHR build_info;
    vk::AccelerationStructureBuildSizesInfoKHR    build_sizes;

    build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    build_info.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    build_info.setGeometryCount(1);
    build_info.setPGeometries(&geometry);

    const uint32_t triangle_count = mesh.index_buffer().count() / 3;
    device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                  &build_info,
                                                  &triangle_count,
                                                  &build_sizes,
                                                  dispatch);

    BlasInfo blas;
    blas.buffer = std::make_unique<re::Buffer>(build_sizes.accelerationStructureSize,
                                            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                            command_buffers);

    // Create acceleration structure
    vk::AccelerationStructureCreateInfoKHR create_info;
    create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    create_info.setBuffer(blas.buffer->buffer());
    create_info.setOffset(0);
    create_info.setSize(build_sizes.accelerationStructureSize);
    auto result = device.createAccelerationStructureKHR(&create_info,
                                                        nullptr,
                                                        &blas.blas,
                                                        dispatch);

    // Get BLAS address
    vk::AccelerationStructureDeviceAddressInfoKHR device_address_info;
    device_address_info.accelerationStructure = blas.blas;
    blas.blas_address = device.getAccelerationStructureAddressKHR(&device_address_info, dispatch);

    // Build BLAS
    auto scratch_buffer = std::make_unique<re::Buffer>(build_sizes.buildScratchSize,
                                                       vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                       vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                       command_buffers);

    build_info.setDstAccelerationStructure(blas.blas);
    build_info.setScratchData(scratch_buffer->address());

    vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
    build_range_info.setPrimitiveCount(mesh.index_buffer().count() / 3);
    const vk::AccelerationStructureBuildRangeInfoKHR* p_build_range_infos[1] = { &build_range_info };

    auto command_buffer = command_buffers.begin_single_time();
    command_buffer.buildAccelerationStructuresKHR(1,
                                                  &build_info,
                                                  p_build_range_infos,
                                                  dispatch);

    command_buffers.end_single_time(command_buffer);

    return blas;
}

TlasInfo TlasInfo::create_tlas(uint32_t instance_count,
                               vk::DeviceAddress instance_address,
                               const CommandBuffers& command_buffers)
{
    auto& device = command_buffers.device();
    auto dispatch = command_buffers.device().dispatch();

    vk::Device dh = device.handle();

    vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances;
    geometry_instances.setArrayOfPointers(false);
    geometry_instances.setData(instance_address);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry(geometry_instances);

    vk::AccelerationStructureBuildGeometryInfoKHR build_info;
    build_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    build_info.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    build_info.setGeometryCount(1);
    build_info.setPGeometries(&geometry);

    vk::AccelerationStructureBuildSizesInfoKHR build_sizes;
    dh.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                            &build_info,
                                            &instance_count,
                                            &build_sizes,
                                            dispatch);

    TlasInfo tlas;
    tlas.buffer = std::make_unique<re::Buffer>(build_sizes.accelerationStructureSize,
                                               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                               command_buffers);

    vk::AccelerationStructureCreateInfoKHR create_info;
    create_info.setBuffer(tlas.buffer->buffer());
    create_info.setOffset(0);
    create_info.setSize(build_sizes.accelerationStructureSize);
    create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    auto r = dh.createAccelerationStructureKHR(&create_info, nullptr, &tlas.tlas, dispatch);

    tlas.scratch_buffer = std::make_unique<re::Buffer>(build_sizes.buildScratchSize,
                                                   vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                   command_buffers);

    build_info.setDstAccelerationStructure(tlas.tlas);
    build_info.setScratchData(tlas.scratch_buffer->address());

    vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
    build_range_info.setPrimitiveCount(instance_count);
    const vk::AccelerationStructureBuildRangeInfoKHR* p_build_range_infos[1] = { &build_range_info };

    auto command_buffer = command_buffers.begin_single_time();
    command_buffer.buildAccelerationStructuresKHR(1,
                                                  &build_info,
                                                  p_build_range_infos,
                                                  dispatch);

    command_buffers.end_single_time(command_buffer);

    return tlas;
}
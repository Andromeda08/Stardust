#include "Raytracing.hpp"

 RaytracingBuilder::RaytracingBuilder(const Device& device,
                                      const CommandBuffers& cmds,
                                      uint32_t queue_index)
: m_device(device)
, m_cmds(cmds)
, m_queue_index(queue_index)
{
}

void RaytracingBuilder::build_blas(const std::vector<BlasInput>& input,
                                   vk::BuildAccelerationStructureFlagsKHR flags)
{
    auto blas_count = static_cast<uint32_t>(input.size());
    vk::DeviceSize as_size;
    vk::DeviceSize max_scratch_size;

    std::vector<BuildAccelerationStructure> build_as(blas_count);
    for (uint32_t i = 0; i < blas_count; i++)
    {
        build_as[i].as_geometry_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        build_as[i].as_geometry_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        build_as[i].as_geometry_info.setFlags(input[i].as_flags | flags);
        build_as[i].as_geometry_info.setGeometryCount(input[i].as_geometry.size());
        build_as[i].as_geometry_info.setPGeometries(input[i].as_geometry.data());

        build_as[i].as_range_info = input[i].as_range_info.data();

        std::vector<uint32_t> max_primitive_count(input[i].as_range_info.size());
        for (auto j = 0; j < input[i].as_range_info.size(); j++)
        {
            max_primitive_count[j] = input[i].as_range_info[i].primitiveCount;
        }

        as_size += build_as[i].as_size_info.accelerationStructureSize;
        max_scratch_size = std::max(max_scratch_size, build_as[i].as_size_info.buildScratchSize);
    }

    auto scratch_buffer = std::make_unique<Buffer>(max_scratch_size,
                                                   vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                   m_device);

    vk::BufferDeviceAddressInfo buffer_info;
    buffer_info.setBuffer(scratch_buffer->handle());
    vk::DeviceAddress scratch_address = m_device.handle().getBufferAddress(&buffer_info);

    vk::QueryPool query_pool;
    vk::QueryPoolCreateInfo qpci;
    qpci.setQueryCount(blas_count);
    qpci.setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
    auto result = m_device.handle().createQueryPool(&qpci, nullptr, &query_pool);

    uint32_t query_count { 0 };
    for (uint32_t i = 0; i < blas_count; i++)
    {
        auto cmd = m_cmds.begin_single_time();
        vk::AccelerationStructureCreateInfoKHR create_info;
        create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        create_info.setSize(build_as[i].as_size_info.accelerationStructureSize);
        m_device.handle().createAccelerationStructureKHR(&create_info, nullptr, &build_as[i].as_result.accel);

        build_as[i].as_geometry_info.setDstAccelerationStructure(build_as[i].as_result.accel);
        build_as[i].as_geometry_info.setScratchData(scratch_address);

        cmd.buildAccelerationStructuresKHR(1, &build_as[i].as_geometry_info, &build_as[i].as_range_info);
        vk::MemoryBarrier barrier;
        barrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR);
        barrier.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                            {}, 1, &barrier, 0, nullptr, 0, nullptr);

        cmd.writeAccelerationStructuresPropertiesKHR(1, &build_as[i].as_geometry_info.dstAccelerationStructure,
                                                     vk::QueryType::eAccelerationStructureSizeKHR,
                                                     query_pool,
                                                     query_count++);
    }
}

#include "Tlas.hpp"

namespace sdvk
{

    Tlas::Tlas(const std::vector<sd::Object>& objects, CommandBuffers const& command_buffers, Context const& context)
    : m_command_buffers(command_buffers), m_context(context)
    {
        create(objects);
    }

    void Tlas::build_instance_data(const std::vector<sd::Object>& objects)
    {
        std::vector<vk::AccelerationStructureInstanceKHR> instances(m_instance_count);
        for (int32_t i = 0; i < instances.size(); i++)
        {
            instances[i].setTransform(objects[i].transform.model3x4());
            instances[i].setMask(objects[i].rt_mask);
            instances[i].setInstanceShaderBindingTableRecordOffset(objects[i].rt_hit_group);
            instances[i].setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instances[i].setAccelerationStructureReference(objects[i].mesh->blas_address());
        }

        vk::DeviceSize instances_size = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        auto staging_buf = Buffer::Builder().with_size(instances_size).create_staging(m_context);

        m_instance_data = Buffer::Builder()
            .with_size(instances_size)
            .with_usage_flags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent)
            .create(m_context);

        staging_buf->set_data(instances.data(), m_context.device());
        m_command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            staging_buf->copy_to_buffer(*m_instance_data, cmd);
        });
    }

    void Tlas::build_top_level_as()
    {
        vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances_data;
        geometry_instances_data.setArrayOfPointers(false);
        geometry_instances_data.setData(m_instance_data->address());

        vk::AccelerationStructureGeometryKHR geometry;
        geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
        geometry.setGeometry(geometry_instances_data);

        vk::AccelerationStructureBuildGeometryInfoKHR build_info;
        build_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        build_info.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        build_info.setGeometryCount(1);
        build_info.setPGeometries(&geometry);

        vk::AccelerationStructureBuildSizesInfoKHR build_sizes;
        m_context.device().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &build_info, &m_instance_count, &build_sizes);

        m_buffer = Buffer::Builder()
            .with_size(build_sizes.accelerationStructureSize)
            .as_acceleration_structure_storage()
            .create(m_context);

        vk::AccelerationStructureCreateInfoKHR create_info;
        create_info.setBuffer(m_buffer->buffer());
        create_info.setOffset(0);
        create_info.setSize(build_sizes.accelerationStructureSize);
        create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        auto result = m_context.device().createAccelerationStructureKHR(&create_info, nullptr, &m_tlas);

        auto scratch_buf = Buffer::Builder().with_size(build_sizes.buildScratchSize).as_storage_buffer().create(m_context);

        build_info.setDstAccelerationStructure(m_tlas);
        build_info.setScratchData(scratch_buf->address());

        vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
        build_range_info.setPrimitiveCount(m_instance_count);
        const vk::AccelerationStructureBuildRangeInfoKHR* p_build_range_infos[1] = { &build_range_info };

        m_command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            cmd.buildAccelerationStructuresKHR(1, &build_info, p_build_range_infos);
        });
    }

    void Tlas::create(const std::vector<sd::Object>& objects)
    {
        m_instance_count = objects.size();
        build_instance_data(objects);
        build_top_level_as();
    }

    void Tlas::rebuild(const std::vector<sd::Object>& objects)
    {
        create(objects);
    }
}
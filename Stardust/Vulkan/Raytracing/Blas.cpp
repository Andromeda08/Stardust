#include "Blas.hpp"

namespace sdvk
{
    Blas::Builder& Blas::Builder::with_geometry(const std::shared_ptr<sd::Geometry>& geometry)
    {
        _geometry = std::shared_ptr<sd::Geometry>(geometry);
        return *this;
    }

    Blas::Builder& Blas::Builder::with_vertex_buffer(const std::shared_ptr<Buffer>& vertex_buffer)
    {
        _vertex_buffer = std::shared_ptr<Buffer>(vertex_buffer);
        return *this;
    }

    Blas::Builder& Blas::Builder::with_index_buffer(const std::shared_ptr<Buffer>& index_buffer)
    {
        _index_buffer = std::shared_ptr<Buffer>(index_buffer);
        return *this;
    }

    Blas::Builder& Blas::Builder::with_name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    std::unique_ptr<Blas> Blas::Builder::create(const CommandBuffers& command_buffers, const Context& context)
    {
        auto result = std::make_unique<Blas>(*_geometry, *_vertex_buffer, *_index_buffer, command_buffers, context);

        if (context.is_debug())
        {
            vk::DebugUtilsObjectNameInfoEXT name_info;
            name_info.setObjectHandle((uint64_t) static_cast<VkAccelerationStructureKHR>(result->blas()));
            name_info.setObjectType(vk::ObjectType::eAccelerationStructureKHR);
            name_info.setPObjectName(_name.c_str());
            auto r = context.device().setDebugUtilsObjectNameEXT(&name_info);
        }

        return result;
    }


    Blas::Blas(const sd::Geometry& geometry, const Buffer& vertex_buffer, const Buffer& index_buffer,
               const CommandBuffers& command_buffers, const Context& context)
    {
        vk::AccelerationStructureGeometryTrianglesDataKHR geometry_data;
        geometry_data.setVertexFormat(vk::Format::eR32G32B32Sfloat);
        geometry_data.setVertexData(vertex_buffer.address());
        geometry_data.setVertexStride(sizeof(sd::VertexData));
        geometry_data.setMaxVertex(geometry.vertex_count());
        geometry_data.setIndexData(index_buffer.address());
        geometry_data.setIndexType(vk::IndexType::eUint32);

        vk::AccelerationStructureGeometryKHR as_geo;
        as_geo.setGeometryType(vk::GeometryTypeKHR::eTriangles);
        as_geo.setGeometry(geometry_data);

        vk::AccelerationStructureBuildSizesInfoKHR as_sizes_info;
        vk::AccelerationStructureBuildGeometryInfoKHR as_geometry_info;
        as_geometry_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        as_geometry_info.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        as_geometry_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        as_geometry_info.setGeometryCount(1);
        as_geometry_info.setPGeometries(&as_geo);

        const uint32_t triangle_count = geometry.index_count() / 3;
        context.device().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                               &as_geometry_info, &triangle_count, &as_sizes_info);

        m_buffer = Buffer::Builder().with_size(as_sizes_info.accelerationStructureSize).as_acceleration_structure_storage().create(context);

        vk::AccelerationStructureCreateInfoKHR create_info;
        create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        create_info.setBuffer(m_buffer->buffer());
        create_info.setOffset(0);
        create_info.setSize(as_sizes_info.accelerationStructureSize);
        vk::Result vkr = context.device().createAccelerationStructureKHR(&create_info, nullptr, &m_blas);

        vk::AccelerationStructureDeviceAddressInfoKHR address_info;
        address_info.setAccelerationStructure(m_blas);
        m_address = context.device().getAccelerationStructureAddressKHR(&address_info);

        auto scratch_buf = Buffer::Builder().with_size(as_sizes_info.buildScratchSize).as_storage_buffer().create(context);
        as_geometry_info.setDstAccelerationStructure(m_blas);
        as_geometry_info.setScratchData(scratch_buf->address());

        vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
        build_range_info.setPrimitiveCount(triangle_count);
        const vk::AccelerationStructureBuildRangeInfoKHR* p_build_range_infos[1] = { &build_range_info };

        command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            cmd.buildAccelerationStructuresKHR(1, &as_geometry_info, p_build_range_infos);
        });
    }
}
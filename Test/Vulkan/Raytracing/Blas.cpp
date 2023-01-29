#include "Blas.hpp"

namespace sdvk
{

    Blas Blas::create_blas(const Mesh& mesh, const CommandBuffers& command_buffers, const Context& context)
    {
        vk::AccelerationStructureGeometryTrianglesDataKHR geometry_data;
        geometry_data.setVertexFormat(vk::Format::eR32G32B32Sfloat);
        geometry_data.setVertexData(mesh.vertex_buffer().address());
        geometry_data.setVertexStride(sizeof(sd::VertexData));
        geometry_data.setMaxVertex(mesh.geometry().vertex_count());
        geometry_data.setIndexData(mesh.index_buffer().address());
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

        const uint32_t triangle_count = mesh.geometry().index_count() / 3;
        context.device().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                               &as_geometry_info, &triangle_count, &as_sizes_info);

        Blas result;
        result.buffer = Buffer::Builder().with_size(as_sizes_info.accelerationStructureSize).as_acceleration_structure_storage().create(context);

        vk::AccelerationStructureCreateInfoKHR create_info;
        create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        create_info.setBuffer(result.buffer->buffer());
        create_info.setOffset(0);
        create_info.setSize(as_sizes_info.accelerationStructureSize);
        vk::Result vkr = context.device().createAccelerationStructureKHR(&create_info, nullptr, &result.blas);

        vk::AccelerationStructureDeviceAddressInfoKHR address_info;
        address_info.setAccelerationStructure(result.blas);
        result.address = context.device().getAccelerationStructureAddressKHR(&address_info);

        auto scratch_buf = Buffer::Builder().with_size(as_sizes_info.buildScratchSize).as_storage_buffer().create(context);
        as_geometry_info.setDstAccelerationStructure(result.blas);
    }
}
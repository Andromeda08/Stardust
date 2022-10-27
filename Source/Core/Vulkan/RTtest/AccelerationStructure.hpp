#pragma once

#include <vulkan/vulkan.hpp>
#include "../Buffer/Buffer.hpp"
#include "../Buffer/IndexBuffer.hpp"
#include "../Buffer/VertexBuffer.hpp"
#include "BlasInput.hpp"

struct AccelerationStructure
{
    vk::AccelerationStructureKHR accel;
    std::unique_ptr<Buffer>      buffer;
};

namespace rt
{
    auto to_vk_geometry(const Device& device,
                        const VertexBuffer& vtx,
                        const IndexBuffer& idx)
    {
        auto d = device.handle();
        auto vertex_addr = d.getBufferAddress(vtx.handle().handle());
        auto index_addr = d.getBufferAddress(idx.handle().handle());
        uint32_t max_primitive_count = idx.index_count() / 3;

        vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
        triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
        triangles.setVertexData(vertex_addr);
        triangles.setVertexStride(sizeof(Vertex));
        triangles.setIndexType(vk::IndexType::eUint32);
        triangles.setIndexData(index_addr);
        triangles.setMaxVertex(vtx.vertex_count());

        vk::AccelerationStructureGeometryKHR as_geom;
        as_geom.setGeometryType(vk::GeometryTypeKHR::eInstances);
        as_geom.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        as_geom.setGeometry(triangles);

        vk::AccelerationStructureBuildRangeInfoKHR offset;
        offset.setFirstVertex(0);
        offset.setPrimitiveCount(max_primitive_count);
        offset.setPrimitiveCount(0);
        offset.setTransformOffset(0);

        BlasInput input;
        input.as_geometry.emplace_back(as_geom);
        input.as_range_info.emplace_back(offset);

        return input;
    }
}
#include "Mesh.hpp"

namespace sdvk
{
    Mesh::Mesh(sd::Geometry* p_geometry, const CommandBuffers& command_buffers, const Context& context, const std::string& name)
    : m_geometry(p_geometry)
    {
        m_vertex_buffer = sdvk::Buffer::Builder()
            .with_name("[VtxB] " + name)
            .with_size(sizeof(sd::VertexData) * m_geometry->vertices().size())
            .as_vertex_buffer()
            .create_with_data(m_geometry->vertices().data(), command_buffers, context);

        m_index_buffer = sdvk::Buffer::Builder()
            .with_name("[IdxB] " + name)
            .with_size(sizeof(uint32_t) * m_geometry->indices().size())
            .as_index_buffer()
            .create_with_data(m_geometry->indices().data(), command_buffers, context);
    }

    void Mesh::draw(const vk::CommandBuffer& command_buffer) const
    {
        static const std::vector<vk::DeviceSize> offsets = { 0 };
        command_buffer.bindVertexBuffers(0, 1, &m_vertex_buffer->buffer(), offsets.data());
        command_buffer.bindIndexBuffer(m_index_buffer->buffer(), 0, vk::IndexType::eUint32);
        command_buffer.drawIndexed(m_geometry->indices().size(), 1, 0, 0, 0);
    }
}

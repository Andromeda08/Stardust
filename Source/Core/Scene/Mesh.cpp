#include "Mesh.hpp"

#include <utility>

Mesh::Mesh(Geometry *geometry, std::vector<IData> instances, const vk::Pipeline& pipeline,
           const Device& device, const CommandBuffers& cmds)
    : m_pipeline(pipeline), m_device(device), m_geometry(geometry), m_instances(std::move(instances)) {
    m_index_buffer = std::make_unique<IndexBuffer>(m_geometry->indices(), cmds, device);
    m_vertex_buffer = std::make_unique<VertexBuffer>(m_geometry->vertices(), cmds, device);
    m_instance_buffer = std::make_unique<InstanceBuffer>(m_instances, cmds, device);
}

void Mesh::draw(vk::CommandBuffer &cmd_buffer) {
    std::vector<vk::DeviceSize> offsets = { 0 };
    std::vector<vk::Buffer> vertex_buffer = {m_vertex_buffer->buffer().handle()};
    cmd_buffer.bindVertexBuffers(0, 1, vertex_buffer.data(), offsets.data());
    cmd_buffer.bindVertexBuffers(1, 1, &m_instance_buffer->handle().handle(), offsets.data());
    cmd_buffer.bindIndexBuffer(m_index_buffer->buffer().handle(), 0, vk::IndexType::eUint32);
    cmd_buffer.drawIndexed(m_index_buffer->index_count(), m_instances.size(), 0, 0,0);
}
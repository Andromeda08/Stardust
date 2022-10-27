#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Vertex.hpp"

class VertexBuffer
{
public:
    NON_COPIABLE(VertexBuffer)

    explicit VertexBuffer(const std::vector<Vertex>& vertices,
                          const CommandBuffers& cmd_buffers,
                          const Device& device);

    uint32_t vertex_count() const { return m_vertices.size(); }

    const Buffer& handle() const { return *m_buffer; }
    const Buffer& buffer() const { return *m_buffer; }

    vk::DeviceAddress address() const { return m_device.handle().getBufferAddress(m_buffer->handle()); }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<Vertex>& m_vertices;

    const Device& m_device;
};

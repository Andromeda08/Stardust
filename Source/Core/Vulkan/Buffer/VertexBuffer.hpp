#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Vertex.hpp"

class VertexBuffer
{
public:
    explicit VertexBuffer(const std::vector<Vertex>& vertices,
                          const CommandBuffers& cmd_buffers,
                          const Device& device);

    const Buffer& handle() const { return *m_buffer; }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<Vertex>& m_vertices;

    const Device& m_device;
};

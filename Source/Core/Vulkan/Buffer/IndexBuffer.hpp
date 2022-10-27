#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Vertex.hpp"

class IndexBuffer
{
public:
    NON_COPIABLE(IndexBuffer)

    explicit IndexBuffer(const std::vector<uint32_t>& indices,
                         const CommandBuffers& cmd_buffers,
                         const Device& device);

    uint32_t index_count() const { return static_cast<uint32_t>(m_indices.size()); }

    const Buffer& handle() const { return *m_buffer; }
    const Buffer& buffer() const { return *m_buffer; }

    vk::DeviceAddress address() const { return m_device.handle().getBufferAddress(m_buffer->handle()); }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<uint32_t> m_indices;

    const Device& m_device;
};

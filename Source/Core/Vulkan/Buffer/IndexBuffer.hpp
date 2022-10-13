#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Vertex.hpp"


class IndexBuffer
{
public:
    explicit IndexBuffer(const std::vector<uint32_t>& indices,
                         const CommandBuffers& cmd_buffers,
                         const Device& device);

    const Buffer& handle() const { return *m_buffer; }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<uint32_t>& m_indices;

    const Device& m_device;
};

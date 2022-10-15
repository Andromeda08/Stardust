#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Geometry.hpp"
#include "../../Resources/Vertex.hpp"

class InstanceBuffer
{
public:
    NON_COPIABLE(InstanceBuffer)

    InstanceBuffer(const std::vector<InstanceData>& data,
                   const CommandBuffers& cmd_buffers,
                   const Device& device);

    const Buffer& handle() const { return *m_buffer; }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<InstanceData>& m_data;

    const Device& m_device;
};

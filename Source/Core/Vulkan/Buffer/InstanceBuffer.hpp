#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Resources/Geometry.hpp"
#include "../../Resources/Vertex.hpp"
#include "../../Scene/IData.hpp"

class InstanceBuffer
{
public:
    explicit InstanceBuffer(const std::vector<IData>& data, const CommandBuffers& cmd_buffers, const Device& device);

    const Buffer& handle() const { return *m_buffer; }

    const Buffer& buffer() const { return *m_buffer; }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const std::vector<IData>& m_data;

    const Device& m_device;
};

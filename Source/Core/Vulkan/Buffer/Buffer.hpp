#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../Command/CommandBuffers.hpp"

class Buffer
{
public:
    NON_COPIABLE(Buffer)

    Buffer(vk::DeviceSize buffer_size,
           vk::BufferUsageFlags usage_flags,
           vk::MemoryPropertyFlags property_flags,
           const Device& device);

    ~Buffer();

    static Buffer make_staging_buffer(vk::DeviceSize buffer_size,
                                      const Device& device)
    {
        return { buffer_size,
                 vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 device };
    }

    static void copy_buffer(const CommandBuffers& cmd_buffers,
                            vk::Buffer src,
                            vk::Buffer dst,
                            vk::DeviceSize buffer_size);

    vk::Buffer handle() const { return m_buffer; }

    vk::DeviceMemory memory() const { return m_memory; }

    const Device& device() const { return m_device; }

private:
    vk::Buffer m_buffer;
    vk::DeviceMemory m_memory;

    const Device& m_device;
};

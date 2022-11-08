#include "InstanceBuffer.hpp"

InstanceBuffer::InstanceBuffer(const std::vector<IData>& instance_data,
                               const CommandBuffers &cmd_buffers,
                               const Device &device)
: m_device(device)
, m_data(instance_data)
{
    vk::DeviceSize buffer_size = sizeof(IData) * instance_data.size();

    auto staging = Buffer::make_staging_buffer(buffer_size, m_device);

    void* data;
    vkMapMemory(m_device.handle(), staging.memory(), 0, buffer_size, 0, &data);
    memcpy(data, instance_data.data(), (size_t) buffer_size);
    vkUnmapMemory(m_device.handle(), staging.memory());

    m_buffer = std::make_unique<Buffer>(buffer_size,
                                        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                        m_device);

    Buffer::copy_buffer(cmd_buffers, staging.handle(), m_buffer->handle(), buffer_size);
}

#include "UniformBuffer.hpp"

UniformBuffer::UniformBuffer(const Device &device)
: m_device(device)
{
    m_buffer = std::make_unique<Buffer>(sizeof(UniformBufferObject),
                                        vk::BufferUsageFlagBits::eUniformBuffer,
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                        m_device);
}

UniformBuffer::UniformBuffer(const UniformBufferObject& ubo,
                             const Device& device)
: m_device(device)
{
    m_buffer = std::make_unique<Buffer>(sizeof(UniformBufferObject),
                                        vk::BufferUsageFlagBits::eUniformBuffer,
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                        m_device);
    update(ubo);
}

void UniformBuffer::update(const UniformBufferObject& ubo) const
{
    void* data;
    vkMapMemory(m_device.handle(), m_buffer->memory(), 0, sizeof(UniformBufferObject), 0, &data);
    std::memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(m_device.handle(), m_buffer->memory());
}

#include "CommandBuffers.hpp"

CommandBuffers::CommandBuffers(const Device& device, uint32_t size)
: m_device(device)
{
    m_pool = std::make_unique<CommandPool>(m_device, device.graphics_index());

    m_buffers.resize(size);
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandBufferCount(size);
    alloc_info.setCommandPool(m_pool->handle());
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    auto result = m_device.handle().allocateCommandBuffers(&alloc_info, m_buffers.data());
}

vk::CommandBuffer CommandBuffers::begin(size_t index)
{
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    vkBeginCommandBuffer(m_buffers[index], reinterpret_cast<const VkCommandBufferBeginInfo*>(&begin_info));
    return m_buffers[index];
}

void CommandBuffers::end(size_t index)
{
    vkEndCommandBuffer(m_buffers[index]);
}


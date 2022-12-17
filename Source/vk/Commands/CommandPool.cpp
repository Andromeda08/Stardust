#include "CommandPool.hpp"

CommandPool::CommandPool(const Device& device)
: m_device(device)
{
    vk::CommandPoolCreateInfo create_info;
    create_info.setQueueFamilyIndex(device.graphics_index());
    create_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    auto result = device.handle().createCommandPool(&create_info, nullptr, &m_command_pool);
}

CommandPool::~CommandPool()
{
    m_device.handle().destroy(m_command_pool);
}

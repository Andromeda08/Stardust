#include "CommandPool.hpp"

CommandPool::CommandPool(const Device& device, uint32_t queue_family_index)
: m_device(device)
{
    vk::CommandPoolCreateInfo create_info;
    create_info.setQueueFamilyIndex(queue_family_index);
    create_info.setPNext(nullptr);
    create_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    auto result = m_device.handle().createCommandPool(&create_info, nullptr, &m_handle);
}

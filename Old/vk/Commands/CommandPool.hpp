#pragma once

#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>

/**
 * @brief The class represents a command pool.
 */
class CommandPool
{
public:
    explicit CommandPool(Device const&);

    ~CommandPool();

    const vk::CommandPool& handle() const { return m_command_pool; }

private:
    vk::CommandPool m_command_pool;
    Device const&   m_device;
};

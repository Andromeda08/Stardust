#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Macro.hpp"

class CommandPool
{
public:
    NON_COPIABLE(CommandPool)

    CommandPool(const Device& device, uint32_t queue_family_index);

    vk::CommandPool handle() const { return m_handle; }

    const Device& device() const { return m_device; }

private:
    vk::CommandPool m_handle;
    const Device&   m_device;
};

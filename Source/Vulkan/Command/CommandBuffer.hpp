#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "CommandPool.hpp"
#include "../Device.hpp"
#include "../../Utility/Macro.hpp"

class CommandBuffer {
public:
    NON_COPIABLE(CommandBuffer)

    explicit CommandBuffer(const Device& device, uint32_t size = 16);

    vk::CommandBuffer begin(size_t index) const;

    void end(size_t index) const;

    vk::CommandBuffer begin_single_time() const;

    void end_single_time(vk::CommandBuffer cmd_buffer) const;

    vk::CommandBuffer get_buffer(size_t index) const { return m_buffers[index]; }

    uint32_t size() const { return static_cast<uint32_t>(m_buffers.size()); }

    const Device& device() const { return m_device; }

private:
    // Default pool
    std::unique_ptr<CommandPool>   m_pool;
    std::vector<vk::CommandBuffer> m_buffers;

    // Single time pool
    std::unique_ptr<CommandPool>   m_pool_single_time;

    const Device& m_device;
};

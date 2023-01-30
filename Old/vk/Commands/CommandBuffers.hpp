#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>
#include "CommandPool.hpp"

/**
 * @brief This class provides command buffers for N (swapchain) images and single time commands.
 */
class CommandBuffers {
public:
    explicit CommandBuffers(const Device& device, uint32_t images = 2);

    /**
     * @brief Begin the recording of a single time command buffer.
     */
    vk::CommandBuffer begin_single_time() const;

    /**
     * @brief End the recording of the specified command buffer, submit it, then free it.
     * @param command_buffer Single time command buffer
     */
    void end_single_time(vk::CommandBuffer command_buffer) const;

    /**
     * @brief Access command buffer at the specified index.
     */
    const vk::CommandBuffer& get_buffer(uint32_t index) const;

    /**
     * @brief Provide access to command buffers using the subscript operator.
     */
    const vk::CommandBuffer& operator[](uint32_t index) const;

    const Device& device() const { return m_device; }

private:
    std::shared_ptr<CommandPool>   m_pool;
    std::vector<vk::CommandBuffer> m_buffers;

    const Device& m_device;
};

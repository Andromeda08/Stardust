#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "CommandPool.hpp"
#include "../Device.hpp"
#include "../../Macro.hpp"

class CommandBuffers {
public:
    NON_COPIABLE(CommandBuffers)

    explicit CommandBuffers(const Device& device, uint32_t size = 16);

    vk::CommandBuffer begin(size_t index);

    void end(size_t index);

    vk::CommandBuffer get(size_t index) { return m_buffers[index]; }

    vk::CommandBuffer operator[](size_t index) { return get(index); }

    uint32_t size() const { return static_cast<uint32_t>(m_buffers.size()); }

private:
    std::unique_ptr<CommandPool>   m_pool;
    std::vector<vk::CommandBuffer> m_buffers;

    const Device& m_device;
};

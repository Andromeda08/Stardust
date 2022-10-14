#include "CommandBuffers.hpp"

CommandBuffers::CommandBuffers(const Device& device, uint32_t size)
: m_device(device)
{
    m_pool = std::make_unique<CommandPool>(m_device, device.graphics_index());
    m_pool_single_time = std::make_unique<CommandPool>(m_device, device.graphics_index());

    m_buffers.resize(size);
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandBufferCount(size);
    alloc_info.setCommandPool(m_pool->handle());
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    auto result = m_device.handle().allocateCommandBuffers(&alloc_info, m_buffers.data());
}

vk::CommandBuffer CommandBuffers::begin(size_t index) const
{
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    m_buffers[index].begin(&begin_info);
    return m_buffers[index];
}

void CommandBuffers::end(size_t index) const
{
    vkEndCommandBuffer(m_buffers[index]);
}

vk::CommandBuffer CommandBuffers::begin_single_time() const
{
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandPool(m_pool_single_time->handle());
    alloc_info.setCommandBufferCount(1);

    vk::CommandBuffer cmd_buffer;
    auto result = m_device.handle().allocateCommandBuffers(&alloc_info, &cmd_buffer);

    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    result = cmd_buffer.begin(&begin_info);

    return cmd_buffer;
}

void CommandBuffers::end_single_time(vk::CommandBuffer cmd_buffer) const
{
    cmd_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&cmd_buffer);

    auto result = m_device.graphics_queue().submit(1, &submit_info, nullptr);
    m_device.graphics_queue().waitIdle();

    m_device.handle().freeCommandBuffers(m_pool_single_time->handle(), 1, &cmd_buffer);
}


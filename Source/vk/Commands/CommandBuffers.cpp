#include "CommandBuffers.hpp"

CommandBuffers::CommandBuffers(const Device& device, uint32_t images)
: m_device(device)
{
    m_pool = std::make_shared<CommandPool>(m_device);

    m_buffers.resize(images);
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandBufferCount(images);
    alloc_info.setCommandPool(m_pool->handle());
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    auto result = m_device.handle().allocateCommandBuffers(&alloc_info, m_buffers.data());
}

vk::CommandBuffer CommandBuffers::begin_single_time() const
{
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandPool(m_pool->handle());
    alloc_info.setCommandBufferCount(1);

    vk::CommandBuffer cmd_buffer;
    auto result = m_device.handle().allocateCommandBuffers(&alloc_info, &cmd_buffer);

    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    result = cmd_buffer.begin(&begin_info);

    return cmd_buffer;
}

void CommandBuffers::end_single_time(vk::CommandBuffer command_buffer) const
{
    command_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&command_buffer);

    auto result = m_device.graphics_queue().submit(1, &submit_info, nullptr);
    m_device.graphics_queue().waitIdle();

    m_device.handle().freeCommandBuffers(m_pool->handle(), 1, &command_buffer);
}

const vk::CommandBuffer& CommandBuffers::get_buffer(uint32_t index) const
{
    if (index > m_buffers.size())
    {
        throw std::out_of_range("Swapchain image index \"" + std::to_string(index) + "\" out of bounds.");
    }
    m_buffers[index].reset();
    return m_buffers[index];
}

const vk::CommandBuffer& CommandBuffers::operator[](uint32_t index) const
{
    return get_buffer(index);
}

#include "CommandBuffers.hpp"

namespace sdvk
{
    CommandBuffers::CommandBuffers(uint32_t buffer_count, const Context& context)
    : m_ctx(context)
    {
        spawn_pool();

        {
            m_buffers.resize(buffer_count);
            vk::CommandBufferAllocateInfo allocate_info;
            allocate_info.setCommandBufferCount(buffer_count);
            allocate_info.setCommandPool(m_pool);
            allocate_info.setLevel(vk::CommandBufferLevel::ePrimary);
            vk::Result result = m_ctx.device().allocateCommandBuffers(&allocate_info, m_buffers.data());

            m_in_use.resize(buffer_count);
            for (auto b : m_in_use)
            {
                b = false;
            }
        }
    }

    void CommandBuffers::spawn_pool()
    {
        vk::CommandPoolCreateInfo create_info;
        create_info.setQueueFamilyIndex(m_ctx.q_graphics().index);
        create_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        vk::Result result = m_ctx.device().createCommandPool(&create_info, nullptr, &m_pool);
    }

    void CommandBuffers::execute_single_time(const std::function<void(const vk::CommandBuffer&)>& commands) const
    {
        vk::CommandBufferAllocateInfo allocate_info;
        allocate_info.setLevel(vk::CommandBufferLevel::ePrimary);
        allocate_info.setCommandPool(m_pool);
        allocate_info.setCommandBufferCount(1);

        vk::CommandBuffer buffer;
        vk::Result result = m_ctx.device().allocateCommandBuffers(&allocate_info, &buffer);

        vk::CommandBufferBeginInfo begin_info;
        begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        result = buffer.begin(&begin_info);

        commands(buffer);

        buffer.end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBufferCount(1);
        submit_info.setPCommandBuffers(&buffer);
        result = m_ctx.q_graphics().queue.submit(1, &submit_info, nullptr);

        m_ctx.q_graphics().queue.waitIdle();

        m_ctx.device().freeCommandBuffers(m_pool, 1, &buffer);
    }

    const vk::CommandBuffer& CommandBuffers::get(uint32_t id) const
    {
        if (id > m_buffers.size())
        {
            throw std::out_of_range("CommandBuffer index " + std::to_string(id) + " out of range.");
        }

        m_buffers[id].reset();
        return m_buffers[id];
    }

    const vk::CommandBuffer& CommandBuffers::operator[](uint32_t id) const
    {
        return get(id);
    }

    const vk::CommandBuffer& CommandBuffers::begin(uint32_t id) const
    {
        vk::CommandBufferBeginInfo begin_info;
        auto result = get(id).begin(&begin_info);
        return m_buffers[id];
    }
}
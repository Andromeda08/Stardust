#pragma once

#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace sdvk
{
    class CommandBuffers
    {
    public:
        CommandBuffers(uint32_t buffer_count, const Context& context);

        void execute_single_time(const std::function<void(vk::CommandBuffer const&)>& commands) const;

        const vk::CommandBuffer& begin(uint32_t id) const;

        const vk::CommandBuffer& get(uint32_t id) const;

        const vk::CommandBuffer& operator[](uint32_t id) const;

    private:
        void spawn_pool();

    private:
        vk::CommandPool m_pool;

        std::vector<bool> m_in_use;
        std::vector<vk::CommandBuffer> m_buffers;

        const Context& m_ctx;
    };
}

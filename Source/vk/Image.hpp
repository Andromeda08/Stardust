#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>

namespace re
{
    class vkImage
    {
    public:
        vkImage(vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
                vk::ImageUsageFlags usage_flags, vk::MemoryPropertyFlags property_flags,
                const CommandBuffer& command_buffers);

        /**
         * @brief Transitions the layout of the specified image from A to B.
         * @deprecated Restrictive, doesn't support many transitions.
         */
        void transition_layout(vk::ImageLayout from, vk::ImageLayout to);

        /**
         * @brief Transitions the layout of the specified image from A to B.
         */
        static void set_layout(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                               vk::ImageSubresourceRange subresource_range,
                               vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eAllCommands);

        vk::Extent2D extent() const { return m_extent; }

        vk::Format format() const { return m_format; }

        const vk::Image& image() const { return m_image; }

        const vk::ImageView& view() const { return m_view; }

    private:
        vk::Extent2D   m_extent;
        vk::Format     m_format;

        vk::Image        m_image;
        vk::DeviceMemory m_memory;
        vk::ImageView    m_view;

        const CommandBuffer& m_command_buffers;
    };
}
#pragma once

#include <vulkan/vulkan.hpp>
#include <vk/Commands/CommandBuffers.hpp>

namespace re
{
    class Image
    {
    public:
        Image(vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
              vk::ImageUsageFlags usage_flags, vk::MemoryPropertyFlags property_flags,
              vk::ImageAspectFlags aspect_flags, const CommandBuffers& command_buffers);

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

        const CommandBuffers& m_command_buffers;
    };

    class DepthBuffer : public Image
    {
    public:
        DepthBuffer(vk::Extent2D extent, const CommandBuffers& command_buffers)
            : Image(extent,
                    find_depth_format(command_buffers.device()),
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    vk::ImageAspectFlagBits::eDepth,
                    command_buffers) {}

    private:
        static vk::Format find_depth_format(const Device& device,
                                            const std::vector<vk::Format>& candidates = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint })
        {
            for (vk::Format format : candidates)
            {
                auto props = device.physicalDevice().getFormatProperties(format);

                if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
                    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
                {
                    return format;
                }
            }

            throw std::runtime_error("Failed to find supported format for depth buffer.");
        }

    };
}
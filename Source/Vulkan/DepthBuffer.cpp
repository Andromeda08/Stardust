#include "DepthBuffer.hpp"

#include <stdexcept>

vk::Format find_supported_format(const Device& device,
                                 const std::vector<vk::Format>& candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        auto props = device.physicalDevice().getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear
            && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if (tiling == vk::ImageTiling::eOptimal
            && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("[DepthBuffer] Failed to find supported format.");
}

DepthBuffer::DepthBuffer(vk::Extent2D extent,
                         const Device& device,
                         const CommandBuffer& cmd_buffers)
: m_device(device)
{
    m_format      = find_depth_format();
    m_depth_image = Image::make_depth_buffer_image(extent, m_format, device, cmd_buffers);
    m_depth_view  = std::make_unique<ImageView>(device, m_depth_image->image(), m_format, vk::ImageAspectFlagBits::eDepth);
}

vk::Format DepthBuffer::find_depth_format()
{
    return find_supported_format(m_device,
                                 { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
                                 vk::ImageTiling::eOptimal,
                                 vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

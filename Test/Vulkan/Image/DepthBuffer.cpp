#include "DepthBuffer.hpp"

namespace sdvk
{
    DepthBuffer::DepthBuffer(vk::Extent2D extent, const Context& context)
    : Image(extent,
            find_depth_format(context.physical_device()),
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageAspectFlagBits::eDepth,
            vk::ImageTiling::eOptimal,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            context) {}

    vk::Format DepthBuffer::find_depth_format(const vk::PhysicalDevice& physical_device)
    {
        static const std::vector<vk::Format> candidates = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
        for (auto format : candidates)
        {
            auto format_props = physical_device.getFormatProperties(format);

            if ((format_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
                vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
                return format;
            }
        }
        throw std::runtime_error("Failed to find a supported depth buffer format.");
    }
}
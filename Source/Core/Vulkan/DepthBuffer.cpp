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
        else if (tiling == vk::ImageTiling::eOptimal
                 && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("[DepthBuffer] Failed to find supported format.");
}

vk::Format find_depth_format(const Device& device)
{
    return find_supported_format(device,
                                 { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
                                 vk::ImageTiling::eOptimal,
                                 vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void transitionImageLayout(const CommandBuffers& cmd_buffers,
                           vk::Image image,
                           vk::Format format,
                           vk::ImageLayout old_layout,
                           vk::ImageLayout new_layout)
{
    auto cmd = cmd_buffers.begin(2);
    vk::ImageSubresourceRange srr;
    srr.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
    srr.setBaseArrayLayer(0);
    srr.setLayerCount(1);
    srr.setBaseMipLevel(0);
    srr.setLevelCount(1);

    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(old_layout);
    barrier.setNewLayout(new_layout);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image),
    barrier.setSubresourceRange(srr);

    vk::PipelineStageFlagBits source_stage;
    vk::PipelineStageFlagBits destination_stage;

    if (old_layout == vk::ImageLayout::eUndefined
        && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.setSrcAccessMask({});
        barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else
    {
        throw std::invalid_argument(":D");
    }

    cmd.pipelineBarrier(source_stage, destination_stage,
                        {},
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

    cmd.end();
    cmd.reset();
}

DepthBuffer::DepthBuffer(vk::Extent2D extent,
                         const Device& device,
                         const CommandBuffers& cmd_buffers)
{
    vk::Result result;
    auto format = find_depth_format(device);
    m_format = format;

    vk::ImageCreateInfo image_ci;
    image_ci.setImageType(vk::ImageType::e2D);
    image_ci.setExtent({ extent.width, extent.height, 1 });
    image_ci.setMipLevels(1);
    image_ci.setArrayLayers(1);
    image_ci.setFormat(format);
    image_ci.setTiling(vk::ImageTiling::eOptimal);
    image_ci.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    image_ci.setSharingMode(vk::SharingMode::eExclusive);
    image_ci.setSamples(vk::SampleCountFlagBits::e1);
    result = device.handle().createImage(&image_ci, nullptr, &m_depth_image);

    auto mem_reqs = device.handle().getImageMemoryRequirements(m_depth_image);
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.setAllocationSize(mem_reqs.size);
    alloc_info.setMemoryTypeIndex(device.findMemoryType(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    result = device.handle().allocateMemory(&alloc_info, nullptr, &m_depth_memory);
    device.handle().bindImageMemory(m_depth_image, m_depth_memory, 0);

    vk::ImageSubresourceRange srr;
    srr.setAspectMask(vk::ImageAspectFlagBits::eDepth);
    srr.setBaseArrayLayer(0);
    srr.setLayerCount(1);
    srr.setBaseMipLevel(0);
    srr.setLevelCount(1);

    vk::ImageViewCreateInfo view_ci;
    view_ci.setImage(m_depth_image);
    view_ci.setViewType(vk::ImageViewType::e2D);
    view_ci.setFormat(format);
    view_ci.setSubresourceRange(srr);
    result = device.handle().createImageView(&view_ci, nullptr, &m_depth_view);

    transitionImageLayout(cmd_buffers,
                          m_depth_image,
                          format,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
}
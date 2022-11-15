#include "Image.hpp"

void Image::transition_image_layout(vk::ImageLayout from, vk::ImageLayout to)
{
    auto cmd = m_cmd_buffers.begin_single_time();


    m_cmd_buffers.end_single_time(cmd);
}

Image::Image(vk::Extent2D          extent,
             vk::Format            format,
             vk::ImageUsageFlags   usage,
             const Device&         device,
             const CommandBuffer& cmd_buffers)
: m_device(device)
, m_cmd_buffers(cmd_buffers)
, m_format(format)
{
    vk::Result result;

    vk::ImageCreateInfo create_info;

    create_info.setExtent({ extent.width, extent.height, 1 });
    create_info.setFormat(format);
    create_info.setUsage(usage);

    create_info.setImageType(vk::ImageType::e2D);
    create_info.setMipLevels(1);
    create_info.setArrayLayers(1);
    create_info.setTiling(vk::ImageTiling::eOptimal);
    create_info.setSharingMode(vk::SharingMode::eExclusive);
    create_info.setSamples(vk::SampleCountFlagBits::e1);

    result = device.handle().createImage(&create_info, nullptr, &m_image);

    auto mem_reqs = device.handle().getImageMemoryRequirements(m_image);

    vk::MemoryAllocateInfo alloc_info;

    alloc_info.setAllocationSize(mem_reqs.size);
    alloc_info.setMemoryTypeIndex(device.findMemoryType(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

    result = m_device.handle().allocateMemory(&alloc_info, nullptr, &m_memory);

    m_device.handle().bindImageMemory(m_image, m_memory, 0);
}

std::unique_ptr<Image>
Image::make_depth_buffer_image(vk::Extent2D extent, vk::Format format, const Device& device, const CommandBuffer& cmd_buffers)
{
    return std::make_unique<Image>(extent,
                                   format,
                                   vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                   device,
                                   cmd_buffers);
}

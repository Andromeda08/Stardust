#include "Image.hpp"

#include <sstream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>
#include <Vulkan/Device.hpp>

namespace re
{
    vkImage::vkImage(vk::Extent2D extent,vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage_flags, vk::MemoryPropertyFlags property_flags,
                     vk::ImageAspectFlags aspect_flags, const CommandBuffer& command_buffers)
        : m_command_buffers(command_buffers), m_extent(extent), m_format(format)
    {
        vk::Result result;
        auto& device = command_buffers.device();
        auto  vkdevice = device.handle();
        auto  dispatch = device.dispatch();

        // Create Image
        vk::ImageCreateInfo create_info;
        create_info.setExtent({ extent.width, extent.height, 1 });
        create_info.setFormat(format);
        create_info.setTiling(tiling);
        create_info.setUsage(usage_flags);
        create_info.setArrayLayers(1);
        create_info.setImageType(vk::ImageType::e2D);
        create_info.setInitialLayout(vk::ImageLayout::eUndefined);
        create_info.setSharingMode(vk::SharingMode::eExclusive);
        create_info.setMipLevels(1);
        result = vkdevice.createImage(&create_info, nullptr, &m_image, dispatch);

        auto memory_requirements = vkdevice.getImageMemoryRequirements(m_image, dispatch);
        vk::MemoryAllocateInfo alloc_info;
        alloc_info.setAllocationSize(memory_requirements.size);
        alloc_info.setMemoryTypeIndex(device.findMemoryType(memory_requirements.memoryTypeBits, property_flags));
        result = vkdevice.allocateMemory(&alloc_info, nullptr, &m_memory, dispatch);

        vkdevice.bindImageMemory(m_image, m_memory, 0, dispatch);

        // Create ImageView
        vk::ImageSubresourceRange range;
        range.setAspectMask(aspect_flags);
        range.setBaseMipLevel(0);
        range.setLevelCount(1);
        range.setBaseArrayLayer(0);
        range.setLayerCount(1);

        vk::ImageViewCreateInfo view_create_info;
        view_create_info.setImage(m_image);
        view_create_info.setViewType(vk::ImageViewType::e2D);
        view_create_info.setFormat(m_format);
        view_create_info.setSubresourceRange(range);

        result = vkdevice.createImageView(&view_create_info, nullptr, &m_view, dispatch);
    }

    void
    vkImage::transition_layout(vk::ImageLayout from, vk::ImageLayout to)
    {
        auto cmd = m_command_buffers.begin_single_time();
        {
            vk::ImageSubresourceRange subresource_range;
            subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor);
            subresource_range.setBaseMipLevel(0);
            subresource_range.setLevelCount(1);
            subresource_range.setBaseArrayLayer(0);
            subresource_range.setLayerCount(1);

            vk::ImageMemoryBarrier barrier;
            barrier.setOldLayout(from);
            barrier.setNewLayout(to);
            barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setImage(m_image);
            barrier.setSrcAccessMask({});
            barrier.setDstAccessMask({});
            barrier.setSubresourceRange(subresource_range);

            vk::PipelineStageFlags src_stage, dst_stage;

            if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eTransferDstOptimal)
            {
                barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
                src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                dst_stage = vk::PipelineStageFlagBits::eTransfer;
            }
            else if (from == vk::ImageLayout::eTransferDstOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                src_stage = vk::PipelineStageFlagBits::eTransfer;
                dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
            }
            else if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eGeneral)
            {
                src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                dst_stage = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
                barrier.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);
            }
            else
            {
                std::stringstream out;
                out << "Layout transition from "
                    << string_VkImageLayout(static_cast<VkImageLayout>(from))
                    << "to "
                    << string_VkImageLayout(static_cast<VkImageLayout>(to))
                    << " is not supported by this function.";
                throw std::invalid_argument(out.str());
            }

            cmd.pipelineBarrier(src_stage, dst_stage, {},
                                0, nullptr,
                                0, nullptr,
                                1, &barrier,
                                m_command_buffers.device().dispatch());
        }
        m_command_buffers.end_single_time(cmd);
    }

    void
    vkImage::set_layout(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                        vk::ImageSubresourceRange subresource_range, vk::PipelineStageFlags src_stage_mask,
                        vk::PipelineStageFlags dst_stage_mask)
    {
        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(old_layout);
        barrier.setNewLayout(new_layout);
        barrier.setImage(image);
        barrier.setSubresourceRange(subresource_range);

        switch (old_layout)
        {
            case vk::ImageLayout::eUndefined:
                barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                barrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
                break;
            default:
                break;
        }

        switch (new_layout)
        {
            case vk::ImageLayout::eTransferDstOptimal:
                barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                barrier.setDstAccessMask(barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                if (barrier.srcAccessMask == vk::AccessFlagBits::eNone)
                {
                    barrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
                }
                barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                break;
            default:
                break;
        }

        cmd.pipelineBarrier(src_stage_mask, dst_stage_mask,
                            {}, 0, nullptr, 0, nullptr,
                            1, &barrier);
    }

}
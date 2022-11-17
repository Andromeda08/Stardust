#pragma once

#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <stb_image.h>
#include <Vulkan/Device.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>

namespace re
{
    class vkImage
    {
    public:
        vkImage(vk::Extent2D extent,
              vk::Format format,
              vk::ImageTiling tiling,
              vk::ImageUsageFlags usage_flags,
              vk::MemoryPropertyFlags property_flags,
              const CommandBuffer& command_buffers)
        : m_command_buffers(command_buffers)
        , m_extent(extent)
        , m_format(format)
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
            range.setAspectMask(vk::ImageAspectFlagBits::eColor);
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

        void transition_layout(vk::ImageLayout from, vk::ImageLayout to)
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
                    throw std::invalid_argument("Requested layout transition not supported!");
                }

                cmd.pipelineBarrier(src_stage, dst_stage, {},
                                    0, nullptr,
                                    0, nullptr,
                                    1, &barrier,
                                    m_command_buffers.device().dispatch());
            }
            m_command_buffers.end_single_time(cmd);
        }

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
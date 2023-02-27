#include "Image.hpp"

#include <Vulkan/Utils.hpp>

namespace sdvk
{

    Image::Builder& Image::Builder::with_extent(vk::Extent2D extent)
    {
        _extent = extent;
        return *this;
    }

    Image::Builder& Image::Builder::with_format(vk::Format format)
    {
        _format = format;
        return *this;
    }

    Image::Builder& Image::Builder::with_usage_flags(vk::ImageUsageFlags usage_flags)
    {
        _usage = usage_flags;
        return *this;
    }

    Image::Builder& Image::Builder::with_aspect_flags(vk::ImageAspectFlags aspect_flags)
    {
        _aspect = aspect_flags;
        return *this;
    }

    Image::Builder& Image::Builder::with_tiling(vk::ImageTiling tiling)
    {
        _tiling = tiling;
        return *this;
    }

    Image::Builder& Image::Builder::with_memory_property_flags(vk::MemoryPropertyFlags memory_property_flags)
    {
        _memory_property_flags = memory_property_flags;
        return *this;
    }

    Image::Builder& Image::Builder::with_sample_count(vk::SampleCountFlagBits sample_count)
    {
        _sample_count = sample_count;
        return *this;
    }

    Image::Builder& Image::Builder::with_name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    std::unique_ptr<sdvk::Image> Image::Builder::create(const Context& context)
    {
        if (_extent.width == 0 || _extent.height == 0)
        {
            throw std::runtime_error("Image dimensions must be greater than 0.");
        }

        auto result = std::make_unique<sdvk::Image>(_extent, _format, _usage, _aspect, _tiling, _memory_property_flags, _sample_count, context);

        if (!_name.empty())
        {
            sdvk::util::name_vk_object(_name + " Image", (uint64_t) static_cast<VkImage>(result->image()), vk::ObjectType::eImage, context.device());
            sdvk::util::name_vk_object(_name + " ImageView", (uint64_t) static_cast<VkImageView>(result->view()), vk::ObjectType::eImageView, context.device());
        }

        return result;
    }

    Image::Image(vk::Extent2D extent,
                 vk::Format format,
                 vk::ImageUsageFlags usage,
                 vk::ImageAspectFlags aspect,
                 vk::ImageTiling tiling,
                 vk::MemoryPropertyFlags memory_property_flags,
                 vk::SampleCountFlagBits sample_count,
                 const Context& context)
    : m_extent(extent)
    , m_format(format)
    {
        vk::Result result;

        vk::ImageCreateInfo create_info;
        create_info.setExtent({ extent.width, extent.height, 1 });
        create_info.setFormat(format);
        create_info.setTiling(tiling);
        create_info.setUsage(usage);
        create_info.setArrayLayers(1);
        create_info.setImageType(vk::ImageType::e2D);
        create_info.setInitialLayout(vk::ImageLayout::eUndefined);
        create_info.setSharingMode(vk::SharingMode::eExclusive);
        create_info.setMipLevels(1);
        create_info.setSamples(sample_count);
        result = context.device().createImage(&create_info, nullptr, &m_image);

        auto memory_requirements = context.device().getImageMemoryRequirements(m_image);
        context.allocate_memory(memory_requirements, memory_property_flags, &m_memory);

        context.device().bindImageMemory(m_image, m_memory, 0);

        vk::ImageViewCreateInfo view_create_info;
        view_create_info.setImage(m_image);
        view_create_info.setFormat(format);
        view_create_info.setViewType(vk::ImageViewType::e2D);
        view_create_info.setSubresourceRange({ aspect, 0, 1, 0, 1 });
        result = context.device().createImageView(&view_create_info, nullptr, &m_view);
    }

    void Image::transition_layout(const vk::CommandBuffer& command_buffer,
                                  vk::ImageLayout old_layout,
                                  vk::ImageLayout new_layout,
                                  vk::PipelineStageFlags src_stage_mask,
                                  vk::PipelineStageFlags dst_stage_mask,
                                  vk::ImageSubresourceRange subresource_range)
    {
        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(old_layout);
        barrier.setNewLayout(new_layout);
        barrier.setImage(m_image);
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

        command_buffer.pipelineBarrier(src_stage_mask, dst_stage_mask,
                                       {}, 0, nullptr, 0, nullptr,
                                       1, &barrier);
    }
}

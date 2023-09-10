#include "Image.hpp"
#include <format>
#include <Vulkan/Utils.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula
{
    Image::Image(const sdvk::Context& context,
                 vk::Format format,
                 vk::Extent2D extent,
                 vk::SampleCountFlagBits sample_count,
                 vk::ImageUsageFlags usage_flags,
                 vk::ImageAspectFlags aspect_flags,
                 vk::ImageTiling tiling,
                 vk::MemoryPropertyFlags memory_property_flags,
                 const std::string& name)
    {
        m_properties = ImageProperties {
            .format = format,
            .extent = extent,
            .sample_count = sample_count,
            .subresource_range = { aspect_flags, 0, 1, 0, 1 }
        };

        auto device = context.device();

        {
            vk::ImageCreateInfo create_info = {};
            create_info.setFormat(format);
            create_info.setExtent({ extent.width, extent.height, 1 });
            create_info.setSamples(sample_count);
            create_info.setUsage(usage_flags);
            create_info.setTiling(tiling);
            create_info.setArrayLayers(1);
            create_info.setMipLevels(1);
            create_info.setImageType(vk::ImageType::e2D);
            create_info.setSharingMode(vk::SharingMode::eExclusive);
            create_info.setInitialLayout(vk::ImageLayout::eUndefined);

            auto result = device.createImage(&create_info, nullptr, &m_image);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to create Image!");
            }
        }

        auto memory_requirements = context.device().getImageMemoryRequirements(m_image);
        context.allocate_memory(memory_requirements, memory_property_flags, &m_device_memory);
        device.bindImageMemory(m_image, m_device_memory, 0);

        {
            vk::ImageViewCreateInfo create_info;
            create_info.setImage(m_image);
            create_info.setFormat(format);
            create_info.setViewType(vk::ImageViewType::e2D);
            create_info.setSubresourceRange(m_properties.subresource_range);

            auto result = device.createImageView(&create_info, nullptr, &m_image_view);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to create ImageView!");
            }
        }

        if (context.is_debug())
        {
            std::string image_name = "Unknown: Image";
            std::string image_view_name = "Unknown: ImageView";

            if (!name.empty())
            {
//                image_name = std::format("{}: Image", name);
//                image_view_name = std::format("{}: ImageView", name);
            }

            sdvk::util::name_vk_object(image_name,
                                       (uint64_t) static_cast<VkImage>(m_image),
                                       vk::ObjectType::eImage,
                                       context.device());

            sdvk::util::name_vk_object(image_view_name,
                                       (uint64_t) static_cast<VkImageView>(m_image_view),
                                       vk::ObjectType::eImageView,
                                       context.device());
        }
    }
}
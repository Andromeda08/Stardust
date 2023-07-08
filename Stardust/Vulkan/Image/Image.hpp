#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>

namespace sdvk
{
    class Image
    {
    public:
        struct Builder
        {
            Builder() = default;

            Builder& with_extent(vk::Extent2D extent);
            Builder& with_format(vk::Format format);
            Builder& with_usage_flags(vk::ImageUsageFlags usage_flags);
            Builder& with_aspect_flags(vk::ImageAspectFlags aspect_flags);
            Builder& with_tiling(vk::ImageTiling tiling);
            Builder& with_memory_property_flags(vk::MemoryPropertyFlags memory_property_flags);
            Builder& with_sample_count(vk::SampleCountFlagBits sample_count);
            Builder& with_name(std::string const& name);

            [[nodiscard]] std::unique_ptr<sdvk::Image> create(Context const& context);

        private:
            vk::Extent2D            _extent { 0, 0 };
            vk::Format              _format { vk::Format::eR32G32B32A32Sfloat };
            vk::ImageUsageFlags     _usage  { vk::ImageUsageFlagBits::eColorAttachment };
            vk::ImageAspectFlags    _aspect { vk::ImageAspectFlagBits::eColor };
            vk::ImageTiling         _tiling { vk::ImageTiling::eOptimal };
            vk::MemoryPropertyFlags _memory_property_flags {};
            vk::SampleCountFlagBits _sample_count { vk::SampleCountFlagBits::e1 };
            std::string _name;
        };

        Image(vk::Extent2D extent,
              vk::Format format,
              vk::ImageUsageFlags usage,
              vk::ImageAspectFlags aspect,
              vk::ImageTiling tiling,
              vk::MemoryPropertyFlags memory_property_flags,
              vk::SampleCountFlagBits sample_count,
              Context const& context);

        void transition_layout(vk::CommandBuffer const& command_buffer,
                               vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout,
                               vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eAllCommands,
                               vk::ImageSubresourceRange subresource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        const vk::Image& image() const { return m_image; }

        const vk::ImageView& view() const { return m_view; }

        const vk::Extent2D& extent() const { return m_extent; }

        const vk::Format& format() const { return m_format; }

        vk::ImageSubresourceRange image_subresource_range() const { return { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }; }

    private:
        vk::Image m_image;
        vk::ImageView m_view;
        vk::DeviceMemory m_memory;

        vk::Format m_format;
        vk::Extent2D m_extent;
    };
}

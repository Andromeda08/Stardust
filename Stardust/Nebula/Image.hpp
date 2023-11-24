#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula
{
    class Image
    {
    public:
        struct ImageProperties
        {
            vk::Format                 format { vk::Format::eR32G32B32Sfloat };
            vk::Extent2D               extent { 1920, 1080 };
            vk::SampleCountFlagBits    sample_count { vk::SampleCountFlagBits::e1 };
            vk::ImageSubresourceRange  subresource_range { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            vk::ImageSubresourceLayers subresource_layers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
        };

        struct ImageState
        {
            vk::AccessFlags2 access_flags { vk::AccessFlagBits2::eNone };
            vk::ImageLayout  layout { vk::ImageLayout::eUndefined };
        };

        Image(const sdvk::Context& context,
              vk::Format format,
              vk::Extent2D extent,
              vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1,
              vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
              vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlags memory_property_flags = vk::MemoryPropertyFlagBits::eDeviceLocal,
              const std::string& name = "");

        const vk::Image& image() const { return m_image; }

        const vk::ImageView& image_view() const { return m_image_view; }

        const ImageProperties& properties() const { return m_properties; }

        const ImageState& state() { return m_state; }

        void update_state(ImageState state) { m_state = state; }

        static std::shared_ptr<Image> make_depth_image(vk::Extent2D extent,
                                                       const sdvk::Context& context,
                                                       const std::string& name);

    private:
        static vk::Format find_depth_format(vk::PhysicalDevice const& physical_device);

    private:
        vk::Image        m_image;
        vk::ImageView    m_image_view;
        vk::DeviceMemory m_device_memory;
        ImageProperties  m_properties {};
        ImageState       m_state {};

        const sdvk::Context& m_context;
    };
}
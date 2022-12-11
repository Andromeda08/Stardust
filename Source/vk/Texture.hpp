#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Vulkan/Device.hpp>
#include <vk/Image.hpp>
#include <vk/Sampler.hpp>

namespace re
{
    class Texture
    {
    public:
        Texture(const std::string& image, const CommandBuffer& command_buffer);

        const vk::Image& image() const { return m_image->image(); }

        const vk::ImageView& view() const { return m_image->view(); }

    private:
        vk::Extent2D m_extent;

        std::unique_ptr<vkImage> m_image;
        std::unique_ptr<Sampler> m_sampler;

        const CommandBuffer& m_command_buffers;
        const Device& m_device;
    };
}
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk/Image.hpp>
#include <vk/Sampler.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Device/Device.hpp>

namespace re
{
    class Texture
    {
    public:
        Texture(const std::string& image, const CommandBuffers& command_buffer);

        const vk::Image& image() const { return m_image->image(); }

        const vk::ImageView& view() const { return m_image->view(); }

    private:
        vk::Extent2D m_extent;

        std::unique_ptr<Image> m_image;
        std::unique_ptr<Sampler> m_sampler;

        const CommandBuffers& m_command_buffers;
        const Device& m_device;
    };
}
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Vulkan/Device.hpp>
#include <vk/Buffer.hpp>
#include <vk/Image.hpp>
#include <vk/Sampler.hpp>

namespace re
{
    class Texture
    {
    public:
        Texture(const std::string& image, const CommandBuffer& command_buffer)
        : m_command_buffers(command_buffer)
        , m_device(command_buffer.device())
        {
            int img_w, img_h, img_ch;
            stbi_uc* pixels = stbi_load(image.c_str(), &img_w, &img_h, &img_ch, STBI_rgb_alpha);
            vk::DeviceSize image_size = img_w * img_h * 4;

            if (!pixels)
            {
                throw std::runtime_error("Failed to load texture image \"" + image + "\"!");
            }

            m_extent = vk::Extent2D{ static_cast<uint32_t>(img_w), static_cast<uint32_t>(img_h) };

            // Create Staging buffer and upload image data to it.
            auto staging = Buffer::make_staging_buffer(image_size, m_command_buffers);
            Buffer::set_data(pixels, *staging, m_command_buffers);

            // Now we can free the image CPU side.
            stbi_image_free(pixels);

            // Create Image & View intended to be used for sampling
            m_image = std::make_unique<vkImage>(m_extent,
                                                vk::Format::eR8G8B8A8Srgb,
                                                vk::ImageTiling::eOptimal,
                                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                                                vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                m_command_buffers);

            // Upload image to buffer, then prepare for shader usage
            m_image->transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

            Buffer::copy_to_image(*staging, *m_image, m_command_buffers);

            m_image->transition_layout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

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
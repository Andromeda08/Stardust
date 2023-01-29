#include "Texture.hpp"

#include <stb_image.h>
#include <Vulkan/Buffer.hpp>

namespace sdvk
{
    Texture2D::Texture2D(const std::string& path, const CommandBuffers& command_buffers, const Context& context)
    {
        int img_w, img_h, img_ch;
        stbi_uc* pixels = stbi_load(path.c_str(), &img_w, &img_h, &img_ch, STBI_rgb_alpha);

        if (pixels == nullptr)
        {
            throw std::runtime_error("Failed to load texture image \"" + path + "\"!");
        }

        vk::DeviceSize image_size = img_w * img_h * 4;
        auto extent = vk::Extent2D{ static_cast<uint32_t>(img_w), static_cast<uint32_t>(img_h) };
        auto staging = Buffer::Builder().with_size(image_size).create_staging(context);
        staging->set_data(pixels, context.device());

        stbi_image_free(pixels);

        m_image = Image::Builder()
            .with_extent(extent)
            .with_format(vk::Format::eR8G8B8A8Srgb)
            .with_tiling(vk::ImageTiling::eOptimal)
            .with_usage_flags(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_aspect_flags(vk::ImageAspectFlagBits::eColor)
            .create(context);

        command_buffers.execute_single_time([&](const auto& cmd){
            m_image->transition_layout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            staging->copy_to_image(*m_image, cmd);
            m_image->transition_layout(cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        });
    }
}
#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Image.hpp>

namespace sdvk
{
    class Texture2D
    {
    public:
        Texture2D(std::string const& path, CommandBuffers const& command_buffers, Context const& context);

        const vk::Extent2D& extent() const { return m_image->extent(); }

        const vk::Image& image() const { return m_image->image(); }

        const vk::ImageView& view() const { return m_image->view(); }

    private:
        std::unique_ptr<Image> m_image;
    };
}

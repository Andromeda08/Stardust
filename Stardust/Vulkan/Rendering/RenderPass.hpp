#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>

namespace sdvk
{
    class RenderPass
    {
    public:
        RenderPass(RenderPass const& render_pass) = delete;
        RenderPass& operator=(RenderPass const&) = delete;

        RenderPass(vk::Format swapchain_img_format, vk::Format depth_image_format, Context const& context);

        vk::RenderPass handle() const { return m_render_pass; }

    private:
        inline static vk::AttachmentDescription make_color_attachment(vk::Format swap_chain_img_format);

        inline static vk::AttachmentDescription make_depth_attachment(vk::Format depth_img_format);

    private:
        vk::RenderPass m_render_pass;
    };

}
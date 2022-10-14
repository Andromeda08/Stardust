#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Macro.hpp"

class RenderPass
{
public:
    NON_COPIABLE(RenderPass)

    RenderPass(const Device& device, vk::Format swap_chain_img_format);

    vk::RenderPass handle() const { return m_render_pass; }

    const Device& device() const { return m_device; }

private:
    inline static vk::AttachmentDescription make_color_attachment(vk::Format swap_chain_img_format);

private:
    vk::RenderPass m_render_pass;

    const Device& m_device;
};

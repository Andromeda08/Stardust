#pragma once

#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "Command/CommandBuffers.hpp"

class DepthBuffer
{
public:
    DepthBuffer(vk::Extent2D extent,
                const Device& device,
                const CommandBuffers& cmd_buffers);

    vk::Format format() const { return m_format; }

    const vk::ImageView& view() const { return m_depth_view; }

private:
    vk::Format       m_format;
    vk::Image        m_depth_image;
    vk::ImageView    m_depth_view;
    vk::DeviceMemory m_depth_memory;
};

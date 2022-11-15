#pragma once

#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "Command/CommandBuffer.hpp"
#include "Image/Image.hpp"
#include "Image/ImageView.hpp"
#include "../Utility/Macro.hpp"

class DepthBuffer
{
public:
    NON_COPIABLE(DepthBuffer)

    DepthBuffer(vk::Extent2D          extent,
                const Device&         device,
                const CommandBuffer& cmd_buffers);

    vk::Format format() const { return m_format; }

    const vk::ImageView& view() const { return m_depth_view->handle(); }

private:
    vk::Format find_depth_format();

private:
    std::unique_ptr<Image>     m_depth_image;

    std::unique_ptr<ImageView> m_depth_view;

    vk::Format m_format;

    const Device& m_device;
};

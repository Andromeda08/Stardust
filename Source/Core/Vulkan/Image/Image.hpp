#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Macro.hpp"

class Image
{
public:
    NON_COPIABLE(Image)

    /**
     * @brief Creates a vk::Image with the specified attributes
     * and the memory associated with the image.
     */
    Image(vk::Extent2D          extent,
          vk::Format            format,
          vk::ImageUsageFlags   usage,
          const Device&         device,
          const CommandBuffers& cmd_buffers);

    /**
     * @brief Explicitly transitions an image layout.
     */
    void transition_image_layout(vk::ImageLayout from, vk::ImageLayout to);

    /**
     * @brief Creates a vk::Image to be used by a Depth Buffer
     */
    static std::unique_ptr<Image> make_depth_buffer_image(vk::Extent2D          extent,
                                                          vk::Format            format,
                                                          const Device&         device,
                                                          const CommandBuffers& cmd_buffers);

    const vk::Image& image() const { return m_image; }

private:
    vk::Image        m_image;

    vk::DeviceMemory m_memory;

    vk::Format       m_format;

    const Device&    m_device;

    const CommandBuffers& m_cmd_buffers;
};

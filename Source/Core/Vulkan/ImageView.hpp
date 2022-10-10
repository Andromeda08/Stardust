#pragma once

#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "../Macro.hpp"

class ImageView {
public:
    NON_COPIABLE(ImageView)

    ImageView(const Device& device, vk::Image image, vk::Format format, vk::ImageAspectFlagBits imageAspectFlags);

    const Device& device() const { return mDevice; }

private:
    vk::ImageView mImageView;

    const vk::Image mImage;

    const vk::Format mFormat;

    const Device& mDevice;
};

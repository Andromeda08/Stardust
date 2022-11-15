#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Utility/Macro.hpp"

class ImageView {
public:
    NON_COPIABLE(ImageView)

    ImageView(const Device& device, vk::Image image, vk::Format format, vk::ImageAspectFlagBits imageAspectFlags);

    const vk::ImageView& handle() const { return mImageView; }

private:
    vk::ImageView mImageView;

    const vk::Format mFormat;
};

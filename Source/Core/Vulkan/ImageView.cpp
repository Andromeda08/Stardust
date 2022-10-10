#include "ImageView.hpp"

ImageView::ImageView(const Device& device, vk::Image image, vk::Format format, vk::ImageAspectFlagBits imageAspectFlags)
: mDevice(device)
, mImage(image)
, mFormat(format)
{
    vk::ComponentMapping componentMapping = {};
    componentMapping.setR(vk::ComponentSwizzle::eIdentity);
    componentMapping.setG(vk::ComponentSwizzle::eIdentity);
    componentMapping.setB(vk::ComponentSwizzle::eIdentity);
    componentMapping.setA(vk::ComponentSwizzle::eIdentity);

    vk::ImageSubresourceRange imageSubresourceRange = {};
    imageSubresourceRange.setAspectMask(imageAspectFlags);
    imageSubresourceRange.setBaseMipLevel(0);
    imageSubresourceRange.setLevelCount(1);
    imageSubresourceRange.setBaseArrayLayer(0);
    imageSubresourceRange.setLayerCount(1);

    vk::ImageViewCreateInfo createInfo = {};
    createInfo.setImage(image);
    createInfo.setFormat(format);
    createInfo.setViewType(vk::ImageViewType::e2D);
    createInfo.setComponents(componentMapping);
    createInfo.setSubresourceRange(imageSubresourceRange);

    auto result = mDevice.handle().createImageView(&createInfo, nullptr, &mImageView);
}


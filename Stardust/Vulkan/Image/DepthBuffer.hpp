#pragma once

#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Image.hpp>

namespace sdvk
{
    class DepthBuffer : public Image
    {
    public:
        DepthBuffer(DepthBuffer const&) = delete;
        DepthBuffer& operator=(DepthBuffer const&) = delete;

        DepthBuffer(vk::Extent2D extent, vk::SampleCountFlagBits sample_count, Context const& context);

    private:
        static vk::Format find_depth_format(vk::PhysicalDevice const& physical_device);
    };
}
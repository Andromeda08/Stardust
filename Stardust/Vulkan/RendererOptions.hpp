#pragma once

#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct RendererOptions
    {
        struct MSAA
        {
            bool enabled {false};
            vk::SampleCountFlagBits sample_count {vk::SampleCountFlagBits::e1};
        } msaa;

        // Ambient Occlusion
        // Shadows
    };
}
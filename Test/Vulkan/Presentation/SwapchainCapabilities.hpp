#pragma once

#include <limits>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct SwapchainCapabilities
    {
        vk::SurfaceCapabilitiesKHR        surface_capabilities;
        std::vector<vk::SurfaceFormatKHR> surface_formats;
        std::vector<vk::PresentModeKHR>   present_modes;

        vk::Extent2D         selected_extent;
        vk::SurfaceFormatKHR selected_format;
        vk::PresentModeKHR   selected_present_mode;
        uint32_t             image_count;
    };
}
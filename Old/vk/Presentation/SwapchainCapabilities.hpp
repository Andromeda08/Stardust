#pragma once

#include <limits>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR        _capabilities {};
    std::vector<vk::SurfaceFormatKHR> _formats;
    std::vector<vk::PresentModeKHR>   _present_modes;

    static SwapchainSupportDetails query_swapchain_support(const Device& device)
    {
        SwapchainSupportDetails result;

        result._capabilities = device.physicalDevice().getSurfaceCapabilitiesKHR(device.surface().handle());
        result._formats = device.physicalDevice().getSurfaceFormatsKHR(device.surface().handle());
        result._present_modes = device.physicalDevice().getSurfacePresentModesKHR(device.surface().handle());

        return result;
    }

    vk::SurfaceFormatKHR select_format(vk::Format preferred_format = vk::Format::eB8G8R8A8Unorm,
                                       vk::ColorSpaceKHR preferred_color_space = vk::ColorSpaceKHR::eSrgbNonlinear) const
    {
        vk::Format result = vk::Format::eUndefined;

        for (const auto& format : _formats)
        {
            if (format.format == preferred_format && format.colorSpace == preferred_color_space)
            {
                result = format.format;
            }
        }

        if ((_formats.size() == 1 && _formats[0].format == vk::Format::eUndefined) ||
            (result == vk::Format::eUndefined))
        {
            return vk::Format::eB8G8R8A8Unorm;
        }

        return result;
    }

    vk::PresentModeKHR select_present_mode(vk::PresentModeKHR preferred) const
    {
        auto find = std::find(std::begin(_present_modes), std::end(_present_modes), preferred);
        if (find != std::end(_present_modes))
        {
            return preferred;
        }

        // Fallback to fifo
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D select_extent(const Device& device) const
    {
        if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return _capabilities.currentExtent;
        }

        auto min = _capabilities.minImageExtent;
        auto max = _capabilities.maxImageExtent;

        auto extent = device.surface().instance().window().get_framebuffer_extent();
        extent.width = std::clamp(extent.width, min.width, max.width);
        extent.height = std::clamp(extent.height, min.height, max.height);
        return extent;
    }
};
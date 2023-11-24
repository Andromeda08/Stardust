#include "SwapchainBuilder.hpp"

#include <limits>
#include <vector>

namespace sdvk
{
    SwapchainBuilder::SwapchainBuilder(const sd::Window &w, const Context &context)
    : ctx(context), window(w) {}

    SwapchainBuilder& SwapchainBuilder::set_preferred_format(vk::Format format)
    {
        preferred_format = format;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::set_preferred_color_space(vk::ColorSpaceKHR color_space)
    {
        preferred_color_space = color_space;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::set_present_mode(vk::PresentModeKHR present_mode)
    {
        preferred_present_mode = present_mode;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::set_image_count(uint32_t images)
    {
        preferred_image_count = 2;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::with_defaults()
    {
        query_swapchain_support();
        set_preferred_format();
        set_preferred_color_space();
        set_present_mode();
        set_image_count();
        return *this;
    }

    std::unique_ptr<Swapchain> SwapchainBuilder::create()
    {
        query_swapchain_support();
        if (result.surface_formats.empty() || result.present_modes.empty())
        {
            throw std::runtime_error("Inadequate swapchain support.");
        }

        if (result.surface_capabilities.minImageCount > preferred_image_count ||
            result.surface_capabilities.maxImageCount < preferred_image_count)
        {
            throw std::runtime_error("Image count \"" + std::to_string(preferred_image_count) + "\" out of supported range.");
        }

        result.image_count = preferred_image_count;

        select_format();
        select_present_mode();
        select_extent();

        return std::make_unique<Swapchain>(result, ctx);
    }

    void SwapchainBuilder::query_swapchain_support()
    {
        result.surface_capabilities = ctx.physical_device().getSurfaceCapabilitiesKHR(ctx.surface());
        result.surface_formats = ctx.physical_device().getSurfaceFormatsKHR(ctx.surface());
        result.present_modes = ctx.physical_device().getSurfacePresentModesKHR(ctx.surface());
    }

    void SwapchainBuilder::select_format()
    {
        vk::SurfaceFormatKHR format = result.surface_formats[0];

        for (const auto& f : result.surface_formats)
        {
            if (f.format == preferred_format) // vk::Format::eB8G8R8A8Srgb /*&& f.colorSpace == preferred_color_space*/)
            {
                result.selected_format = f;
                return;
            }
        }

        result.selected_format = format;
    }

    void SwapchainBuilder::select_present_mode()
    {
        auto find = std::find(std::begin(result.present_modes), std::end(result.present_modes), preferred_present_mode);
        if (find != std::end(result.present_modes))
        {
            result.selected_present_mode = preferred_present_mode;
        }

        result.selected_present_mode = vk::PresentModeKHR::eFifo;
    }

    void SwapchainBuilder::select_extent()
    {
        if (result.surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            result.selected_extent = result.surface_capabilities.currentExtent;
        }

        auto min = result.surface_capabilities.minImageExtent;
        auto max = result.surface_capabilities.maxImageExtent;

        auto extent = window.get_framebuffer_extent();
        extent.width = std::clamp(extent.width, min.width, max.width);
        extent.height = std::clamp(extent.height, min.height, max.height);
        result.selected_extent = vk::Extent2D { extent.width, extent.height };
    }

};
#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>
#include <Vulkan/Presentation/SwapchainCapabilities.hpp>
#include <Window/Window.hpp>

namespace sdvk
{
    struct SwapchainBuilder
    {
        SwapchainBuilder(sd::Window const& w, sdvk::Context const& context);

        SwapchainBuilder& set_preferred_format(vk::Format format = vk::Format::eB8G8R8A8Unorm);

        SwapchainBuilder& set_preferred_color_space(vk::ColorSpaceKHR color_space = vk::ColorSpaceKHR::eSrgbNonlinear);

        SwapchainBuilder& set_present_mode(vk::PresentModeKHR present_mode = vk::PresentModeKHR::eMailbox);

        SwapchainBuilder& set_image_count(uint32_t images = 2);

        SwapchainBuilder& with_defaults();

        std::unique_ptr<Swapchain> create();

    private:
        void query_swapchain_support();

        void select_format();

        void select_present_mode();

        void select_extent();

    private:
        vk::Format         preferred_format {};
        vk::ColorSpaceKHR  preferred_color_space {};
        vk::PresentModeKHR preferred_present_mode {};
        uint32_t           preferred_image_count { 0 };

        SwapchainCapabilities result;

        const sd::Window&    window;
        const sdvk::Context& ctx;
    };
}
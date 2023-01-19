#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include "SwapchainCapabilities.hpp"

namespace sdvk
{
    class Swapchain
    {
    public:
        Swapchain(Swapchain const&) = delete;
        Swapchain& operator=(Swapchain const&) = delete;

        Swapchain(SwapchainCapabilities const& capabilities, Context const& context);

        /**
         * @brief Create a scissor based on the current extent.
         */
        vk::Rect2D make_scissor() const;

        /**
         * @brief Create a viewport object based on the current state of the Swapchain.
         * The viewport is flipped along the Y axis for GLM compatibility.
         * This requires Maintenance1, which is included by default since API version 1.1.
         * https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
         */
        vk::Viewport make_viewport() const;

        const vk::Image& image(uint32_t id) const;
        const vk::Image& operator[](uint32_t id) const;

        const vk::ImageView& view(uint32_t id) const;

        float aspect_ratio() const { return (float) m_extent.width / (float) m_extent.height; }

        uint32_t image_count() const { return m_images.size(); }

        vk::Extent2D extent() const { return m_extent; }
        vk::Format format() const { return m_format.format; }
        [[maybe_unused]] vk::ColorSpaceKHR color_space() const { return m_format.colorSpace; }

    private:
        void create_swapchain();

        void create_images();

        void create_sync_objects();

        void cleanup();

    private:
        uint32_t m_image_count { 0 };
        vk::SurfaceFormatKHR m_format;
        vk::PresentModeKHR m_present_mode;
        vk::Extent2D m_extent;

        vk::SwapchainKHR m_swapchain;

        std::vector<vk::Image>     m_images;
        std::vector<vk::ImageView> m_views;

        struct Sync
        {
            std::array<vk::Semaphore, 2> s_image_available, s_render_finished;
            std::array<vk::Fence,     2> f_in_flight;
        } m_sync;

        Context const& m_ctx;
        const SwapchainCapabilities& m_capabilities;
    };
}

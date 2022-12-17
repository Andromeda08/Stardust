#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <vk/Image.hpp>
#include <vk/Device/Device.hpp>
#include <vk/Pipelines/RenderPass.hpp>
#include <vk/Presentation/SwapchainCapabilities.hpp>

class Swapchain
{
public:
    /**
     * @brief Create a double buffered Swapchain with it present mode set as Mailbox.
     */
    explicit Swapchain(const Device& device);

    /**
     * @brief Create a Swapchain with the specified present mode and image count
     */
    Swapchain(const Device& device, vk::PresentModeKHR preferred_present_mode, uint32_t images);

    // TODO: Decouple framebuffers from swapchain
    void create_frame_buffers(RenderPass const& render_pass, re::DepthBuffer const& depth_buffer);

    /**
     * @brief Rebuild the swapchain according to Surface parameters.
     */
    void rebuild();

    /**
     * @brief Free resources allocated for the Swapchain
     */
    void cleanup();

    const vk::Image& image(uint32_t index) const;

    const vk::ImageView& view(uint32_t index) const;

    const vk::Framebuffer& framebuffer(uint32_t index) const;

    /**
     * @brief Swapchain image access through indexing operator.
     */
    const vk::Image& operator[](uint32_t index) const;

    /**
     * @brief Create a scissor based on the current extent.
     */
    vk::Rect2D make_scissor() const;

    /**
     * @brief Create a viewport object based on the current state of the Swapchain.
     * The viewport is flipped along the Y axis for GLM compatibility.
     * https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
     */
    vk::Viewport make_viewport() const;

    float aspect_ratio() const { return (float) m_extent.width / (float) m_extent.height; }

    vk::Extent2D extent() const { return m_extent; }

    vk::Format format() const { return m_format.format; }

    [[maybe_unused]] vk::ColorSpaceKHR color_space() const { return m_format.colorSpace; }

    uint32_t image_count() const { return m_images.size(); }

    const vk::SwapchainKHR& handle() const { return m_swapchain; }

    Swapchain(Swapchain const&) = delete;
    Swapchain& operator=(Swapchain const&) = delete;

private:
    void create_swapchain(vk::PresentModeKHR preferred_present_mode, uint32_t images);

private:
    vk::SwapchainKHR             m_swapchain;
    SwapchainSupportDetails      m_support_details;

    vk::PresentModeKHR           m_present_mode {};
    vk::SurfaceFormatKHR         m_format {};
    vk::Extent2D                 m_extent {};

    std::vector<vk::Image>       m_images;
    std::vector<vk::ImageView>   m_views;
    std::vector<vk::Framebuffer> m_framebuffers;

    const Device&                m_device;
};
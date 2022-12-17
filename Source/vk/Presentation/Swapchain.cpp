#include "Swapchain.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

Swapchain::Swapchain(const Device& device)
: m_device(device)
{
    create_swapchain(vk::PresentModeKHR::eMailbox, 2);
}

Swapchain::Swapchain(const Device& device, vk::PresentModeKHR preferred_present_mode, uint32_t images)
: m_device(device)
{
    create_swapchain(preferred_present_mode, images);
}

void Swapchain::create_swapchain(vk::PresentModeKHR preferred_present_mode, uint32_t images)
{
    m_support_details = SwapchainSupportDetails::query_swapchain_support(m_device);
    if (m_support_details._formats.empty() || m_support_details._present_modes.empty())
    {
        throw std::runtime_error("Inadequate swapchain support.");
    }

    m_present_mode = m_support_details.select_present_mode(preferred_present_mode);
    m_format       = m_support_details.select_format();
    m_extent       = m_support_details.select_extent(m_device);

    if (m_support_details._capabilities.minImageCount > images ||
        m_support_details._capabilities.maxImageCount < images)
    {
        throw std::runtime_error("Image count \"" + std::to_string(images) + "\" out of supported range.");
    }

    vk::SwapchainCreateInfoKHR create_info;
    {
        create_info.setSurface(m_device.surface().handle());
        create_info.setMinImageCount(images);
        create_info.setImageFormat(m_format.format);
        create_info.setImageColorSpace(m_format.colorSpace);
        create_info.setImageExtent(m_extent);
        create_info.setImageArrayLayers(1);
        create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        create_info.setPreTransform(m_support_details._capabilities.currentTransform);
        create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        create_info.setPresentMode(m_present_mode);
        create_info.setClipped(true);
        create_info.setOldSwapchain(nullptr);

        // Assuming that the graphics and present queue indices are equal
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);
    }

    if (m_device.graphics_index() != m_device.present_index())
    {
        std::array<uint32_t, 2> indices = { m_device.graphics_index(), m_device.present_index() };
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndexCount(indices.size());
        create_info.setPQueueFamilyIndices(indices.data());
    }

    vk::Result result;
    result = m_device.handle().createSwapchainKHR(&create_info, nullptr, &m_swapchain);

    m_images.resize(images);
    m_images = m_device.handle().getSwapchainImagesKHR(m_swapchain);

    // Create image views
    vk::ComponentMapping component_mapping = {
        vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
    };

    vk::ImageViewCreateInfo view_create_info;
    view_create_info.setFormat(m_format.format);
    view_create_info.setViewType(vk::ImageViewType::e2D);
    view_create_info.setComponents(component_mapping);
    view_create_info.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    m_views.resize(images);
    for (size_t i = 0; i < images; i++)
    {
        view_create_info.setImage(m_images[i]);
        result = m_device.handle().createImageView(&view_create_info, nullptr, &m_views[i]);
    }

    std::cout
        << "Swapchain format: " << string_VkFormat(static_cast<VkFormat>(m_format.format))
        << std::endl
        << "Swapchain color space: " << string_VkColorSpaceKHR(static_cast<VkColorSpaceKHR>(m_format.colorSpace))
        << std::endl;
}

void Swapchain::create_frame_buffers(RenderPass const& render_pass, re::DepthBuffer const& depth_buffer)
{
    std::array<vk::ImageView, 2> attachments = { m_views[0], depth_buffer.view() };

    vk::FramebufferCreateInfo create_info;
    create_info.setRenderPass(render_pass.handle());
    create_info.setAttachmentCount(attachments.size());
    create_info.setPAttachments(attachments.data());
    create_info.setWidth(m_extent.width);
    create_info.setHeight(m_extent.height);
    create_info.setLayers(1);

    vk::Result result;

    m_framebuffers.resize(image_count());
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        attachments[0] = m_views[i];
        result = m_device.handle().createFramebuffer(&create_info, nullptr, &m_framebuffers[i]);
    }
}

void Swapchain::rebuild()
{
    auto window = m_device.surface().instance().window().handle();

    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    m_device.waitIdle();

    cleanup();
    create_swapchain(m_present_mode, m_images.size());
    // TODO: Framebuffer recreation
}

void Swapchain::cleanup()
{
    for (auto framebuffer : m_framebuffers)
    {
        m_device.handle().destroy(framebuffer);
    }

    for (auto imageview : m_views)
    {
        m_device.handle().destroy(imageview);
    }

    m_device.handle().destroy(m_swapchain);
}

const vk::Image& Swapchain::operator[](uint32_t index) const
{
    return this->image(index);
}

const vk::Image& Swapchain::image(uint32_t index) const
{
    if (index > m_images.size())
    {
        throw std::out_of_range("Swapchain image index \"" + std::to_string(index) + "\" out of bounds.");
    }
    return m_images[index];
}

const vk::ImageView& Swapchain::view(uint32_t index) const
{
    if (index > m_views.size())
    {
        throw std::out_of_range("Swapchain image view index \"" + std::to_string(index) + "\" out of bounds.");
    }
    return m_views[index];
}

const vk::Framebuffer& Swapchain::framebuffer(uint32_t index) const
{
    if (index > m_framebuffers.size())
    {
        throw std::out_of_range("Framebuffer index \"" + std::to_string(index) + "\" out of bounds.");
    }
    return m_framebuffers[index];
}

vk::Rect2D Swapchain::make_scissor() const
{
    return { { 0, 0 }, m_extent};
}

vk::Viewport Swapchain::make_viewport() const
{
    vk::Viewport vp;

    vp.setX(0.0f);
    vp.setY(static_cast<float>(m_extent.height));
    vp.setWidth(static_cast<float>(m_extent.width));
    vp.setHeight(-1.0f * static_cast<float>(m_extent.height));
    vp.setMaxDepth(1.0f);
    vp.setMinDepth(0.0f);

    return vp;
}
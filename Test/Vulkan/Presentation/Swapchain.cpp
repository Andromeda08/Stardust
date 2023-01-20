#include "Swapchain.hpp"

#include <iostream>
#include <vulkan/vk_enum_string_helper.h>

namespace sdvk
{
    Swapchain::Swapchain(const SwapchainCapabilities& capabilities, const Context& context)
    : m_ctx(context), m_capabilities(capabilities)
    , m_image_count(capabilities.image_count), m_format(capabilities.selected_format)
    , m_present_mode(capabilities.selected_present_mode), m_extent(capabilities.selected_extent)
    {
        create_swapchain();
        create_images();
        create_sync_objects();

        if (m_ctx.is_debug())
        {
            std::cout
                << "Swapchain format: " << string_VkFormat(static_cast<VkFormat>(m_format.format))
                << std::endl
                << "Swapchain color space: " << string_VkColorSpaceKHR(static_cast<VkColorSpaceKHR>(m_format.colorSpace))
                << std::endl;

            for (auto i = 0; i < m_image_count; i++)
            {
                vk::Result result;
                std::string name = "[SC] Image " + std::to_string(i);

                vk::DebugUtilsObjectNameInfoEXT name_info;
                name_info.setObjectHandle((uint64_t) static_cast<VkImage>(m_images[i]));
                name_info.setObjectType(vk::ObjectType::eImage);
                name_info.setPObjectName(name.c_str());
                result = m_ctx.device().setDebugUtilsObjectNameEXT(&name_info);

                name = "[SC] ImageView " + std::to_string(i);
                name_info.setObjectHandle((uint64_t) static_cast<VkImageView>(m_views[i]));
                name_info.setObjectType(vk::ObjectType::eImageView);
                name_info.setPObjectName(name.c_str());
                result = m_ctx.device().setDebugUtilsObjectNameEXT(&name_info);
            }
        }
    }

    void Swapchain::create_swapchain()
    {
        vk::SwapchainCreateInfoKHR create_info;
        create_info.setSurface(m_ctx.surface());
        create_info.setMinImageCount(m_capabilities.image_count);
        create_info.setImageFormat(m_capabilities.selected_format.format);
        create_info.setImageColorSpace(m_capabilities.selected_format.colorSpace);
        create_info.setImageExtent(m_capabilities.selected_extent);
        create_info.setImageArrayLayers(1);
        create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        create_info.setPreTransform(m_capabilities.surface_capabilities.currentTransform);
        create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        create_info.setPresentMode(m_capabilities.selected_present_mode);
        create_info.setClipped(true);
        create_info.setOldSwapchain(nullptr);

        // Assuming that the graphics and present queue indices are equal
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);

        if (m_ctx.q_graphics().index != m_ctx.q_present().index)
        {
            std::array<uint32_t, 2> indices = { m_ctx.q_graphics().index, m_ctx.q_present().index };
            create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
            create_info.setQueueFamilyIndexCount(indices.size());
            create_info.setPQueueFamilyIndices(indices.data());
        }

        vk::Result result = m_ctx.device().createSwapchainKHR(&create_info, nullptr, &m_swapchain);
    }

    void Swapchain::create_images()
    {
        m_images.resize(m_image_count);
        m_images = m_ctx.device().getSwapchainImagesKHR(m_swapchain);

        vk::ComponentMapping component_mapping = {
                vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
        };

        vk::ImageViewCreateInfo create_info;
        create_info.setFormat(m_format.format);
        create_info.setViewType(vk::ImageViewType::e2D);
        create_info.setComponents(component_mapping);
        create_info.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        m_views.resize(m_image_count);
        for (auto i = 0; i < m_image_count; i++)
        {
            create_info.setImage(m_images[i]);
            vk::Result result = m_ctx.device().createImageView(&create_info, nullptr, &m_views[i]);
        }
    }

    void Swapchain::create_sync_objects()
    {
        for (uint32_t i = 0; i < m_image_count; i++)
        {
            vk::Result result;
            vk::SemaphoreCreateInfo sci;
            vk::FenceCreateInfo fci;
            fci.setFlags(vk::FenceCreateFlagBits::eSignaled);

            result = m_ctx.device().createSemaphore(&sci, nullptr, &m_sync.s_image_available[i]);
            result = m_ctx.device().createSemaphore(&sci, nullptr, &m_sync.s_render_finished[i]);
            result = m_ctx.device().createFence(&fci, nullptr, &m_sync.f_in_flight[i]);
        }
    }

    void Swapchain::cleanup()
    {
        for (auto imageview : m_views)
        {
            m_ctx.device().destroy(imageview);
        }

        m_ctx.device().destroy(m_swapchain);
    }

    vk::Rect2D Swapchain::make_scissor() const
    {
        return { { 0, 0 }, m_extent};
    }

    vk::Viewport Swapchain::make_viewport() const
    {
        vk::Viewport vp;

        vp.setX(0.0f);
        vp.setWidth(static_cast<float>(m_extent.width));
        vp.setY(static_cast<float>(m_extent.height));
        vp.setHeight(-1.0f * static_cast<float>(m_extent.height));
        vp.setMaxDepth(1.0f);
        vp.setMinDepth(0.0f);

        return vp;
    }

    const vk::Image& Swapchain::image(uint32_t id) const
    {
        if (id > m_images.size())
        {
            throw std::out_of_range("Swapchain image index \"" + std::to_string(id) + "\" out of bounds.");
        }
        return m_images[id];
    }

    const vk::Image& Swapchain::operator[](uint32_t id) const
    {
        return Swapchain::image(id);
    }

    const vk::ImageView& Swapchain::view(uint32_t id) const
    {
        if (id > m_views.size())
        {
            throw std::out_of_range("Swapchain image view index \"" + std::to_string(id) + "\" out of bounds.");
        }
        return m_views[id];
    }

    uint32_t Swapchain::acquire_frame(uint32_t current_frame) const
    {
        vk::Result result;
        auto fence = m_sync.f_in_flight[current_frame];
        result = m_ctx.device().waitForFences(1, &fence, true, std::numeric_limits<uint64_t>::max());
        result = m_ctx.device().resetFences(1, &fence);
        return m_ctx.device().acquireNextImageKHR(m_swapchain,
                                                  std::numeric_limits<uint64_t>::max(),
                                                  m_sync.s_image_available[current_frame],
                                                  nullptr).value;
    }

    void Swapchain::submit_and_present(uint32_t current_frame, uint32_t acquired_frame, vk::CommandBuffer const& command_buffer) const
    {
        vk::Result result;
        vk::Semaphore wait_semaphores[] = { m_sync.s_image_available[current_frame] };
        vk::Semaphore signal_semaphores[] = { m_sync.s_render_finished[current_frame] };
        vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        vk::SubmitInfo submit_info;
        submit_info.setWaitSemaphoreCount(1);
        submit_info.setPWaitSemaphores(wait_semaphores);
        submit_info.setPWaitDstStageMask(wait_stages);
        submit_info.setCommandBufferCount(1);
        submit_info.setCommandBuffers(command_buffer);
        submit_info.setSignalSemaphoreCount(1);
        submit_info.setPSignalSemaphores(signal_semaphores);
        result = m_ctx.q_graphics().queue.submit(1, &submit_info, m_sync.f_in_flight[current_frame]);

        vk::PresentInfoKHR present_info;
        present_info.setWaitSemaphoreCount(1);
        present_info.setPWaitSemaphores(signal_semaphores);
        present_info.setSwapchainCount(1);
        present_info.setPSwapchains(&m_swapchain);
        present_info.setImageIndices(acquired_frame);
        present_info.setPResults(nullptr);
        result = m_ctx.q_present().queue.presentKHR(&present_info);
    }
}

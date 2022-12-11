#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Device.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Vulkan/Image/ImageView.hpp>
#include <Vulkan/GraphicsPipeline/RenderPass.hpp>
#include <Utility/Macro.hpp>
#include <vk/Image.hpp>

class Swapchain {
public:
    NON_COPIABLE(Swapchain)

    Swapchain(const Device&      device,
              vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eMailbox,
              uint32_t           frames = 2);

#pragma region frame_buffers
    /**
     * @brief Build frame buffers after a render pass and depth buffer have been created.
     */
    void createFrameBuffers(const RenderPass& renderPass, const re::DepthBuffer& depthBuffer);

    /**
     * @brief Get framebuffer for the frame specified by index.
     */
    vk::Framebuffer framebuffer(size_t idx) const { return mFrameBuffers[idx]; }
#pragma endregion

#pragma region swap_chain_properties
    /**
     * @brief Return aspect ratio calculated from swap chain extent
     */
    float aspectRatio() const { return (float) mExtent.width / (float) mExtent.height; }

    vk::Extent2D extent() const { return mExtent; }

    vk::Format format() const { return mFormat; }

    uint32_t image_count() const { return mImages.size(); }
#pragma endregion

#pragma region scissor_viewport
    /**
     * @brief Default scissor based on swap chain extent.
     */
    vk::Rect2D make_scissor() const
    {
        vk::Rect2D scissor;
        scissor.setExtent(mExtent);
        scissor.setOffset({ 0, 0 });
        return scissor;
    }

    /**
     * @brief Default viewport based on swap chain extent
     */
    vk::Viewport make_viewport() const
    {
        vk::Viewport viewport;
        viewport.setX(0.0f);
        viewport.setY(0.0f);
        viewport.setWidth((float) mExtent.width);
        viewport.setHeight((float) mExtent.height);
        viewport.setMaxDepth(1.0f);
        viewport.setMinDepth(0.0f);
        return viewport;
    }

    vk::Viewport make_negative_viewport() const
    {
        vk::Viewport viewport;
        viewport.setX(0.0f);
        viewport.setY((float) mExtent.height);
        viewport.setWidth((float) mExtent.width);
        viewport.setHeight(-(float) mExtent.height);
        viewport.setMaxDepth(1.0f);
        viewport.setMinDepth(0.0f);
        return viewport;
    }
#pragma endregion

    const vk::SwapchainKHR& handle() const { return mSwapchain; }

    const vk::Image& image(size_t i) const { return mImages[i]; }

    const vk::ImageView& view(size_t i ) const { return mImageViews[i]; }

    void destroy();

private:
    struct SwapChainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities {};
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport();

    static vk::SurfaceFormatKHR pickFormat(const std::vector<vk::SurfaceFormatKHR>& formats);

    static vk::PresentModeKHR pickPresentMode(const std::vector<vk::PresentModeKHR>& presentModes,
                                              const vk::PresentModeKHR& preferred);

    vk::Extent2D pickExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

private:
    vk::SwapchainKHR   mSwapchain;

    vk::PresentModeKHR mPresentMode;
    vk::Format         mFormat {};
    vk::Extent2D       mExtent {};

    std::vector<vk::Image>       mImages;
    std::vector<vk::ImageView>   mImageViews;
    std::vector<vk::Framebuffer> mFrameBuffers;

    uint32_t mMinImages {};

    const vk::PhysicalDevice& mPhysicalDevice;

    const Device& mDevice;
};
#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "ImageView.hpp"
#include "GraphicsPipeline/RenderPass.hpp"
#include "DepthBuffer.hpp"
#include "../Macro.hpp"

class Swapchain {
public:
    NON_COPIABLE(Swapchain)

    explicit Swapchain(const Device& device, vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eMailbox);

    void createFrameBuffers(const RenderPass& renderPass, const DepthBuffer& depthBuffer);

    vk::Framebuffer framebuffer(size_t idx) const { return mFrameBuffers[idx]; }

    vk::SwapchainKHR handle() const { return mSwapchain; }

    const std::vector<vk::ImageView>& imageViews() const { return mImageViews; }

    const std::vector<vk::Image>& images() const { return mImages; }

    vk::PresentModeKHR presentMode() const { return mPresentMode; }

    vk::Extent2D extent() const { return mExtent; }

    float aspectRatio() const { return (float) mExtent.width / (float) mExtent.height; }

    vk::Format format() const { return mFormat; }

    vk::PhysicalDevice physicalDevice() const { return mPhysicalDevice; }

    const Device& device() const { return mDevice; }

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

    const vk::PhysicalDevice mPhysicalDevice;

    const Device& mDevice;
};
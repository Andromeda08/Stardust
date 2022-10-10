#include "Application.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include "Window.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/Swapchain.hpp"

Application::Application(const ApplicationSettings& app_settings)
: mSettings(app_settings)
{
    mWindow   = std::make_unique<Window>(app_settings.windowSettings);
    mInstance = std::make_unique<Instance>(*mWindow);
    mSurface  = std::make_unique<Surface>(*mInstance);

    selectRayTracingDevice();

    createSwapChain();
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow->handle()))
    {
        glfwPollEvents();

        // TODO: Draw Frame

        mDevice->waitIdle();
    }

    Application::cleanup();
}

void Application::selectRayTracingDevice()
{
    const auto& physicalDevices = mInstance->physicalDevices();

    const auto result = std::find_if(
        std::begin(physicalDevices), std::end(physicalDevices),
        [](const vk::PhysicalDevice& device) {
            // 1. Query ray tracing support
            const auto extensions = device.enumerateDeviceExtensionProperties();
            const auto hasRayTracing = std::find_if(
                std::begin(extensions), std::end(extensions),
                [](const vk::ExtensionProperties& extensionProperties) {
                    return std::string(extensionProperties.extensionName.data()) == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
                });

            // 2. Query if the device has a graphics queue or not
            const auto queueFamilies = device.getQueueFamilyProperties();
            const auto hasGraphicsQueue = std::find_if(
                std::begin(queueFamilies), std::end(queueFamilies),
                [](const vk::QueueFamilyProperties& queueFamilyProperties) {
                    return queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics;
                });

            return (hasRayTracing != std::end(extensions))
                && (hasGraphicsQueue != std::end(queueFamilies));
        });

    if (result == std::end(physicalDevices))
    {
        throw std::runtime_error("Couldn't find a ray tracing capable device.");
    }

    if (mSettings.logging)
    {
        Application::printDevices();
        auto props = result->getProperties();
        std::cout << "Selected device: " << props.deviceName << std::endl;
    }

    const auto extensions = mRequiredDeviceExtensions;
    mDevice = std::make_unique<Device>(*mSurface, *result, extensions);
}

void Application::createSwapChain()
{
    mSwapChain = std::make_unique<Swapchain>(*mDevice);

    for (uint32_t i = 0; i < mSwapChain->imageViews().size(); i++)
    {
        mImageAvailableSemaphores.emplace_back(*mDevice);
        mRenderFinishedSemaphores.emplace_back(*mDevice);
        mInFlightFences.emplace_back(*mDevice);
    }
}

void Application::cleanup()
{
    mSwapChain->destroy();
}

#pragma region printer_utilities

void Application::printDevices()
{
    const auto& foundDevices = mInstance->physicalDevices();
    std::cout << "Detected devices:" << std::endl;
    for (const auto& pd : foundDevices) {
        auto props = pd.getProperties();
        std::cout << "- " << props.deviceName << " [" << to_string(props.deviceType) << "]" << std::endl;
    }
}

#pragma endregion

#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "Macro.hpp"
#include "Struct/ApplicationSettings.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Synchronization/Fence.hpp"
#include "Vulkan/Synchronization/Semaphore.hpp"

class Application {
public:
    NON_COPIABLE(Application)

    explicit Application(const ApplicationSettings& app_settings);

    /**
     * @brief Application main loop
     */
    void run();

    std::vector<const char*> deviceExtensions() const { return mRequiredDeviceExtensions; }

private:
    /**
     * @brief Selects a Physical Device that supports VK_KHR_ray_tracing_pipeline and has a Graphics Queue
     * then attempts to create a Logical Device.
     */
    void selectRayTracingDevice();

    /**
     * @brief Builds the swap chain and synchronization objects
     */
    void createSwapChain();

    /**
     * @brief Free resources.
     */
    void cleanup();

    #pragma region print_utilities

    /**
     * @brief Prints the list of detected physical devices.
     */
    void printDevices();

    #pragma endregion

private:
    ApplicationSettings mSettings {};

    std::unique_ptr<class Window>    mWindow;
    std::unique_ptr<class Instance>  mInstance;
    std::unique_ptr<class Surface>   mSurface;
    std::unique_ptr<class Device>    mDevice;
    std::unique_ptr<Swapchain>       mSwapChain;

    // Vulkan synchronization objects
    std::vector<Fence>     mInFlightFences = {};
    std::vector<Semaphore> mRenderFinishedSemaphores = {};
    std::vector<Semaphore> mImageAvailableSemaphores = {};

    uint32_t mCurrentFrame {};

    // Device extensions for ray tracing
    const std::vector<const char*> mRequiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
};